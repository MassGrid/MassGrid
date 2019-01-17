#include"dockeredge.h"
#include "n2n/include/n2n.h"
#include "n2n/include/n2n_transforms.h"
#include <assert.h>
#include <sys/stat.h>
#include "n2n/include/minilzo.h"

#include "n2n/include/scm.h"

int n2n_main(int, char **);
int n2n_stop(void *);

struct SCM_def sd;

#if defined(DEBUG)
#define SOCKET_TIMEOUT_INTERVAL_SECS    5
#define DEFAULT_HOLEPUNCH_INTERVAL      20 /* sec */
#else  /* #if defined(DEBUG) */
#define SOCKET_TIMEOUT_INTERVAL_SECS    5
#define DEFAULT_HOLEPUNCH_INTERVAL      25 /* sec */
#endif /* #if defined(DEBUG) */

#define REGISTER_SUPER_INTERVAL_MIN     10   /* sec */
#define REGISTER_SUPER_INTERVAL_MAX     120  /* sec */

#define IFACE_UPDATE_INTERVAL           (30) /* sec. How long it usually takes to get an IP lease. */
#define TRANSOP_TICK_INTERVAL           (10) /* sec */

#define STAT_CALC_INTERVAL				(10) /* sec. Calculate the bps values in roughly this interval steps */

/** maximum length of command line arguments */
#define MAX_CMDLINE_BUFFER_LENGTH    4096

/** maximum length of a line in the configuration file */
#define MAX_CONFFILE_LINE_LENGTH        1024

#define N2N_PATHNAME_MAXLEN             256
#define N2N_MAX_TRANSFORMS              16
#define N2N_EDGE_MGMT_PORT              5644
#define N2N_SN_PROT                     8999

/** Positions in the transop array where various transforms are stored.
 *
 *  Used by transop_enum_to_index(). See also the transform enumerations in
 *  n2n_transforms.h */
#define N2N_TRANSOP_NULL_IDX    0
#define N2N_TRANSOP_TF_IDX      1
#define N2N_TRANSOP_AESCBC_IDX  2
/* etc. */



/* Work-memory needed for compression. Allocate memory in units
 * of `lzo_align_t' (instead of `char') to make sure it is properly aligned.
 */

/* #define HEAP_ALLOC(var,size)						\ */
/*   lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ] */

/* static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS); */

/* ******************************************************* */

#define N2N_EDGE_SN_HOST_SIZE 48
#define N2N_EDGE_LOCAL_IP_SIZE 48

typedef char n2n_sn_name_t[N2N_EDGE_SN_HOST_SIZE];
typedef char n2n_local_ip_t[N2N_EDGE_LOCAL_IP_SIZE];

#define N2N_EDGE_NUM_SUPERNODES 2
#define N2N_EDGE_SUP_ATTEMPTS   3       /* Number of failed attmpts before moving on to next supernode. */

std::function<void(bool)> fstart = nullptr;
boost::thread* thrd = NULL;
std::string strCommunity;
std::string strLocalAddr;
std::string strSnAddr;
int   keep_running=1;
/** Main structure type for edge. */
struct n2n_edge
{
    int                 daemon;                 /**< Non-zero if edge should detach and run in the background. */
    uint8_t             re_resolve_supernode_ip;

    n2n_sock_t          supernode;
    n2n_sock_t          local_sock;

    size_t              sn_idx;                 /**< Currently active supernode. */
    size_t              sn_num;                 /**< Number of supernode addresses defined. */
    n2n_sn_name_t       sn_ip_array[N2N_EDGE_NUM_SUPERNODES];
    n2n_local_ip_t      local_ip_str;          /** storing a local ip socket */
    int                 local_sock_ena;        /** > 0 if local_sock is enabled */
    int                 sn_wait;                /**< Whether we are waiting for a supernode response. */

    n2n_community_t     community_name;         /**< The community. 16 full octets. */
    char                keyschedule[N2N_PATHNAME_MAXLEN];
    int                 null_transop;           /**< Only allowed if no key sources defined. */

    int                 udp_sock;
    int                 udp_mgmt_sock;          /**< socket for status info. */

    tuntap_dev          device;                 /**< All about the TUNTAP device */
    int                 dyn_ip_mode;            /**< Interface IP address is dynamically allocated, eg. DHCP. */
    int                 allow_routing;          /**< Accept packet no to interface address. */
    int                 drop_multicast;         /**< Multicast ethernet addresses. */

    n2n_trans_op_t      transop[N2N_MAX_TRANSFORMS]; /* one for each transform at fixed positions */
    size_t              tx_transop_idx;         /**< The transop to use when encoding. */

	struct peer_info *  known_peers[PEER_HASH_TAB_SIZE];            /**< Edges we are connected to. */
    struct peer_info *  pending_peers[PEER_HASH_TAB_SIZE];          /**< Edges we have tried to register with. */
    time_t              last_register_req;      /**< Check if time to re-register with super*/
    size_t              holepunch_interval;      /**< Time distance after last_register_req at which to re-register. */
    time_t              last_purge;             /** last time clients were purged **/
    time_t              last_p2p;               /**< Last time p2p traffic was received. */
    time_t              last_sup;               /**< Last time a packet arrived from supernode. */
    size_t              sup_attempts;           /**< Number of remaining attempts to this supernode. */
    n2n_cookie_t        last_cookie;            /**< Cookie sent in last REGISTER_SUPER. */

    time_t              start_time;             /**< For calculating uptime */

    /* Statistics */
    size_t              tx_p2p;
	size_t				tx_bit_p2p;
	size_t				tx_bps_p2p;
    size_t              rx_p2p;
	size_t				rx_bit_p2p;
	size_t				rx_bps_p2p;
    size_t              tx_sup;
	size_t				tx_bit_sup;
	size_t				tx_bps_sup;
    size_t              rx_sup;
	size_t				rx_bit_sup;
	size_t				rx_bps_sup;
};

/** Return the IP address of the current supernode in the ring. */
static const char * supernode_ip( const n2n_edge_t * eee )
{
    return (eee->sn_ip_array)[eee->sn_idx];
}


static void supernode2addr(n2n_sock_t * sn, const n2n_sn_name_t addr);
static int localip2addr(n2n_sock_t * l_ip,
        const n2n_local_ip_t local_ip_strIn, int local_port);
static int set_localip(n2n_edge_t * eee);

static void send_packet2net(n2n_edge_t * eee,
			    uint8_t *decrypted_msg, size_t len);


/* ************************************** */

/* parse the configuration file */
// static int readConfFile(const char * filename, char * const linebuffer) {
//   struct stat stats;
//   FILE    *   fd;
//   char    *   buffer = NULL;

//   buffer = (char *)malloc(MAX_CONFFILE_LINE_LENGTH);
//   if (!buffer) {
//     //traceEvent( TRACE_ERROR, "Unable to allocate memory");
//     return -1;
//   }

//   if (stat(filename, &stats)) {
//     if (errno == ENOENT)
//       //traceEvent(TRACE_ERROR, "parameter file %s not found/unable to access\n", filename);
//     else
//       //traceEvent(TRACE_ERROR, "cannot stat file %s, errno=%d\n",filename, errno);
//     free(buffer);
//     return -1;
//   }

//   fd = fopen(filename, "rb");
//   if (!fd) {
//     //traceEvent(TRACE_ERROR, "Unable to open parameter file '%s' (%d)...\n",filename,errno);
//     free(buffer);
//     return -1;
//   }
//   while(fgets(buffer, MAX_CONFFILE_LINE_LENGTH,fd)) {
//     char    *   p = NULL;

//     /* strip out comments */
//     p = strchr(buffer, '#');
//     if (p) *p ='\0';

//     /* remove \n */
//     p = strchr(buffer, '\n');
//     if (p) *p ='\0';

//     /* strip out heading spaces */
//     p = buffer;
//     while(*p == ' ' && *p != '\0') ++p;
//     if (p != buffer) strncpy(buffer,p,strlen(p)+1);

//     /* strip out trailing spaces */
//     while(strlen(buffer) && buffer[strlen(buffer)-1]==' ')
//       buffer[strlen(buffer)-1]= '\0';

//     /* check for nested @file option */
//     if (strchr(buffer, '@')) {
//       //traceEvent(TRACE_ERROR, "@file in file nesting is not supported\n");
//       free(buffer);
//       return -1;
//     }
//     if ((strlen(linebuffer)+strlen(buffer)+2)< MAX_CMDLINE_BUFFER_LENGTH) {
//       strncat(linebuffer, " ", 1);
//       strncat(linebuffer, buffer, strlen(buffer));
//     } else {
//       //traceEvent(TRACE_ERROR, "too many argument");
//       free(buffer);
//       return -1;
//     }
//   }

//   free(buffer);
//   fclose(fd);

//   return 0;
// }

/* Create the argv vector */
/* FIXME
 * - this function does not handle quoted parameters
 * Worse still, by this point, we have removed any indications of what the
 * original params were - so quote handling performed by the shell is gone..
 */
static char ** buildargv(int * effectiveargc, char * const linebuffer) {
  const int  INITIAL_MAXARGC = 16;	/* Number of args + NULL in initial argv */
  int     maxargc;
  int     argc=0;
  char ** argv;
  char *  buffer, * buff;

  *effectiveargc = 0;
  buffer = (char *)calloc(1, strlen(linebuffer)+2);
  if (!buffer) {
    LogPrint("edge","Unable to allocate memory\n");
    return NULL;
  }
  strncpy(buffer, linebuffer,strlen(linebuffer));

  maxargc = INITIAL_MAXARGC;
  argv = (char **)malloc(maxargc * sizeof(char*));
  if (argv == NULL) {
    // //traceEvent( TRACE_ERROR, "Unable to allocate memory");
    return NULL;
  }
  buff = buffer;
  while(buff) {
    char * p = strchr(buff,' ');
    if (p) {
      *p='\0';
      argv[argc++] = strdup(buff);
      while(*++p == ' ' && *p != '\0');
      buff=p;
      if (argc >= maxargc) {
	maxargc *= 2;
	argv = (char **)realloc(argv, maxargc * sizeof(char*));
	if (argv == NULL) {
	//   //traceEvent(TRACE_ERROR, "Unable to re-allocate memory");
	  free(buffer);
	  return NULL;
	}
      }
    } else {
      argv[argc++] = strdup(buff);
      break;
    }
  }
  free(buffer);
  *effectiveargc = argc;
  return argv;
}



/* ************************************** */


/** Initialise an edge to defaults.
 *
 *  This also initialises the NULL transform operation opstruct.
 */
static int edge_init(n2n_edge_t * eee)
{
#ifdef WIN32
    initWin32();
#endif
    memset(eee, 0, sizeof(n2n_edge_t));
    eee->start_time = time(NULL);

    transop_null_init(    &(eee->transop[N2N_TRANSOP_NULL_IDX]) );
    transop_twofish_init( &(eee->transop[N2N_TRANSOP_TF_IDX]  ) );
    transop_aes_init( &(eee->transop[N2N_TRANSOP_AESCBC_IDX]  ) );

    eee->tx_transop_idx = N2N_TRANSOP_NULL_IDX; /* No guarantee the others have been setup */

    eee->daemon = 0;    /* By default run in daemon mode. */
    eee->re_resolve_supernode_ip = 0;
    /* keyschedule set to NULLs by memset */
    /* community_name set to NULLs by memset */
    eee->null_transop   = 0;
    eee->udp_sock       = -1;
    eee->udp_mgmt_sock  = -1;
    eee->dyn_ip_mode    = 0;
    eee->allow_routing  = 0;
    eee->drop_multicast = 1;
    eee->local_sock_ena = 0;
	sglib_hashed_peer_info_t_init(eee->known_peers);
	sglib_hashed_peer_info_t_init(eee->pending_peers);
    eee->last_register_req = 0;
    eee->holepunch_interval = DEFAULT_HOLEPUNCH_INTERVAL;
    eee->last_p2p = 0;
    eee->last_sup = 0;
    eee->last_purge = 0;
    eee->sup_attempts = N2N_EDGE_SUP_ATTEMPTS;
	eee->tx_bit_p2p = 0;
	eee->tx_bit_sup = 0;
	eee->rx_bit_p2p = 0;
	eee->rx_bit_sup = 0;

    if(lzo_init() != LZO_E_OK)
    {
        //traceEvent(TRACE_ERROR, "LZO compression error");
        return(-1);
    }

    return(0);
}



/* Called in main() after options are parsed. */
static int edge_init_twofish( n2n_edge_t * eee, uint8_t *encrypt_pwd, uint32_t encrypt_pwd_len )
{
    return transop_twofish_setup( &(eee->transop[N2N_TRANSOP_TF_IDX]), 0, encrypt_pwd, encrypt_pwd_len );
}


/** Find the transop op-struct for the transform enumeration required.
 *
 * @return - index into the transop array, or -1 on failure.
 */
static int transop_enum_to_index( n2n_transform_t id )
{
    switch (id)
    {
    case N2N_TRANSFORM_ID_TWOFISH:
        return N2N_TRANSOP_TF_IDX;
        break;
    case N2N_TRANSFORM_ID_NULL:
        return N2N_TRANSOP_NULL_IDX;
        break;
    case N2N_TRANSFORM_ID_AESCBC:
        return N2N_TRANSOP_AESCBC_IDX;
        break;
    default:
        return -1;
    }
}


/** Called periodically to roll keys and do any periodic maintenance in the
 *  tranform operations state machines. */
static int n2n_tick_transop( n2n_edge_t * eee, time_t now )
{
    n2n_tostat_t tst;
    size_t trop = eee->tx_transop_idx;

    /* Tests are done in order that most preferred transform is last and causes
     * tx_transop_idx to be left at most preferred valid transform. */
    tst = (eee->transop[N2N_TRANSOP_NULL_IDX].tick)( &(eee->transop[N2N_TRANSOP_NULL_IDX]), now );
    tst = (eee->transop[N2N_TRANSOP_AESCBC_IDX].tick)( &(eee->transop[N2N_TRANSOP_AESCBC_IDX]), now );
    if ( tst.can_tx )
    {
        //traceEvent( TRACE_DEBUG, "can_tx AESCBC (idx=%u)", (unsigned int)N2N_TRANSOP_AESCBC_IDX );
        trop = N2N_TRANSOP_AESCBC_IDX;
    }

    tst = (eee->transop[N2N_TRANSOP_TF_IDX].tick)( &(eee->transop[N2N_TRANSOP_TF_IDX]), now );
    if ( tst.can_tx )
    {
        //traceEvent( TRACE_DEBUG, "can_tx TF (idx=%u)", (unsigned int)N2N_TRANSOP_TF_IDX );
        trop = N2N_TRANSOP_TF_IDX;
    }

    if ( trop != eee->tx_transop_idx )
    {
        eee->tx_transop_idx = trop;
        //traceEvent( TRACE_NORMAL, "Chose new tx_transop_idx=%u", (unsigned int)(eee->tx_transop_idx) );
    }

    return 0;
}



/** Read in a key-schedule file, parse the lines and pass each line to the
 *  appropriate trans_op for parsing of key-data and adding key-schedule
 *  entries. The lookup table of time->trans_op is constructed such that
 *  encoding can be passed to the correct trans_op. The trans_op internal table
 *  will then determine the best SA for that trans_op from the key schedule to
 *  use for encoding. */
static int edge_init_keyschedule( n2n_edge_t * eee )
{

#define N2N_NUM_CIPHERSPECS 32

    int retval = -1;
    ssize_t numSpecs=0;
    n2n_cipherspec_t specs[N2N_NUM_CIPHERSPECS];
    size_t i;
    time_t now = time(NULL);

    numSpecs = n2n_read_keyfile( specs, N2N_NUM_CIPHERSPECS, eee->keyschedule );

    if ( numSpecs > 0 )
    {
        //traceEvent( TRACE_NORMAL, "keyfile = %s read -> %d specs.\n", optarg, (signed int)numSpecs);

        for ( i=0; i < (size_t)numSpecs; ++i )
        {
            int idx;

            idx = transop_enum_to_index( specs[i].t );

            switch (idx) 
            {
            case N2N_TRANSOP_TF_IDX:
            case N2N_TRANSOP_AESCBC_IDX:
            {
                retval = (eee->transop[idx].addspec)( &(eee->transop[idx]),
                                                      &(specs[i]) );
                break;
            }
            default:
                retval = -1;
            }

            if (0 != retval)
            {
                //traceEvent( TRACE_ERROR, "keyschedule failed to add spec[%u] to transop[%d].\n", 
                            // (unsigned int)i, idx);

                return retval;
            }
        }

        n2n_tick_transop( eee, now );
    }
    else
    {
        //traceEvent( TRACE_ERROR, "Failed to process '%s'", eee->keyschedule );
    }

    return retval;
}


/** Deinitialise the edge and deallocate any owned memory. */
static void edge_deinit(n2n_edge_t * eee)
{
    if ( eee->udp_sock >=0 )
    {
        closesocket( eee->udp_sock );
    }

    if ( eee->udp_mgmt_sock >= 0 )
    {
        closesocket(eee->udp_mgmt_sock);
    }

    clear_hashed_peer_info_t_list( eee->pending_peers );
    clear_hashed_peer_info_t_list( eee->known_peers );

    (eee->transop[N2N_TRANSOP_TF_IDX].deinit)(&eee->transop[N2N_TRANSOP_TF_IDX]);
    (eee->transop[N2N_TRANSOP_NULL_IDX].deinit)(&eee->transop[N2N_TRANSOP_NULL_IDX]);
}

static void readFromIPSocket( n2n_edge_t * eee );

static void readFromMgmtSocket( n2n_edge_t * eee, int * keep_running );


/** Send a datagram to a socket defined by a n2n_sock_t */
static ssize_t sendto_sock( int fd, const void * buf, size_t len, const n2n_sock_t * dest )
{
    n2n_sock_str_t sockbuf;
    struct sockaddr_in peer_addr;
    ssize_t sent;

    fill_sockaddr( (struct sockaddr *) &peer_addr,
                   sizeof(peer_addr),
                   dest );

    sent = sendto( fd, buf, len, 0/*flags*/,
                   (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in) );
    if ( sent < 0 )
    {
        char * c = strerror(errno);
        LogPrintf("sendto %s failed (%d) %s\n",
                sock_to_cstr( sockbuf, dest ), errno, c );
    }
    else
    {
        //traceEvent( TRACE_DEBUG, "sendto sent=%d to ", (signed int)sent );
    }

    return sent;
}


/** Send a REGISTER packet to another edge. */
static void send_register( n2n_edge_t * eee,
                           const n2n_sock_t * remote_peer,
                           const n2n_mac_t dstMac)
{
    uint8_t pktbuf[N2N_PKT_BUF_SIZE];
    size_t idx;
    n2n_common_t cmn = {0};
    n2n_REGISTER_t reg = {{0}};
    n2n_sock_str_t sockbuf;

    cmn.ttl=N2N_DEFAULT_TTL;
    cmn.pc = n2n_register;
    cmn.flags = 0;
    memcpy( cmn.community, eee->community_name, N2N_COMMUNITY_SIZE );

    /* NOTE: Encoding should probably not be done here but in 
     * encode_REGISTER (lukas) */
    idx=0;
    encode_uint32( reg.cookie, &idx, 123456789 );
    idx=0;
    encode_mac( reg.srcMac, &idx, eee->device.mac_addr );
    if(dstMac) {
        idx=0;
        encode_mac( reg.dstMac, &idx, dstMac );
    }

    idx=0;
    encode_REGISTER( pktbuf, &idx, &cmn, &reg );

    LogPrint( "edge", "send REGISTER %s\n",
                sock_to_cstr( sockbuf, remote_peer ) );


    sendto_sock( eee->udp_sock, pktbuf, idx, remote_peer );
}

/** Send a QUERY_PEER packet to the current supernode. */
static void send_query_peer( n2n_edge_t * eee,
                             const n2n_mac_t dstMac)
{
    uint8_t pktbuf[N2N_PKT_BUF_SIZE];
    size_t idx;
    n2n_common_t cmn = {0};
    n2n_QUERY_PEER_t query = {{0}};

    cmn.ttl=N2N_DEFAULT_TTL;
    cmn.pc = n2n_query_peer;
    cmn.flags = 0;
    memcpy( cmn.community, eee->community_name, N2N_COMMUNITY_SIZE );

    idx=0;
    encode_mac( query.srcMac, &idx, eee->device.mac_addr );
    idx=0;
    encode_mac( query.targetMac, &idx, dstMac );

    idx=0;
    encode_QUERY_PEER( pktbuf, &idx, &cmn, &query );

    //traceEvent( TRACE_INFO, "send QUERY_PEER to supernode" );

    sendto_sock( eee->udp_sock, pktbuf, idx, &(eee->supernode) );
}

/** Send a REGISTER_SUPER packet to the current supernode. */
static void send_register_super( n2n_edge_t * eee,
                                 const n2n_sock_t * supernode)
{
    uint8_t pktbuf[N2N_PKT_BUF_SIZE];
    size_t idx;
    n2n_common_t cmn = {0};
    n2n_REGISTER_SUPER_t reg = {0};
    n2n_sock_str_t sockbuf;

    cmn.ttl=N2N_DEFAULT_TTL;
    cmn.pc = n2n_register_super;
    cmn.flags = 0;
    memcpy( cmn.community, eee->community_name, N2N_COMMUNITY_SIZE );

    for( idx=0; idx < N2N_COOKIE_SIZE; ++idx )
    {
        eee->last_cookie[idx] = rand() % 0xff;
    }

    reg.aflags = 0;
    memcpy( reg.cookie, eee->last_cookie, N2N_COOKIE_SIZE );
    reg.auth.scheme=0; /* No auth yet */
    reg.timeout = eee->holepunch_interval;

    idx=0;
    encode_mac( reg.edgeMac, &idx, eee->device.mac_addr );

    if(eee->local_sock_ena) {
        reg.aflags |= N2N_AFLAGS_LOCAL_SOCKET;
        reg.local_sock = eee->local_sock;
    }

    idx=0;
    encode_REGISTER_SUPER( pktbuf, &idx, &cmn, &reg );

    LogPrint( "edge", "send REGISTER_SUPER to %s\n",
                sock_to_cstr( sockbuf, supernode ) );


    sendto_sock( eee->udp_sock, pktbuf, idx, supernode );
}


/** Send a REGISTER_ACK packet to a peer edge. */
static void send_register_ack( n2n_edge_t * eee,
                               const n2n_sock_t * remote_peer,
                               const n2n_REGISTER_t * reg )
{
    uint8_t pktbuf[N2N_PKT_BUF_SIZE];
    size_t idx;
    n2n_common_t cmn;
    n2n_REGISTER_ACK_t ack;
    n2n_sock_str_t sockbuf;

    memset(&cmn, 0, sizeof(cmn) );
    memset(&ack, 0, sizeof(reg) );
    cmn.ttl=N2N_DEFAULT_TTL;
    cmn.pc = n2n_register_ack;
    cmn.flags = 0;
    memcpy( cmn.community, eee->community_name, N2N_COMMUNITY_SIZE );

    memset( &ack, 0, sizeof(ack) );
    memcpy( ack.cookie, reg->cookie, N2N_COOKIE_SIZE );
    memcpy( ack.srcMac, eee->device.mac_addr, N2N_MAC_SIZE );
    memcpy( ack.dstMac, reg->srcMac, N2N_MAC_SIZE );

    idx=0;
    encode_REGISTER_ACK( pktbuf, &idx, &cmn, &ack );

    LogPrint( "edge", "send REGISTER_ACK %s\n",
                sock_to_cstr( sockbuf, remote_peer ) );


    sendto_sock( eee->udp_sock, pktbuf, idx, remote_peer );
}


/** NOT IMPLEMENTED
 *
 *  This would send a DEREGISTER packet to a peer edge or supernode to indicate
 *  the edge is going away.
 */
static void send_deregister(n2n_edge_t * eee,
                            n2n_sock_t * remote_peer)
{
    /* Marshall and send message */
}


static int is_empty_ip_address( const n2n_sock_t * sock );
static void update_peer_address(n2n_edge_t * eee,
                                uint8_t from_supernode,
                                const n2n_mac_t mac,
                                const n2n_sock_t * peer,
                                time_t when);
void check_peer( n2n_edge_t * eee,
                 uint8_t from_supernode,
                 const n2n_mac_t mac,
                 const n2n_sock_t * peer );
void establish_connection( n2n_edge_t * eee,
                        const n2n_mac_t mac );
void set_peer_operational( n2n_edge_t * eee,
                           const n2n_mac_t mac,
                           const n2n_sock_t * peer );



/** Start the registration process.
 *
 *  If the peer is already in pending_peers, ignore the request.
 *  If not in pending_peers, add it and query info about it from supernode
 *
 *  If hdr is for a direct peer-to-peer packet, try to register back to sender
 *  even if the MAC is in pending_peers. This is because an incident direct
 *  packet indicates that peer-to-peer exchange should work so more aggressive
 *  registration can be permitted (once per incoming packet) as this should only
 *  last for a small number of packets..
 *
 *  Called from the main loop when Rx a packet for our device mac.
 */
void establish_connection( n2n_edge_t * eee,
                        const n2n_mac_t mac )
{
    struct peer_info * scan = find_peer_by_mac( eee->pending_peers, mac );
    macstr_t mac_buf;
    n2n_sock_str_t sockbuf;

    if ( NULL == scan )
    {
        time_t now = time(NULL);
        scan = (struct peer_info *)calloc( 1, sizeof( struct peer_info ) );

        memcpy(scan->mac_addr, mac, N2N_MAC_SIZE);
        scan->num_sockets = 0;
        scan->sock = eee->supernode;
        scan->last_seen = now; /* Don't change this it marks the pending peer for removal. */

		sglib_hashed_peer_info_t_add(eee->pending_peers, scan);

        LogPrint( "edge", "=== new pending %s -> %s\n",
                    macaddr_str( mac_buf, scan->mac_addr ),
                    sock_to_cstr( sockbuf, &scan->sock ) );

        LogPrint( "edge", "Pending peers list size=%u\n",
			(unsigned int)hashed_peer_list_t_size( eee->pending_peers ) );

        /* trace Sending REGISTER */

        send_query_peer(eee, scan->mac_addr);
        scan->last_sent_query = now;

        /* pending_peers now owns scan. */
    }
    else
    {
    }
}


/** Update the last_seen time for this peer, or get registered. */
void check_peer( n2n_edge_t * eee,
                 uint8_t from_supernode,
                 const n2n_mac_t mac,
                 const n2n_sock_t * peer)
{
	peer_info_t scan;
	memcpy(scan.mac_addr, mac, sizeof(n2n_mac_t));
    
	if (sglib_hashed_peer_info_t_find_member(eee->known_peers, &scan) == NULL)
    {
        /* Not in known_peers - start the REGISTER process. */
        establish_connection( eee, mac );
    }
    else
    {
        /* Already in known_peers. */
        update_peer_address( eee, from_supernode, mac, peer, time(NULL) );
    }
}


/* Move the peer from the pending_peers list to the known_peers lists.
 *
 * peer must be a pointer to an element of the pending_peers list.
 *
 * Called by main loop when Rx a REGISTER_ACK.
 */
void set_peer_operational( n2n_edge_t * eee,
                        const n2n_mac_t mac,
                        const n2n_sock_t * peer )
{
    peer_info_t tmp;
    peer_info_t *scan;
    macstr_t mac_buf;
    n2n_sock_str_t sockbuf;

    LogPrint( "edge", "set_peer_operational: %s -> %s\n",
                macaddr_str( mac_buf, mac),
                sock_to_cstr( sockbuf, peer ) );

	memcpy(tmp.mac_addr, mac, sizeof(n2n_mac_t));

	scan = sglib_hashed_peer_info_t_find_member(eee->pending_peers, &tmp);

    if(scan) {
        /* Remove scan from pending_peers. */
		sglib_hashed_peer_info_t_delete(eee->pending_peers, scan);
        
        /* Add scan to known_peers. */
		sglib_hashed_peer_info_t_add(eee->known_peers, scan);

        
        scan->sock = *peer;

        LogPrint( "edge", "=== new peer %s -> %s\n",
                    macaddr_str( mac_buf, scan->mac_addr),
                    sock_to_cstr( sockbuf, &scan->sock ) );

        LogPrint( "edge", "Pending peers list size=%u\n",
                    (unsigned int)hashed_peer_list_t_size( eee->pending_peers ) );

        LogPrint( "edge", "Operational peers list size=%u\n",
                    (unsigned int)hashed_peer_list_t_size( eee->known_peers ) );


        scan->last_seen = time(NULL);
    }
    else
    {
        //traceEvent( TRACE_DEBUG, "Failed to find sender in pending_peers." );
    }
}


n2n_mac_t broadcast_mac = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static int is_empty_ip_address( const n2n_sock_t * sock )
{
    const uint8_t * ptr=NULL;
    size_t len=0;
    size_t i;

    if ( AF_INET6 == sock->family )
    {
        ptr = sock->addr.v6;
        len = 16;
    }
    else
    {
        ptr = sock->addr.v4;
        len = 4;
    }

    for (i=0; i<len; ++i)
    {
        if ( 0 != ptr[i] )
        {
            /* found a non-zero byte in address */
            return 0;
        }
    }

    return 1;
}


/** Keep the known_peers list straight.
 *
 *  Ignore broadcast L2 packets, and packets with invalid public_ip.
 *  If the dst_mac is in known_peers make sure the entry is correct:
 *  - if the public_ip socket has changed, erase the entry
 *  - if the same, update its last_seen = when
 */
static void update_peer_address(n2n_edge_t * eee,
                                uint8_t from_supernode,
                                const n2n_mac_t mac,
                                const n2n_sock_t * peer,
                                time_t when)
{
    peer_info_t *scan = NULL;
    peer_info_t tmp;
    n2n_sock_str_t sockbuf1;
    n2n_sock_str_t sockbuf2; /* don't clobber sockbuf1 if writing two addresses to trace */
    macstr_t mac_buf;

    if ( is_empty_ip_address( peer ) )
    {
        /* Not to be registered. */
        return;
    }

    if ( 0 == memcmp( mac, broadcast_mac, N2N_MAC_SIZE ) )
    {
        /* Not to be registered. */
        return;
    }

    memcpy(tmp.mac_addr, mac, sizeof(n2n_mac_t));
    scan = sglib_hashed_peer_info_t_find_member(eee->known_peers, &tmp);

    if ( scan == NULL )
    {
        /* Not in known_peers. */
        return;
    }

    if ( 0 != sock_equal( &(scan->sock), peer))
    {
        if ( 0 == from_supernode )
        {
            LogPrint( "edge", "Peer changed %s: %s -> %s",
                        macaddr_str( mac_buf, scan->mac_addr ),
                        sock_to_cstr(sockbuf1, &(scan->sock)),
                        sock_to_cstr(sockbuf2, peer) );

            /* The peer has changed public socket. It can no longer be assumed to be reachable. */
            /* Remove the peer. */
			sglib_hashed_peer_info_t_delete(eee->known_peers, scan);
            dealloc_peer(scan);

            establish_connection( eee, mac );
        }
        else
        {
            /* Don't worry about what the supernode reports, it could be seeing
             * a different socket. */
            /* anyways, the packet came from supernode. That is potential
             * trouble... just in case we will send a register back to the 
             * origin */
            send_register( eee, &(scan->sock), scan->mac_addr );
        }
    }
    else
    {
        /* Found and unchanged. */
        if(!from_supernode)
            scan->last_seen = when;
    }
}

#if defined(DUMMY_ID_00001) /* Disabled waiting for config option to enable it */



static char gratuitous_arp[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* Dest mac */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Src mac */
  0x08, 0x06, /* ARP */
  0x00, 0x01, /* Ethernet */
  0x08, 0x00, /* IP */
  0x06, /* Hw Size */
  0x04, /* Protocol Size */
  0x00, 0x01, /* ARP Request */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Src mac */
  0x00, 0x00, 0x00, 0x00, /* Src IP */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Target mac */
  0x00, 0x00, 0x00, 0x00 /* Target IP */
};


/** Build a gratuitous ARP packet for a /24 layer 3 (IP) network. */
static int build_gratuitous_arp(char *buffer, uint16_t buffer_len) {
  if(buffer_len < sizeof(gratuitous_arp)) return(-1);

  memcpy(buffer, gratuitous_arp, sizeof(gratuitous_arp));
  memcpy(&buffer[6], device.mac_addr, 6);
  memcpy(&buffer[22], device.mac_addr, 6);
  memcpy(&buffer[28], &device.ip_addr, 4);

  /* REVISIT: BbMaj7 - use a real netmask here. This is valid only by accident
   * for /24 IPv4 networks. */
  buffer[31] = 0xFF; /* Use a faked broadcast address */
  memcpy(&buffer[38], &device.ip_addr, 4);
  return(sizeof(gratuitous_arp));
}

/** Called from update_supernode_reg to periodically send gratuitous ARP
 *  broadcasts. */
static void send_grat_arps(n2n_edge_t * eee,) {
  char buffer[48];
  size_t len;

  //traceEvent(TRACE_NORMAL, "Sending gratuitous ARP...");
  len = build_gratuitous_arp(buffer, sizeof(buffer));
  send_packet2net(eee, buffer, len);
  send_packet2net(eee, buffer, len); /* Two is better than one :-) */
}
#endif /* #if defined(DUMMY_ID_00001) */




/** @brief Check to see if we should re-register with the supernode.
 *
 *  This is frequently called by the main loop.
 */
static void update_supernode_reg( n2n_edge_t * eee, time_t nowTime )
{
    if ( eee->sn_wait && ( nowTime > (eee->last_register_req + (eee->holepunch_interval/10) ) ) )
    {
        /* fall through */
        //traceEvent( TRACE_DEBUG, "update_supernode_reg: doing fast retry." );
    }
    else if ( nowTime < (eee->last_register_req + eee->holepunch_interval))
    {
        return; /* Too early */
    }

    if ( 0 == eee->sup_attempts )
    {
        /* Give up on that supernode and try the next one. */
        ++(eee->sn_idx);

        if (eee->sn_idx >= eee->sn_num)
        {
            /* Got to end of list, go back to the start. Also works for list of one entry. */
            eee->sn_idx=0;
        }

        LogPrintf("update_supernode_reg::Supernode not responding - moving to %u of %u\n", 
                   (unsigned int)eee->sn_idx, (unsigned int)eee->sn_num );

        eee->sup_attempts = N2N_EDGE_SUP_ATTEMPTS;
    }
    else
    {
        --(eee->sup_attempts);
    }

    if(eee->re_resolve_supernode_ip || (eee->sn_num > 1) )
    {
        supernode2addr(&(eee->supernode), eee->sn_ip_array[eee->sn_idx] );
    }

    LogPrint("edge", "update_supernode_reg::Registering with supernode (%s) (attempts left %u)\n",
               supernode_ip(eee), (unsigned int)eee->sup_attempts);

    send_register_super( eee, &(eee->supernode) );

    eee->sn_wait=1;

    /* REVISIT: turn-on gratuitous ARP with config option. */
    /* send_grat_arps(sock_fd, is_udp_sock); */

    eee->last_register_req = nowTime;
}


#define RETRY_INTERVAL 5


/* @return 1 if destination is a peer, 0 if destination is supernode */
static int find_peer_destination(n2n_edge_t * eee,
                                 n2n_mac_t mac_address,
                                 n2n_sock_t * destination)
{
	peer_info_t tmp;
	peer_info_t* scan = NULL;
	peer_info_t* tryscan = NULL;
    macstr_t mac_buf;
    n2n_sock_str_t sockbuf;
    int retval=0;
    int i;
	time_t now = time(NULL);

    LogPrint("edge", "find_peer_destination::Searching destination peer for MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
               mac_address[0] & 0xFF, mac_address[1] & 0xFF, mac_address[2] & 0xFF,
               mac_address[3] & 0xFF, mac_address[4] & 0xFF, mac_address[5] & 0xFF);

	memcpy(tmp.mac_addr, mac_address, sizeof(n2n_mac_t));
	scan = sglib_hashed_peer_info_t_find_member(eee->known_peers, &tmp);
	if(scan) {
        if(now-scan->last_seen > scan->timeout) {
            /* delete the peer and establish new connection */
			sglib_hashed_peer_info_t_delete(eee->known_peers, scan);
            establish_connection( eee, scan->mac_addr );
            dealloc_peer(scan);
        } else if(scan->last_seen > 0) {
			memcpy(destination, &scan->sock, sizeof(n2n_sock_t));
			retval = 1;
		}
	}
    
    if ( 0 == retval )
    {
		tryscan = sglib_hashed_peer_info_t_find_member(eee->pending_peers, &tmp);
		if(tryscan) {
            if(tryscan->num_sockets == 0) {
                /* not yet received peer_info from supernode */
                if(now - tryscan->last_sent_query > RETRY_INTERVAL) {
                    send_query_peer(eee, tryscan->mac_addr);
                    tryscan->last_sent_query = now;
                }
            } else if(now - tryscan->last_seen > RETRY_INTERVAL) {
				LogPrint("edge", "find_peer_destination::retrying to register peer (%s) -> [%s]\n",
					macaddr_str( mac_buf, mac_address),
					sock_to_cstr( sockbuf, &tryscan->sock));
                for(i=0; i<tryscan->num_sockets; i++)
                    send_register(eee, &tryscan->sockets[i], tryscan->mac_addr);
				tryscan->last_seen = now;
			}
		}
        memcpy(destination, &(eee->supernode), sizeof(struct sockaddr_in));
    }

    LogPrint("edge", "find_peer_destination::find_peer_address (%s) -> [%s]\n",
               macaddr_str( mac_buf, mac_address ),
               sock_to_cstr( sockbuf, destination ) );

    return retval;
}




/* *********************************************** */

static const struct option long_options[] = {
  { "community",          required_argument, NULL, 'c' },
  { "supernode-list",     required_argument, NULL, 'l' },
  { "tun-device",         required_argument, NULL, 'd' },
  { "euid",               required_argument, NULL, 'u' },
  { "egid",               required_argument, NULL, 'g' },
  { "local-ip",           required_argument, NULL, 'L' },
  { "holepunch-interval", required_argument, NULL, 'i' },
  { "help"   ,            no_argument,       NULL, 'h' },
  { "verbose",            no_argument,       NULL, 'v' },
#ifdef WIN32
  { "remove",             no_argument,       NULL, 'R' },
#endif
  { NULL,                 0,                 NULL,  0  }
};

/* ***************************************************** */


/** Send an ecapsulated ethernet PACKET to a destination edge or broadcast MAC
 *  address. */
static int send_PACKET( n2n_edge_t * eee,
                        n2n_mac_t dstMac,
                        const uint8_t * pktbuf,
                        size_t pktlen,
                        n2n_sock_t * destination)
{
    n2n_sock_str_t sockbuf;

    /* hexdump( pktbuf, pktlen ); */


    LogPrint( "edge", "send_PACKET::send_PACKET to %s\n", sock_to_cstr( sockbuf, destination ) );

    sendto_sock( eee->udp_sock, pktbuf, pktlen, destination );
    return 0;
}


/* Choose the transop for Tx. This should be based on the newest valid
 * cipherspec in the key schedule. 
 *
 * Never fall back to NULL tranform unless no key sources were specified. It is
 * better to render edge inoperative than to expose user data in the clear. In
 * the case where all SAs are expired an arbitrary transform will be chosen for
 * Tx. It will fail having no valid SAs but one must be selected.
 */
static size_t edge_choose_tx_transop( const n2n_edge_t * eee )
{
    if ( eee->null_transop)
    {
        return N2N_TRANSOP_NULL_IDX;
    }

    return eee->tx_transop_idx;
}


/** A layer-2 packet was received at the tunnel and needs to be sent via UDP. */
static void send_packet2net(n2n_edge_t * eee,
                            uint8_t *tap_pkt, size_t len)
{
    ipstr_t ip_buf;
    n2n_mac_t destMac;

    n2n_common_t cmn;
    n2n_PACKET_t pkt;

    uint8_t pktbuf[N2N_PKT_BUF_SIZE];
    size_t idx=0;
    size_t tx_transop_idx=0;

    ether_hdr_t eh;

    int dest;
    int offset;
    n2n_sock_t destination;

    /* tap_pkt is not aligned so we have to copy to aligned memory */
    memcpy( &eh, tap_pkt, sizeof(ether_hdr_t) );

    /* Discard IP packets that are not originated by this hosts */
    if(!(eee->allow_routing)) {
        if(ntohs(eh.type) == 0x0800) {
            /* This is an IP packet from the local source address - not forwarded. */
            uint32_t *src = (uint32_t*)&tap_pkt[ETH_FRAMEHDRSIZE + IP4_SRCOFFSET];

            /* Note: all elements of the_ip are in network order */
            if( *src != eee->device.ip_addr) {
		/* This is a packet that needs to be routed */
		LogPrint("edge", "send_packet2net::Discarding routed packet [%s]\n",
                           intoa(ntohl(*src), ip_buf, sizeof(ip_buf)));
		return;
            } else {
                /* This packet is originated by us */
                /* traceEvent(TRACE_INFO, "Sending non-routed packet"); */
            }
        }
    }

    /* Optionally compress then apply transforms, eg encryption. */

    /* Once processed, send to destination in PACKET */

    memcpy( destMac, tap_pkt, N2N_MAC_SIZE ); /* dest MAC is first in ethernet header */

    memset( &cmn, 0, sizeof(cmn) );
    cmn.ttl = N2N_DEFAULT_TTL;
    cmn.pc = n2n_packet;
    cmn.flags=0; /* no options, not from supernode, no socket */
    memcpy( cmn.community, eee->community_name, N2N_COMMUNITY_SIZE );

    dest = find_peer_destination(eee, destMac, &destination);

    memset( &pkt, 0, sizeof(pkt) );

    tx_transop_idx = edge_choose_tx_transop( eee );

    pkt.transform = eee->transop[tx_transop_idx].transform_id;

    idx=0;
    encode_PACKET( pktbuf, &idx, &cmn, &pkt );
    /* make the ethernet-header part of the PACKET header */
    offset = copy_ETHFRAMEHDR( pktbuf+idx , tap_pkt);
    idx += offset;
    tap_pkt += offset;
    len -= offset;

    LogPrint("edge", "send_packet2net::encoded PACKET header of size=%u transform %u (idx=%u)\n", 
                (unsigned int)idx, (unsigned int)pkt.transform, (unsigned int)tx_transop_idx );

    idx += eee->transop[tx_transop_idx].fwd( &(eee->transop[tx_transop_idx]),
                                             pktbuf+idx, N2N_PKT_BUF_SIZE-idx,
                                             tap_pkt, len );
    ++(eee->transop[tx_transop_idx].tx_cnt); /* stats */

    if ( dest )
    {
        ++(eee->tx_p2p);
		eee->tx_bit_p2p += idx;
    }
    else
    {
        ++(eee->tx_sup);
		eee->tx_bit_sup += idx;
    }
    send_PACKET( eee, destMac, pktbuf, idx, &destination);
}


/** Destination MAC 33:33:0:00:00:00 - 33:33:FF:FF:FF:FF is reserved for IPv6
 *  neighbour discovery.
 */
static int is_ip6_discovery( const void * buf, size_t bufsize )
{
    int retval = 0;

    if ( bufsize >= sizeof(ether_hdr_t) )
    {
        /* copy to aligned memory */
        ether_hdr_t eh;
        memcpy( &eh, buf, sizeof(ether_hdr_t) );

        if ( (0x33 == eh.dhost[0]) &&
             (0x33 == eh.dhost[1]) )
        {
            retval = 1; /* This is an IPv6 multicast packet [RFC2464]. */
        }
    }
    return retval;
}

/** Destination 01:00:5E:00:00:00 - 01:00:5E:7F:FF:FF is multicast ethernet.
 */
static int is_ethMulticast( const void * buf, size_t bufsize )
{
    int retval = 0;

    /* Match 01:00:5E:00:00:00 - 01:00:5E:7F:FF:FF */
    if ( bufsize >= sizeof(ether_hdr_t) )
    {
        /* copy to aligned memory */
        ether_hdr_t eh;
        memcpy( &eh, buf, sizeof(ether_hdr_t) );

        if ( (0x01 == eh.dhost[0]) &&
             (0x00 == eh.dhost[1]) &&
             (0x5E == eh.dhost[2]) &&
             (0 == (0x80 & eh.dhost[3])) )
        {
            retval = 1; /* This is an ethernet multicast packet [RFC1112]. */
        }
    }
    return retval;
}



/** Read a single packet from the TAP interface, process it and write out the
 *  corresponding packet to the cooked socket.
 */
static void readFromTAPSocket( n2n_edge_t * eee )
{
    /* tun -> remote */
    uint8_t             eth_pkt[N2N_PKT_BUF_SIZE];
    macstr_t            mac_buf;
    ssize_t             len;

    len = tuntap_read( &(eee->device), eth_pkt, N2N_PKT_BUF_SIZE );

    if( (len <= 0) || (len > N2N_PKT_BUF_SIZE) )
    {
        LogPrint("edge", "readFromTAPSocket::read()=%d [%d/%s]\n",
                   (signed int)len, errno, strerror(errno));
    }
    else
    {
        const uint8_t * mac = eth_pkt;
        LogPrint("edge", "readFromTAPSocket::### Rx TAP packet (%4d) for %s\n",
                   (signed int)len, macaddr_str(mac_buf, mac) );

        if ( eee->drop_multicast &&
             ( is_ip6_discovery( eth_pkt, len ) ||
               is_ethMulticast( eth_pkt, len)
                 )
            )
        {
            LogPrint("edge", "readFromTAPSocket::Dropping multicast\n");
        }
        else
        {
            send_packet2net(eee, eth_pkt, len);
        }
    }
}



/** A PACKET has arrived containing an encapsulated ethernet datagram - usually
 *  encrypted. */
static int handle_PACKET( n2n_edge_t * eee,
                          const n2n_common_t * cmn,
                          const n2n_PACKET_t * pkt,
                          const n2n_sock_t * orig_sender,
                          uint8_t * payload,
                          size_t psize )
{
    ssize_t             data_sent_len;
    uint8_t             from_supernode;
    uint8_t *           eth_payload=NULL;
    int                 retval = -1;
    time_t              now;
    n2n_ETHFRAMEHDR_t   eth;

    now = time(NULL);

    LogPrint( "edge", "handle_PACKET::handle_PACKET size %u transform %u\n", 
                (unsigned int)psize, (unsigned int)pkt->transform );
    /* hexdump( payload, psize ); */

    from_supernode= cmn->flags & N2N_FLAGS_FROM_SUPERNODE;

    if ( from_supernode )
    {
        ++(eee->rx_sup);
        eee->last_sup=now;
		eee->rx_bit_sup += psize;
    }
    else
    {
        ++(eee->rx_p2p);
        eee->last_p2p=now;
		eee->rx_bit_p2p += psize;
    }

    decode_ETHFRAMEHDR(&eth, payload);
    /* Update the sender in peer table entry */
    check_peer( eee, from_supernode, eth.srcMac, orig_sender);

    /* Handle transform. */
    {
        uint8_t decodebuffer[N2N_PKT_BUF_SIZE];
        uint8_t * decodebuf = decodebuffer;
        size_t eth_size;
        size_t rx_transop_idx=0;
        int offset;

        /* copy eth header to decodebuf */
        offset = copy_ETHFRAMEHDR(decodebuffer, payload);
        decodebuf += offset;
        payload += offset;
        psize -= offset;
        eth_size = offset;

        rx_transop_idx = transop_enum_to_index(pkt->transform);

        if ( rx_transop_idx >=0 )
        {
            eth_payload = decodebuf;
            eth_size += eee->transop[rx_transop_idx].rev( &(eee->transop[rx_transop_idx]),
                                                         eth_payload, N2N_PKT_BUF_SIZE,
                                                         payload, psize );
            ++(eee->transop[rx_transop_idx].rx_cnt); /* stats */

            /* Write ethernet packet to tap device. */
            LogPrint( "edge", "handle_PACKET::sending to TAP %u\n", (unsigned int)eth_size );
            data_sent_len = tuntap_write(&(eee->device), decodebuffer, eth_size);

            if (data_sent_len == eth_size)
            {
                retval = 0;
            }
        }
        else
        {
            LogPrintf("handle_PACKET::handle_PACKET dropped unknown transform enum %u\n", 
                        (unsigned int)pkt->transform );
        }
    }

    return retval;
}


/** Read a datagram from the management UDP socket and take appropriate
 *  action. */
static void readFromMgmtSocket( n2n_edge_t * eee, int * keep_running )
{
    uint8_t             udp_buf[N2N_PKT_BUF_SIZE+1];   /* Complete UDP packet */
    ssize_t             recvlen;
    struct sockaddr_in  sender_sock;
    socklen_t           i;
    size_t              msg_len;
    time_t              now;
    macstr_t		mac_buf;
    n2n_sock_str_t	sockbuf;
    peer_info_t *	lpi = NULL;
    struct sglib_hashed_peer_info_t_iterator    it;
    int			c;

    now = time(NULL);
    i = sizeof(sender_sock);
    recvlen = recvfrom(eee->udp_mgmt_sock, udp_buf, N2N_PKT_BUF_SIZE, 0/*flags*/,
		     (struct sockaddr *)&sender_sock, (socklen_t*)&i);

    if ( recvlen < 0 )
    {
        LogPrintf("readFromMgmtSocket::mgmt recvfrom failed with %s\n", strerror(errno) );

        return; /* failed to receive data from UDP */
    }

    /* avoid parsing any uninitialized junk from the stack */
    udp_buf[recvlen] = 0;

    if ( recvlen >= 4 )
    {
        if ( 0 == memcmp( udp_buf, "stop", 4 ) )
        {
            LogPrintf("readFromMgmtSocket::stop command received.\n" );
            *keep_running = 0;
            return;
        }

        if ( 0 == memcmp( udp_buf, "help", 4 ) )
        {
            msg_len=0;
            //++traceLevel;

            msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                                 "Help for edge management console:\n"
                                 "  stop    Gracefully exit edge\n"
                                 "  help    This help message\n"
                                 "  +verb   Increase verbosity of logging\n"
                                 "  -verb   Decrease verbosity of logging\n"
								 "  peers   List table of known peers\n"
                                 "  reload  Re-read the keyschedule\n"
                                 "  mangle_peer mac ip [port] mangle peer\n"
                                 "  localip [value]     Show or set the local IP value\n"
                                 "  <enter> Display statistics\n\n");

            sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0/*flags*/,
                    (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );

            return;
        }

    }

    if ( recvlen >= 5 )
    {
        if ( 0 == memcmp( udp_buf, "+verb", 5 ) )
        {
            msg_len=0;
            ++traceLevel;

            sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0/*flags*/,
                    (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );

            return;
        }

        if ( 0 == memcmp( udp_buf, "-verb", 5 ) )
        {
            msg_len=0;

            if ( traceLevel > 0 )
            {
                --traceLevel;
                msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                                     "> -OK traceLevel=%u\n", traceLevel );
            }
            else
            {
                msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                                     "> -NOK traceLevel=%u\n", traceLevel );
            }


            sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0/*flags*/,
                    (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
            return;
        }

		if ( 0 == memcmp( udp_buf, "peers", 5 ) )
		{
			msg_len = 0;
			
			msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
								 "> known peers (connected edges):\n");

			// traceEvent( TRACE_ERROR, "listing peers on mgmt socket", 0);
			sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0,
				(struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );

			c = 0;
			for(lpi=sglib_hashed_peer_info_t_it_init(&it,eee->known_peers); lpi!=NULL; lpi=sglib_hashed_peer_info_t_it_next(&it)) {
				c++;
				msg_len = 0;
				msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                                        ">  %i: %s -> %s last: %li(%ld sec ago)\n", c, macaddr_str( mac_buf, lpi->mac_addr ),
						sock_to_cstr( sockbuf, &(lpi->sock) ),
                                                lpi->last_seen, (now - lpi->last_seen));
				sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0,
					(struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
			}

			msg_len = 0;
			
			msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
								 "> known peers (tried edges):\n");
			sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0,
				(struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );

			c = 0;
			for(lpi=sglib_hashed_peer_info_t_it_init(&it,eee->pending_peers); lpi!=NULL; lpi=sglib_hashed_peer_info_t_it_next(&it)) {
				c++;
				msg_len = 0;
                if(lpi->num_sockets == 0)
                    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                        ">  %i: %s -> (no info) last: %li(%ld sec ago)\n", c,
                        macaddr_str( mac_buf, lpi->mac_addr ), 
                        lpi->last_seen, (now - lpi->last_seen));
                else
                    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                        ">  %i: %s -> %s last: %li(%ld sec ago)\n", c, macaddr_str( mac_buf, lpi->mac_addr ),
                            sock_to_cstr( sockbuf, lpi->sockets ),
                            lpi->last_seen, (now - lpi->last_seen));
                if(lpi->num_sockets > 1)
                    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                        "                           %s\n",
                            sock_to_cstr( sockbuf, lpi->sockets+1 ));
				sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0,
					(struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
			}

			return;
		}
    }

    if ( recvlen >= 6 )
    {
        if ( 0 == memcmp( udp_buf, "reload", 6 ) )
        {
            if ( strlen( eee->keyschedule ) > 0 )
            {
                // if ( edge_init_keyschedule(eee) == 0 )
                // {
                //     msg_len=0;
                //     msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                //                          "> OK\n" );
                //     sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0/*flags*/,
                //             (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
                // }
                return;
            }
        }
    }

    if ( recvlen >= 12 )
    {
        if ( 0 == memcmp (udp_buf, "mangle_peer ", 12) )
        {
            int j, n_matched;
            n2n_mac_t target_mac;
            unsigned int mac[6];
            int ip[4];
            int port;
            struct peer_info * scan;

            udp_buf[recvlen] = 0;
            n_matched = sscanf((const char *) udp_buf,
                    "mangle_peer %x:%x:%x:%x:%x:%x %d.%d.%d.%d %d", mac, mac+1,
                    mac+2, mac+3, mac+4, mac+5, ip, ip+1, ip+2, ip+3, &port);
            for( j=0; j<6; j++ )
                target_mac[j] = (uint8_t) mac[j];
            scan = find_peer_by_mac( eee->pending_peers, target_mac );
            if (NULL != scan && n_matched >= 10) {
                scan->sock.family = AF_INET;
                printf("n_matched: %d, port: %d\n", n_matched, port);
                if (n_matched >= 11 && port > 0)
                    scan->sock.port = (uint16_t) port;
                for( j=0; j<4; j++)
                    scan->sock.addr.v4[j] = (uint8_t) ip[j];
                sendto( eee->udp_mgmt_sock, "success\n", 9, 0,
                        (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
            } else {
                sendto( eee->udp_mgmt_sock, "failure\n", 9, 0,
                        (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
            }
            return;
        }
    }

    char * cmd = strtok( (char *)udp_buf, " \r\n");
    if (cmd) {
        if ( 0 == strcmp( cmd, "localip" ) )
        {
            char *arg = strtok( NULL, " \r\n");
            if (arg) {
                strncpy( eee->local_ip_str, arg, N2N_EDGE_LOCAL_IP_SIZE);
                if ( set_localip(eee) != 0) {
                    sendto( eee->udp_mgmt_sock, "failure\n", 9, 0,
                            (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
                    return;
                }
            }

            /* show current value - possibly after changing it */
            msg_len=0;
            if (!eee->local_sock_ena) {
                msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                                     "> localip off\n" );
            } else {
                n2n_sock_str_t local_sockbuf;
                msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                                     "> localip %s (%s)\n",
                                     eee->local_ip_str,
                                     sock_to_cstr( local_sockbuf, &(eee->local_sock) ) );
            }
            sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0/*flags*/,
                    (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
            return;
        }
    }

    //traceEvent(TRACE_DEBUG, "mgmt status rq" );

    msg_len=0;
    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                         "Statistics for edge\n" );

    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                         "uptime %lu\n",
                         time(NULL) - eee->start_time );

    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
			"paths  super:(tx),(rx) p2p: (tx),(rx)\n");
    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                         "packets      %u,%u     %u,%u\n",
                         (unsigned int)eee->tx_sup,
			 (unsigned int)eee->rx_sup,
			 (unsigned int)eee->tx_p2p,
			 (unsigned int)eee->rx_p2p );

   msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
			 "byte/s       %u,%u     %u,%u\n",
			 (unsigned int)eee->tx_bps_sup,
			 (unsigned int)eee->rx_bps_sup,
			 (unsigned int)eee->tx_bps_p2p,
			 (unsigned int)eee->rx_bps_p2p );

    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                         "trans:null |%6u|%6u|\n"
                         "trans:tf   |%6u|%6u|\n"
                         "trans:aes  |%6u|%6u|\n",
                         (unsigned int)eee->transop[N2N_TRANSOP_NULL_IDX].tx_cnt,
                         (unsigned int)eee->transop[N2N_TRANSOP_NULL_IDX].rx_cnt,
                         (unsigned int)eee->transop[N2N_TRANSOP_TF_IDX].tx_cnt,
                         (unsigned int)eee->transop[N2N_TRANSOP_TF_IDX].rx_cnt,
                         (unsigned int)eee->transop[N2N_TRANSOP_AESCBC_IDX].tx_cnt,
                         (unsigned int)eee->transop[N2N_TRANSOP_AESCBC_IDX].rx_cnt );

    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                         "peers  pend:%u full:%u\n",
                         (unsigned int)hashed_peer_list_t_size( eee->pending_peers ), 
			 (unsigned int)hashed_peer_list_t_size( eee->known_peers ) );

    msg_len += snprintf( (char *)(udp_buf+msg_len), (N2N_PKT_BUF_SIZE-msg_len),
                         "last   super:%lu(%ld sec ago) p2p:%lu(%ld sec ago)\n",
                         eee->last_sup, (now - eee->last_sup), eee->last_p2p, (now - eee->last_p2p) );

    //traceEvent(TRACE_DEBUG, "mgmt status sending: %s", udp_buf );


    sendto( eee->udp_mgmt_sock, udp_buf, msg_len, 0/*flags*/,
            (struct sockaddr *)&sender_sock, sizeof(struct sockaddr_in) );
}


/** Read a datagram from the main UDP socket to the internet. */
static void readFromIPSocket( n2n_edge_t * eee )
{
    n2n_common_t        cmn; /* common fields in the packet header */

    n2n_sock_str_t      sockbuf1;
    n2n_sock_str_t      sockbuf2; /* don't clobber sockbuf1 if writing two addresses to trace */
    macstr_t            mac_buf1;
    macstr_t            mac_buf2;

    uint8_t             udp_buf[N2N_PKT_BUF_SIZE];      /* Compete UDP packet */
    ssize_t             recvlen;
    size_t              rem;
    size_t              idx;
    size_t              msg_type;
    struct sockaddr_in  sender_sock;
    n2n_sock_t          sender;
    n2n_sock_t *        orig_sender=NULL;
    time_t              now=0;
    size_t              i;
    int                 j;

    /* for PACKET packages */
    n2n_PACKET_t pkt;

    /* for PEER_INFO packages */
    n2n_PEER_INFO_t pi;
    struct peer_info *  scan;

    /* for REGISTER packages */
    n2n_REGISTER_t reg;
    peer_info_t tmp;

    /* for REGISTER_ACK packages */
    n2n_REGISTER_ACK_t ra;

    /* For REGISTER_SUPER_ACK packages */
    n2n_REGISTER_SUPER_ACK_t rsa;


    i = sizeof(sender_sock);
    recvlen = recvfrom(eee->udp_sock, udp_buf, N2N_PKT_BUF_SIZE, 0/*flags*/,
                     (struct sockaddr *)&sender_sock, (socklen_t*)&i);

    if ( recvlen < 0 )
    {
        LogPrintf("readFromIPSocket::recvfrom failed with %s\n", strerror(errno) );

        return; /* failed to receive data from UDP */
    }

    /* REVISIT: when UDP/IPv6 is supported we will need a flag to indicate which
     * IP transport version the packet arrived on. May need to UDP sockets. */
    sender.family = AF_INET; /* udp_sock was opened PF_INET v4 */
    sender.port = ntohs(sender_sock.sin_port);
    memcpy( &(sender.addr.v4), &(sender_sock.sin_addr.s_addr), IPV4_SIZE );

    /* The packet may not have an orig_sender socket spec. So default to last
     * hop as sender. */
    orig_sender=&sender;

    //traceEvent(TRACE_INFO, "### Rx N2N UDP (%d) from %s", 
            //    (signed int)recvlen, sock_to_cstr(sockbuf1, &sender) );

    /* hexdump( udp_buf, recvlen ); */

    rem = recvlen; /* Counts down bytes of packet to protect against buffer overruns. */
    idx = 0; /* marches through packet header as parts are decoded. */
    if ( decode_common(&cmn, udp_buf, &rem, &idx) < 0 )
    {
        LogPrintf("Failed to decode common section in N2N_UDP\n" );
        return; /* failed to decode packet */
    }

    now = time(NULL);

    msg_type = cmn.pc; /* packet code */

    if( 0 == memcmp(cmn.community, eee->community_name, N2N_COMMUNITY_SIZE) )
    {
        switch(msg_type) {
        case MSG_TYPE_PACKET:
            /* process PACKET */
            decode_PACKET( &pkt, &cmn, udp_buf, &rem, &idx );

            LogPrint("edge","Rx PACKET from %s (%s)\n",
                       sock_to_cstr(sockbuf1, &sender),
                       sock_to_cstr(sockbuf2, orig_sender) );

            handle_PACKET( eee, &cmn, &pkt, orig_sender, udp_buf+idx,
                    recvlen-idx );
            break;
        case MSG_TYPE_PEER_INFO:
            decode_PEER_INFO( &pi, &cmn, udp_buf, &rem, &idx );

            scan = find_peer_by_mac( eee->pending_peers, pi.mac );
            if (scan) {
                scan->timeout = pi.timeout;
                if (scan->num_sockets > 0)
                    free(scan->sockets);
                if (pi.aflags & N2N_AFLAGS_LOCAL_SOCKET)
                    scan->num_sockets = 2;
                else
                    scan->num_sockets = 1;
                scan->sockets = (n2n_sock_t *)malloc(scan->num_sockets*sizeof(n2n_sock_t));
                for(j=0; j<scan->num_sockets; j++)
                    scan->sockets[j] = pi.sockets[j];
                LogPrint("edge", "Rx PEER_INFO on %s\n",
                           macaddr_str(mac_buf1, pi.mac) );
                for(j=0; j<scan->num_sockets; j++)
                    send_register(eee, scan->sockets+j, scan->mac_addr);
            } else {
                LogPrint("edge", "Rx PEER_INFO unknown peer %s\n",
                           macaddr_str(mac_buf1, pi.mac) );
            }

            break;
        case MSG_TYPE_REGISTER:
            /* Another edge is registering with us */
            decode_REGISTER( &reg, &cmn, udp_buf, &rem, &idx );

            LogPrint("edge",
                    "Rx REGISTER src=%s dst=%s from peer %s (%s)\n",
                    macaddr_str( mac_buf1, reg.srcMac ),
                    macaddr_str( mac_buf2, reg.dstMac ),
                    sock_to_cstr(sockbuf1, &sender),
                    sock_to_cstr(sockbuf2, orig_sender) );

            if ( 0 == memcmp(reg.dstMac, (eee->device.mac_addr), 6) )
            {
                memcpy(tmp.mac_addr, reg.srcMac, sizeof(n2n_mac_t));
                if (sglib_hashed_peer_info_t_find_member( 
                            eee->pending_peers, &tmp) != NULL)
                    send_register(eee, orig_sender, NULL);
            }

            send_register_ack(eee, orig_sender, &reg);
            break;
        case MSG_TYPE_REGISTER_ACK:
            /* Peer edge is acknowledging our register request */

            decode_REGISTER_ACK( &ra, &cmn, udp_buf, &rem, &idx );

            if ( ra.sock.family )
            {
                orig_sender = &(ra.sock);
            }

            LogPrint("edge", "Rx REGISTER_ACK src=%s dst=%s from peer %s (%s)\n",
                       macaddr_str( mac_buf1, ra.srcMac ),
                       macaddr_str( mac_buf2, ra.dstMac ),
                       sock_to_cstr(sockbuf1, &sender),
                       sock_to_cstr(sockbuf2, orig_sender) );

            /* Move from pending_peers to known_peers; ignore if not in pending. */
            set_peer_operational( eee, ra.srcMac, &sender );
            break;
        case MSG_TYPE_REGISTER_SUPER_ACK:
            if ( eee->sn_wait )
            {
                decode_REGISTER_SUPER_ACK( &rsa, &cmn, udp_buf, &rem, &idx );

                if ( rsa.sock.family )
                {
                    orig_sender = &(rsa.sock);
                }

                LogPrint("edge", "Rx REGISTER_SUPER_ACK myMAC=%s [%s] (external %s). Attempts %u\n",
                           macaddr_str( mac_buf1, rsa.edgeMac ),
                           sock_to_cstr(sockbuf1, &sender),
                           sock_to_cstr(sockbuf2, orig_sender), 
                           (unsigned int)eee->sup_attempts );

                if ( 0 == memcmp( rsa.cookie, eee->last_cookie, N2N_COOKIE_SIZE ) )
                {
                    if ( rsa.num_sn > 0 )
                    {
                        LogPrint("edge", "Rx REGISTER_SUPER_ACK backup supernode at %s\n",
                                   sock_to_cstr(sockbuf1, &(rsa.sn_bak) ) );
                    }

                    eee->last_p2p = now;
                    eee->last_sup = now;
                    eee->sn_wait=0;
                    eee->sup_attempts = N2N_EDGE_SUP_ATTEMPTS; /* refresh because we got a response */

                    /* REVISIT: store sn_back */
                    /* don't adjust lifetime according to supernode - this value should be specified
                     * by the client (because dependent on NAT/firewall) (lukas) 
                    eee->holepunch_interval = rsa.lifetime;
                    eee->holepunch_interval = MAX( eee->holepunch_interval, REGISTER_SUPER_INTERVAL_ );
                    eee->holepunch_interval = ( eee->holepunch_interval, REGISTER_SUPER_INTERVAL_MAX );
                     */
                }
                else
                {
                    LogPrintf("Rx REGISTER_SUPER_ACK with wrong or old cookie.\n" );
                }
            }
            else
            {
                LogPrintf("Rx REGISTER_SUPER_ACK with no outstanding REGISTER_SUPER.\n" );
            }
            break;
        default:
            /* Not a known message type */
            LogPrintf("Unable to handle packet type %d: ignored\n", (signed int)msg_type);
            break;
        } /* end switch(msg_type) */
    } /* if (community match) */
    else
    {
        LogPrintf("Received packet with invalid community\n");
    }

}

/* ***************************************************** */

boost::thread threadTun;
#ifdef WIN32
static void tunReadThread(n2n_edge_t* eee )
{
    try{

        while(1)
        {
            readFromTAPSocket(eee);
        
            boost::this_thread::interruption_point();
        } /* while */
    }catch(boost::thread_interrupted&){
        LogPrintf("dockeredge::tunReadThread exit\n");
    }
}


/** Start a second thread in Windows because TUNTAP interfaces do not expose
 *  file descriptors. */
static void startTunReadThread(n2n_edge_t *eee)
{
    // HANDLE hThread;
    // DWORD dwThreadId;
    
    // hThread = CreateThread(NULL,         /* security attributes */
    //                        0,            /* use default stack size */
    //                        (LPTHREAD_START_ROUTINE)tunReadThread, /* thread function */
    //                        (void*)eee,   /* argument to thread function */
    //                        0,            /* thread creation flags */
    //                        &dwThreadId); /* thread id out */
    // CloseHandle(hThread);

    LogPrintf("dockeredge::tunReadThread started\n");
    threadTun = boost::thread(boost::bind(&tunReadThread,eee));
}
#endif

/* ***************************************************** */

/** Resolve the supernode IP address.
 *
 *  REVISIT: This is a really bad idea. The edge will block completely while the
 *           hostname resolution is performed. This could take 15 seconds.
 */
static void supernode2addr(n2n_sock_t * sn, const n2n_sn_name_t addrIn)
{
    n2n_sn_name_t addr;
	const char *supernode_host;

    memcpy( addr, addrIn, N2N_EDGE_SN_HOST_SIZE );

    supernode_host = strtok(addr, ":");

    sn->port = N2N_SN_PROT;
    if(supernode_host)
    {
        in_addr_t sn_addr;
        char *supernode_port = strtok(NULL, ":");
        const struct addrinfo aihints = {0, PF_INET, 0, 0, 0, NULL, NULL, NULL};
        struct addrinfo * ainfo = NULL;
        int nameerr;

        if ( supernode_port )
            sn->port = atoi(supernode_port);
        else
            LogPrintf("Bad supernode parameter (-l <host:port>) %s %s:\n",
                       addr, supernode_host);

        nameerr = getaddrinfo( supernode_host, NULL, &aihints, &ainfo );

        if( 0 == nameerr )
        {
            struct sockaddr_in * saddr;

            /* ainfo s the head of a linked list if non-NULL. */
            if ( ainfo && (PF_INET == ainfo->ai_family) )
            {
                /* It is definitely and IPv4 address -> sockaddr_in */
                saddr = (struct sockaddr_in *)ainfo->ai_addr;

                memcpy( sn->addr.v4, &(saddr->sin_addr.s_addr), IPV4_SIZE );
                sn->family=AF_INET;
            }
            else
            {
                /* Should only return IPv4 addresses due to aihints. */
                LogPrintf("Failed to resolve supernode IPv4 address for %s\n", supernode_host);
            }

            freeaddrinfo(ainfo); /* free everything allocated by getaddrinfo(). */
            ainfo = NULL;
        } else {
            LogPrintf("Failed to resolve supernode host %s, assug numeric\n", supernode_host);
            sn_addr = inet_addr(supernode_host); /* uint32_t */
            memcpy( sn->addr.v4, &(sn_addr), IPV4_SIZE );
            sn->family=AF_INET;
        }

    } else
        LogPrintf("Wrong supernode parameter (-l <host:port>)\n");
}

/** Decode local address string (given on command line)
 */
static int localip2addr(n2n_sock_t * l_ip,
        const n2n_local_ip_t local_ip_str, int local_port)
{
    /* TODO: IPv6? */
    unsigned int ip[4];
    int i, n_matches;

    n_matches = sscanf( local_ip_str, "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2],
            &ip[3]);

    if( n_matches != 4) {
        return 0;
    }

    for( i=0; i < 4; i++ ) {
        if( ip[i] > 255 ) {
            return 0;
        }
    }
    l_ip->family = AF_INET;
    l_ip->port = local_port;
    for( i = 0; i < 4; i++ )
        l_ip->addr.v4[i] = ip[i];
    return 1;
}

static int set_localip(n2n_edge_t * eee) {
    int     local_port;
    n2n_sock_str_t local_sockbuf;
    struct sockaddr_in sa;

    /* Assume failure, until one of the methods succeeds */
    eee->local_sock_ena = 0;

    if (eee->local_ip_str[0] == 0) {
        /* Sanity check - cannot use an empty string to set localip */
        LogPrintf("localip string is empty\n" );
        return(-1);
    }

    socklen_t sa_len = sizeof(sa);
    if ( getsockname(eee->udp_sock, (struct sockaddr *) &sa, &sa_len) == -1 ) {
        LogPrintf("getsockname() failed\n" );
        return(-1);
    }

    local_port = ntohs(sa.sin_port);
    memset(&(eee->local_sock), 0, sizeof(eee->local_sock));

    if ( localip2addr( &(eee->local_sock),
                    eee->local_ip_str, local_port ) ) {
        eee->local_sock_ena = 1;
    } else if ( 0 == strcmp( eee->local_ip_str, "auto" ) ) {
        /* Automatically use a "sane" value for the localip.
         * This is currently using the local ip address that is used to
         * talk to the supernode, but that could change if a better method
         * if found
         */

        SOCKET fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {
            LogPrintf( "socket() failed\n");
            return(-1);
        }

        struct sockaddr_in sa_sn;

        fill_sockaddr( (struct sockaddr *) &sa_sn,
                       sizeof(sa_sn),
                       &eee->supernode );

        if ( connect(fd, (struct sockaddr *) &sa_sn, sizeof(sa_sn) ) <0 ) {
            close(fd);
            LogPrintf( "connect() failed\n");
            return(-1);
        }

        if ( getsockname(fd, (struct sockaddr *) &sa, &sa_len) <0 ) {
            close(fd);
            LogPrintf("getsockname() failed\n");
            return(-1);
        }

        if ( AF_INET != sa.sin_family ) {
            /* TODO IPV6 */
            close(fd);
            LogPrintf("wrong socket family\n");
            return(-1);
        }

        eee->local_sock.family = sa.sin_family;
        eee->local_sock.port = local_port;
        memcpy( eee->local_sock.addr.v4, &(sa.sin_addr.s_addr), IPV4_SIZE );
        eee->local_sock_ena = 1;
        close(fd);
    }

    if (eee->local_sock_ena) {
        LogPrintf( "Local Socket is %s\n",
                sock_to_cstr( local_sockbuf, &(eee->local_sock) ) );
        return(0);
    }

    LogPrintf("Wrong local_ip parameter (-L ip), disabled\n");
    return(-1);
}

/* ***************************************************** */


/** Find the address and IP mode for the tuntap device.
 *
 *  s is one of these forms:
 *
 *  <host> := <hostname> | A.B.C.D
 *
 *  <host> | static:<host> | dhcp:<host>
 *
 *  If the mode is present (colon required) then fill ip_mode with that value
 *  otherwise do not change ip_mode. Fill ip_mode with everything after the
 *  colon if it is present; or s if colon is not present.
 *
 *  ip_add and ip_mode are NULL terated if modified.
 *
 *  return 0 on success and -1 on error
 */
static int scan_address( char * ip_addr, size_t addr_size,
                         char * ip_mode, size_t mode_size,
                         const char * s )
{
    int retval = -1;
    char * p;

    if ( ( NULL == s ) || ( NULL == ip_addr) )
    {
        return -1;
    }

    memset(ip_addr, 0, addr_size);

    p = (char *)strpbrk(s, ":");

    if ( p )
    {
        /* colon is present */
        if ( ip_mode )
        {
            size_t end=0;

            memset(ip_mode, 0, mode_size);
            end = MIN( p-s, (ssize_t)(mode_size-1) ); /* ensure NULL term */
            strncpy( ip_mode, s, end );
            strncpy( ip_addr, p+1, addr_size-1 ); /* ensure NULL term */
            retval = 0;
        }
    }
    else
    {
        /* colon is not present */
        strncpy( ip_addr, s, addr_size );
    }

    return retval;
}

static int run_loop(n2n_edge_t * eee );
int real_main(int argc, char* argv[]);

#define N2N_NETMASK_STR_SIZE    16 /* dotted decimal 12 numbers + 3 dots */
#define N2N_MACNAMSIZ           18 /* AA:BB:CC:DD:EE:FF + NULL*/
#define N2N_IF_MODE_SIZE        16 /* static | dhcp */

/** Entry point to program from kernel. */
int real_main(int argc, char* argv[])
{
    LogPrint("edge","real_main start\n" );
    int     opt;
    int     local_port = 0 /* any port */;
    int     mgmt_port = N2N_EDGE_MGMT_PORT; /* 5644 by default */
    char    tuntap_dev_name[N2N_IFNAMSIZ] = "edge0";
    char    ip_mode[N2N_IF_MODE_SIZE]="static";
    char    ip_addr[N2N_NETMASK_STR_SIZE] = "";
    char    netmask[N2N_NETMASK_STR_SIZE]="255.255.0.0";
    int     mtu = DEFAULT_MTU;
    int     got_s = 0;

#ifndef WIN32
    uid_t   userid=0; /* root is the only guaranteed ID */
    gid_t   groupid=0; /* root is the only guaranteed ID */
#endif

    char    device_mac[N2N_MACNAMSIZ]="";
    char *  encrypt_key="massgridn2n";

    int     i;

    n2n_edge_t eee; /* single instance for this program */

    if (-1 == edge_init(&eee) )
    {
        LogPrintf("Failed in edge_init\n" );
        exit(1);
    }

    // if( getenv( "N2N_KEY" ))
    // {
    //     encrypt_key = strdup( getenv( "N2N_KEY" ));
    // }

#ifdef WIN32
    tuntap_dev_name[0] = '\0';
#endif
    memset(&(eee.supernode), 0, sizeof(eee.supernode));
    eee.supernode.family = AF_INET;

    scan_address(ip_addr, N2N_NETMASK_STR_SIZE,
                        ip_mode, N2N_IF_MODE_SIZE,
                        strLocalAddr.c_str() );

    if ( eee.sn_num < N2N_EDGE_NUM_SUPERNODES )
    {
        strncpy( (eee.sn_ip_array[eee.sn_num]), strSnAddr.c_str(), N2N_EDGE_SN_HOST_SIZE);
        LogPrint("edge", "Adding supernode[%u] = %s\n", (unsigned int)eee.sn_num, (eee.sn_ip_array[eee.sn_num]) );
        ++eee.sn_num;
    }
    else
    {
        LogPrintf( "Too many supernodes!\n" );
        exit(1);
    }

    memset( eee.community_name, 0, N2N_COMMUNITY_SIZE );
    strncpy( (char *)eee.community_name, strCommunity.c_str(), N2N_COMMUNITY_SIZE);

    for (i=0; i< N2N_EDGE_NUM_SUPERNODES; ++i )
    {
        LogPrint( "edge", "supernode %u => %s\n", i, (eee.sn_ip_array[i]) );
    }

    supernode2addr( &(eee.supernode), eee.sn_ip_array[eee.sn_idx] );

    if ( (NULL == encrypt_key ) && ( 0 == strlen(eee.keyschedule)) )
    {
        //traceEvent(TRACE_WARNING, "Encryption is disabled in edge.");
        
        eee.null_transop = 1;
    }


#ifndef WIN32
    /* If running suid root then we need to setuid before using the force. */
    // setuid( 0 );
    /* setgid( 0 ); */
#endif

    if ( 0 == strcmp( "dhcp", ip_mode ) )
    {
        //traceEvent(TRACE_NORMAL, "Dynamic IP address assignment enabled.");

        eee.dyn_ip_mode = 1;
    }
    else
    {
        LogPrint("edge", "ip_mode='%s'\n", ip_mode);        
    }

    if(tuntap_open(&(eee.device), tuntap_dev_name, ip_mode, ip_addr, netmask, device_mac, mtu) < 0){
        LogPrintf("dockeredge::ThreadEdgeStart  ERROR: ioctl() [Operation not permitted][-1]\n");
        return(-1);
    }

    if(local_port > 0)
        LogPrint("edge", "Binding to local port %d\n", (signed int)local_port);

    if ( encrypt_key ) {
        if(edge_init_twofish( &eee, (uint8_t *)(encrypt_key), strlen(encrypt_key) ) < 0) {
            fprintf(stderr, "Error: twofish setup failed.\n" );
            return(-1);
        }
    }
    /* else run in NULL mode */


    eee.udp_sock = open_socket(local_port, 1 /*bind ANY*/ );
    if(eee.udp_sock < 0)
    {
        LogPrintf("Failed to bind main UDP port %u\n", (signed int)local_port );
        return(-1);
    }


    if(eee.local_sock_ena) {
        if ( set_localip(&eee) != 0) {
            LogPrintf("set localip failed\n" );
            return(-1);
        }
    }

    eee.udp_mgmt_sock = open_socket(mgmt_port, 0 /* bind LOOPBACK*/ );

    if(eee.udp_mgmt_sock < 0)
    {
        LogPrintf("Failed to bind management UDP port %u\n", (unsigned int)N2N_EDGE_MGMT_PORT );
        return(-1);
    }

    update_supernode_reg(&eee, time(NULL) );
    return run_loop(&eee);
}

static int run_loop(n2n_edge_t * eee )
{
    LogPrint("edge","run_loop start\n");
    size_t numPurged;
    time_t lastIfaceCheck=0;
    time_t lastTransop=0;
	time_t lastStatCalc=0;
	time_t lastStatCalcDiff;
    keep_running=1;

    if(fstart)
        fstart(true);

#ifdef WIN32
    startTunReadThread(eee);
#endif

    /* Main loop
     *
     * select() is used to wait for input on either the TAP fd or the UDP/TCP
     * socket. When input is present the data is read and processed by either
     * readFromIPSocket() or readFromTAPSocket()
     */
    try{
        while(keep_running)
        {
            int rc, max_sock = 0;
            fd_set socket_mask;
            struct timeval wait_time;
            time_t nowTime;

            FD_ZERO(&socket_mask);
            FD_SET(eee->udp_sock, &socket_mask);
            FD_SET(eee->udp_mgmt_sock, &socket_mask);
            max_sock = max( eee->udp_sock, eee->udp_mgmt_sock );
#ifndef WIN32
            FD_SET(eee->device.fd, &socket_mask);
            max_sock = max( max_sock, eee->device.fd );
#endif

            wait_time.tv_sec = SOCKET_TIMEOUT_INTERVAL_SECS; wait_time.tv_usec = 0;

            rc = select(max_sock+1, &socket_mask, NULL, NULL, &wait_time);
            nowTime=time(NULL);

            /* Make sure ciphers are updated before the packet is treated. */
            if ( ( nowTime - lastTransop ) > TRANSOP_TICK_INTERVAL )
            {
                lastTransop = nowTime;

                n2n_tick_transop( eee, nowTime );
            }

            if(rc > 0)
            {
                /* Any or all of the FDs could have input; check them all. */

                if(FD_ISSET(eee->udp_sock, &socket_mask))
                {
                    /* Read a cooked socket from the internet socket. Writes on the TAP
                    * socket. */
                    readFromIPSocket(eee);
                }

                if(FD_ISSET(eee->udp_mgmt_sock, &socket_mask))
                {
                    /* Read a cooked socket from the internet socket. Writes on the TAP
                    * socket. */
                    readFromMgmtSocket(eee, &keep_running);
                }

#ifndef WIN32
                if(FD_ISSET(eee->device.fd, &socket_mask))
                {
                    /* Read an ethernet frame from the TAP socket. Write on the IP
                    * socket. */
                    readFromTAPSocket(eee);
                }
    #endif
            }

            /* Finished processing select data. */


            update_supernode_reg(eee, nowTime);

            numPurged = 0;
            if ((nowTime - eee->last_purge) >= PURGE_REGISTRATION_FREQUENCY) {
                numPurged += hashed_purge_expired_registrations(eee->known_peers);
                numPurged += hashed_purge_expired_registrations(eee->pending_peers);
                eee->last_purge = nowTime;
            }
            if ( numPurged > 0 )
            {
                LogPrint( "edge", "Peer removed: pending=%u, operational=%u\n",
                            (unsigned int)hashed_peer_list_t_size( eee->pending_peers ), 
                            (unsigned int)hashed_peer_list_t_size( eee->known_peers ) );
            }

            if ( eee->dyn_ip_mode && 
                (( nowTime - lastIfaceCheck ) > IFACE_UPDATE_INTERVAL ) )
            {
                LogPrint("edge", "Re-checking dynamic IP address.\n");
                tuntap_get_address( &(eee->device) );
                lastIfaceCheck = nowTime;
            }

            lastStatCalcDiff = nowTime - lastStatCalc;
            if(lastStatCalcDiff > STAT_CALC_INTERVAL) {
                // //traceEvent(TRACE_NORMAL, "recalc bps.");
                eee->tx_bps_p2p = eee->tx_bit_p2p / (size_t)lastStatCalcDiff;
                eee->tx_bit_p2p = 0;
                eee->tx_bps_sup = eee->tx_bit_sup / (size_t)lastStatCalcDiff;
                eee->tx_bit_sup = 0;
                eee->rx_bps_p2p = eee->rx_bit_p2p / (size_t)lastStatCalcDiff;
                eee->rx_bit_p2p = 0;
                eee->rx_bps_sup = eee->rx_bit_sup / (size_t)lastStatCalcDiff;
                eee->rx_bit_sup = 0;
                lastStatCalc = nowTime;
            }

            boost::this_thread::interruption_point();
        } /* while */
    }catch(boost::thread_interrupted&){
#ifdef WIN32
    threadTun.interrupt();
    threadTun.join();
#endif
    LogPrintf("EdgePreStop thread_interrupted\n");
    }
    send_deregister( eee, &(eee->supernode));

    closesocket(eee->udp_sock);
    tuntap_close(&(eee->device));

    edge_deinit( eee );
    LogPrintf("EdgeStop\n");
    return(0);
}

int n2n_main(int argc, char **argv) {
        int flag = real_main(argc,argv);
        if(fstart)
            fstart(false);
        return flag;
}

int n2n_stop(void *param1) {
       keep_running=0;
       return 0;
}

void EdgeStart()
{
    LogPrintf("EdgeStart\n");
    sd.name = "edge",
    sd.desc = "edge - n2n VPN Edge node",
    sd.main = n2n_main,
    sd.stop = n2n_stop;
    SCM_Start(&sd,0,NULL)!=SVC_OK;
}
bool ThreadEdgeStart(std::string community,std::string localaddr,std::string snaddr,std::function<void(bool)>start){
    LogPrintf("ThreadEdgeStart\n");
    strCommunity = community;
    strLocalAddr = localaddr;
    strSnAddr = snaddr;
    fstart = start;
    if(thrd && !thrd->timed_join(boost::posix_time::seconds(1))){
        LogPrintf("ThreadEdgeStart existed thread,Please Stop first\n");
        return false;
    }
    thrd = fthreadGroup->create_thread(std::bind(EdgeStart));
    if(thrd && !thrd->timed_join(boost::posix_time::seconds(1))){
        LogPrintf("ThreadEdgeStarted\n");
        return true;
    }
    LogPrintf("ThreadEdgeStart Failed\n");
    return false;
}
void ThreadEdgeStop(){
    LogPrintf("ThreadEdgeStop\n");
    if(!thrd) return;
    thrd->interrupt();
    if(fthreadGroup->is_thread_in(thrd)){
        fthreadGroup->remove_thread(thrd);
        thrd = NULL;
    }
}
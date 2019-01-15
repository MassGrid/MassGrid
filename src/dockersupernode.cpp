/* Supernode for n2n-2.x */

/* (c) 2009 Richard Andrews <andrews@ntop.org> 
 *
 * Contributions by:
 *    Lukasz Taczuk
 *    Struan Bartlett
 */


// #include <boost/thread.hpp>
#include "dockersupernode.h"
#include "init.h"
#include "n2n/include/n2n.h"


#define N2N_SN_LPORT_DEFAULT 8999
#define N2N_SN_PKTBUF_SIZE   2048

#define N2N_SN_MGMT_PORT                5645


struct sn_stats
{
    size_t errors;              /* Number of errors encountered. */
    size_t reg_super;           /* Number of REGISTER_SUPER requests received. */
    size_t reg_super_nak;       /* Number of REGISTER_SUPER requests declined. */
    size_t fwd;                 /* Number of messages forwarded. */
    size_t broadcast;           /* Number of messages broadcast to a community. */
    time_t last_fwd;            /* Time when last message was forwarded. */
    time_t last_reg_super;      /* Time when last REGISTER_SUPER was received. */
};

typedef struct sn_stats sn_stats_t;

struct n2n_sn
{
    time_t              start_time;     /* Used to measure uptime. */
    sn_stats_t          stats;
    int                 daemon;         /* If non-zero then daemonise. */
    uint16_t            lport;          /* Local UDP port to bind to. */
    int                 sock;           /* Main socket for UDP traffic with edges. */
    int                 mgmt_sock;      /* management socket. */
    time_t              last_purge;     /* last purge time */
    peer_info_t *		edges[PEER_HASH_TAB_SIZE];          /* Link list of registered edges. */
};

typedef struct n2n_sn n2n_sn_t;


static int try_forward( n2n_sn_t * sss, 
                        const n2n_common_t * cmn,
                        const n2n_mac_t dstMac,
                        const uint8_t * pktbuf,
                        size_t pktsize );

static int try_broadcast( n2n_sn_t * sss, 
                          const n2n_common_t * cmn,
                          const n2n_mac_t srcMac,
                          const uint8_t * pktbuf,
                          size_t pktsize );



/** Initialise the supernode structure */
static int init_sn( n2n_sn_t * sss )
{
#ifdef WIN32
    initWin32();
#endif
    memset( sss, 0, sizeof(n2n_sn_t) );

    sss->daemon = 0; /* By defult run as a daemon. */
    sss->lport = N2N_SN_LPORT_DEFAULT;
    sss->sock = -1;
    sss->mgmt_sock = -1;
    sss->last_purge = 0;
	sglib_hashed_peer_info_t_init(sss->edges);

    return 0; /* OK */
}

/** Deinitialise the supernode structure and deallocate any memory owned by
 *  it. */
static void deinit_sn( n2n_sn_t * sss )
{
    if (sss->sock >= 0)
    {
        closesocket(sss->sock);
    }
    sss->sock=-1;

    if ( sss->mgmt_sock >= 0 )
    {
        closesocket(sss->mgmt_sock);
    }
    sss->mgmt_sock=-1;

    purge_hashed_peer_list_t(sss->edges, 0xffffffff);
}


/** Determine the appropriate lifetime for new registrations.
 *
 *  If the supernode has been put into a pre-shutdown phase then this lifetime
 *  should not allow registrations to continue beyond the shutdown point.
 */
static uint16_t reg_lifetime( n2n_sn_t * sss )
{
    return 30;
}


/** Update the edge table with the details of the edge which contacted the
 *  supernode. */
static int update_edge( n2n_sn_t * sss, 
                        const n2n_REGISTER_SUPER_t * reg,
                        const n2n_community_t community,
                        const n2n_sock_t * sender_sock,
                        time_t now)
{
    macstr_t            mac_buf;
    n2n_sock_str_t      sockbuf;
    struct peer_info *  scan;

    LogPrint("sn", "update_edge for %s [%s]\n",
                macaddr_str( mac_buf, reg->edgeMac ),
                sock_to_cstr( sockbuf, sender_sock ) );

    scan = find_peer_by_mac( sss->edges, reg->edgeMac );

    if ( NULL == scan )
    {
        /* Not known */

        scan = (struct peer_info*)calloc(1, sizeof(struct peer_info)); /* deallocated in purge_expired_registrations */

        memcpy(scan->community_name, community, sizeof(n2n_community_t) );
        memcpy(&(scan->mac_addr), reg->edgeMac, sizeof(n2n_mac_t));
        memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));

        scan->timeout = reg->timeout;
        if(reg->aflags & N2N_AFLAGS_LOCAL_SOCKET) {
            scan->num_sockets = 2;
            scan->sockets = (n2n_sock_t *)malloc(scan->num_sockets*sizeof(n2n_sock_t));
            scan->sockets[1] = reg->local_sock;
        } else {
            scan->num_sockets = 1;
            scan->sockets = (n2n_sock_t *)malloc(scan->num_sockets*sizeof(n2n_sock_t));
        }
        scan->sockets[0] = scan->sock;

        /* insert this guy at the head of the edges list */
		sglib_hashed_peer_info_t_add(sss->edges, scan);

        LogPrint("sn", "update_edge created   %s ==> %s\n",
                    macaddr_str( mac_buf, reg->edgeMac ),
                    sock_to_cstr( sockbuf, sender_sock ) );
    }
    else
    {
        /* Known */
        int num_changes = 0;
        scan->timeout = reg->timeout;
        if (0 != memcmp(community, scan->community_name, sizeof(n2n_community_t)))
        {
            memcpy(scan->community_name, community, sizeof(n2n_community_t) );
            num_changes++;
        }
        if (0 != sock_equal(sender_sock, &(scan->sock) )) {
            memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));
            memcpy(scan->sockets, sender_sock, sizeof(n2n_sock_t));
            num_changes++;
        }
        if (scan->num_sockets == 1) {
            if (reg->aflags & N2N_AFLAGS_LOCAL_SOCKET) {
                scan->num_sockets = 2;
                free(scan->sockets);
                scan->sockets = (n2n_sock_t *)malloc(scan->num_sockets*sizeof(n2n_sock_t));
                scan->sockets[0] = scan->sock;
                scan->sockets[1] = reg->local_sock;
            }
        } else {
            if (reg->aflags & N2N_AFLAGS_LOCAL_SOCKET) {
                if (0 != sock_equal(&(reg->local_sock), scan->sockets+1 )) {
                    memcpy(scan->sockets+1, &(reg->local_sock), sizeof(n2n_sock_t));
                }
            } else {
                scan->num_sockets = 1;
                free(scan->sockets);
                scan->sockets = (n2n_sock_t *)malloc(scan->num_sockets*sizeof(n2n_sock_t));
                scan->sockets[0] = scan->sock;
            }
        }
        if (num_changes) {
            LogPrint("sn", "update_edge updated   %s ==> %s\n",
                        macaddr_str( mac_buf, reg->edgeMac ),
                        sock_to_cstr( sockbuf, sender_sock ) );
        } else {
            LogPrint("sn", "update_edge unchanged %s ==> %s\n",
                        macaddr_str( mac_buf, reg->edgeMac ),
                        sock_to_cstr( sockbuf, sender_sock ) );
        }

    }

    scan->last_seen = now;
    return 0;
}


/** Send a datagram to the destination embodied in a n2n_sock_t.
 *
 *  @return -1 on error otherwise number of bytes sent
 */
static ssize_t sendto_sock(n2n_sn_t * sss, 
                           const n2n_sock_t * sock, 
                           const uint8_t * pktbuf, 
                           size_t pktsize)
{
    n2n_sock_str_t      sockbuf;

    if ( AF_INET == sock->family )
    {
        struct sockaddr_in udpsock;

        udpsock.sin_family = AF_INET;
        udpsock.sin_port = htons( sock->port );
        memcpy( &(udpsock.sin_addr.s_addr), &(sock->addr.v4), IPV4_SIZE );

        LogPrint("sn", "sendto_sock %lu to [%s]\n",
                    pktsize,
                    sock_to_cstr( sockbuf, sock ) );

        return sendto( sss->sock, pktbuf, pktsize, 0, 
                       (const struct sockaddr *)&udpsock, sizeof(struct sockaddr_in) );
    }
    else
    {
        /* AF_INET6 not implemented */
        errno = EAFNOSUPPORT;
        return -1;
    }
}



/** Try to forward a message to a unicast MAC. If the MAC is unknown then
 *  broadcast to all edges in the destination community.
 */
static int try_forward( n2n_sn_t * sss, 
                        const n2n_common_t * cmn,
                        const n2n_mac_t dstMac,
                        const uint8_t * pktbuf,
                        size_t pktsize )
{
    struct peer_info *  scan;
    macstr_t            mac_buf;
    n2n_sock_str_t      sockbuf;

    scan = find_peer_by_mac( sss->edges, dstMac );

    if ( NULL != scan )
    {
        int data_sent_len;
        data_sent_len = sendto_sock( sss, &(scan->sock), pktbuf, pktsize );

        if ( data_sent_len == pktsize )
        {
            ++(sss->stats.fwd);
            LogPrint("sn", "unicast %lu to [%s] %s\n",
                       pktsize,
                       sock_to_cstr( sockbuf, &(scan->sock) ),
                       macaddr_str(mac_buf, scan->mac_addr));
        }
        else
        {
            ++(sss->stats.errors);
            LogPrintf( "unicast %lu to [%s] %s FAILED (%d: %s)\n",
                       pktsize,
                       sock_to_cstr( sockbuf, &(scan->sock) ),
                       macaddr_str(mac_buf, scan->mac_addr),
                       errno, strerror(errno) );
        }
    }
    else
    {
        LogPrint("sn", "try_forward unknown MAC\n" );

        /* Not a known MAC so drop. */
    }
    
    return 0;
}


/** Try and broadcast a message to all edges in the community.
 *
 *  This will send the exact same datagram to zero or more edges registered to
 *  the supernode.
 */
static int try_broadcast( n2n_sn_t * sss, 
                          const n2n_common_t * cmn,
                          const n2n_mac_t srcMac,
                          const uint8_t * pktbuf,
                          size_t pktsize )
{
    peer_info_t *		ll;
	struct sglib_hashed_peer_info_t_iterator it;
    macstr_t            mac_buf;
    n2n_sock_str_t      sockbuf;

    LogPrint("sn", "try_broadcast\n" );

	for(ll=sglib_hashed_peer_info_t_it_init(&it,sss->edges); ll!=NULL; ll=sglib_hashed_peer_info_t_it_next(&it)) {
        if( 0 == (memcmp(ll->community_name, cmn->community, sizeof(n2n_community_t)) )
            && (0 != memcmp(srcMac, ll->mac_addr, sizeof(n2n_mac_t)) ) )
            /* REVISIT: exclude if the destination socket is where the packet came from. */
        {
            int data_sent_len;
          
            data_sent_len = sendto_sock(sss, &(ll->sock), pktbuf, pktsize);

            if(data_sent_len != pktsize)
            {
                ++(sss->stats.errors);
                LogPrintf( "multicast %lu to [%s] %s failed %s\n",
                           pktsize,
                           sock_to_cstr( sockbuf, &(ll->sock) ),
                           macaddr_str(mac_buf, ll->mac_addr),
                           strerror(errno));
            }
            else 
            {
                ++(sss->stats.broadcast);
                LogPrint("sn", "multicast %lu to [%s] %s\n",
                           pktsize,
                           sock_to_cstr( sockbuf, &(ll->sock) ),
                           macaddr_str(mac_buf, ll->mac_addr));
            }
        }
    } /* for */
    
    return 0;
}


static int process_mgmt( n2n_sn_t * sss, 
                         const struct sockaddr_in * sender_sock,
                         const uint8_t * mgmt_buf, 
                         size_t mgmt_size,
                         time_t now)
{
    char resbuf[N2N_SN_PKTBUF_SIZE];
    size_t ressize=0;
    ssize_t r;

    LogPrint("sn", "process_mgmt\n" );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "----------------\n" );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "uptime    %lu\n", (now - sss->start_time) );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "edges     %u\n", 
						 (unsigned int)hashed_peer_list_t_size( sss->edges ));

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "errors    %u\n", 
			 (unsigned int)sss->stats.errors );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "reg_sup   %u\n", 
			 (unsigned int)sss->stats.reg_super );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "reg_nak   %u\n", 
			 (unsigned int)sss->stats.reg_super_nak );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "fwd       %u\n",
			 (unsigned int) sss->stats.fwd );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "broadcast %u\n",
			 (unsigned int) sss->stats.broadcast );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "last fwd  %lu sec ago\n", 
			 (long unsigned int)(now - sss->stats.last_fwd) );

    ressize += snprintf( resbuf+ressize, N2N_SN_PKTBUF_SIZE-ressize, 
                         "last reg  %lu sec ago\n",
			 (long unsigned int) (now - sss->stats.last_reg_super) );


    r = sendto( sss->mgmt_sock, resbuf, ressize, 0/*flags*/, 
                (struct sockaddr *)sender_sock, sizeof(struct sockaddr_in) );

    if ( r <= 0 )
    {
        ++(sss->stats.errors);
        LogPrintf( "process_mgmt : sendto failed. %s\n", strerror(errno) );
    }

    return 0;
}


/** Examine a datagram and determine what to do with it.
 *
 */
static int process_udp( n2n_sn_t * sss, 
                        const struct sockaddr_in * sender_sock,
                        const uint8_t * udp_buf, 
                        size_t udp_size,
                        time_t now)
{
    n2n_common_t        cmn; /* common fields in the packet header */
    n2n_common_t        cmn2; /* common fields of newly generated packets */
    size_t              rem;
    size_t              idx;
    size_t              msg_type;
    uint8_t             from_supernode;
    macstr_t            mac_buf;
    macstr_t            mac_buf2;
    n2n_sock_str_t      sockbuf;
    const uint8_t *     rec_buf; /* either udp_buf or encbuf */
    int                 unicast; /* non-zero if unicast */
    size_t              encx=0;
    uint8_t             encbuf[N2N_SN_PKTBUF_SIZE];
    n2n_ETHFRAMEHDR_t   eth;
    int                 i;

    /* for PACKET packages */
    n2n_PACKET_t                    pkt; 

    /* for QUERY_PEER packages */
    n2n_QUERY_PEER_t                query;
    struct peer_info *              scan;
    n2n_PEER_INFO_t                 pi;

    /* for REGISTER packages */
    n2n_REGISTER_t                  reg;

    /* for REGISTER_SUPER packages */
    n2n_REGISTER_SUPER_t            regs;
    n2n_REGISTER_SUPER_ACK_t        ack;

    LogPrint("sn", "process_udp(%lu)\n", udp_size );

    /* Use decode_common() to determine the kind of packet then process it:
     *
     * REGISTER_SUPER adds an edge and generate a return REGISTER_SUPER_ACK
     *
     * REGISTER, REGISTER_ACK and PACKET messages are forwarded to their
     * destination edge. If the destination is not known then PACKETs are
     * broadcast.
     */

    rem = udp_size; /* Counts down bytes of packet to protect against buffer overruns. */
    idx = 0; /* marches through packet header as parts are decoded. */
    if ( decode_common(&cmn, udp_buf, &rem, &idx) < 0 )
    {
        LogPrintf( "Failed to decode common section\n" );
        return -1; /* failed to decode packet */
    }

    msg_type = cmn.pc; /* packet code */
    from_supernode= cmn.flags & N2N_FLAGS_FROM_SUPERNODE;

    if ( cmn.ttl < 1 )
    {
        LogPrintf( "Expired TTL\n" );
        return 0; /* Don't process further */
    }

    --(cmn.ttl); /* The value copied into all forwarded packets. */

    switch(msg_type) {
    case MSG_TYPE_PACKET:
        /* PACKET from one edge to another edge via supernode. */

        /* pkt will be modified in place and recoded to an output of potentially
         * different size due to addition of the socket.*/


        sss->stats.last_fwd=now;
        decode_PACKET( &pkt, &cmn, udp_buf, &rem, &idx );
        decode_ETHFRAMEHDR( &eth, udp_buf+idx );

        unicast = (0 == is_multi_broadcast(eth.dstMac) );

        LogPrint("sn", "Rx PACKET (%s) %s -> %s %s\n",
                    (unicast?"unicast":"multicast"),
                    macaddr_str( mac_buf, eth.srcMac ),
                    macaddr_str( mac_buf2, eth.dstMac ),
                    (from_supernode?"from sn":"local") );

        if ( !from_supernode )
        {
            memcpy( &cmn2, &cmn, sizeof( n2n_common_t ) );

            /* We are going to add socket even if it was not there before */
            cmn2.flags |= N2N_FLAGS_FROM_SUPERNODE;

            rec_buf = encbuf;

            /* Re-encode the header. */
            encode_PACKET( encbuf, &encx, &cmn2, &pkt );

            /* Copy the original payload unchanged */
            encode_buf( encbuf, &encx, (udp_buf + idx), (udp_size - idx ) );
        }
        else
        {
            /* Already from a supernode. Nothing to modify, just pass to
             * destination. */

            LogPrint("sn", "Rx PACKET fwd unmodified\n" );

            rec_buf = udp_buf;
            encx = udp_size;
        }

        /* Common section to forward the final product. */
        if ( unicast )
        {
            try_forward( sss, &cmn, eth.dstMac, rec_buf, encx );
        }
        else
        {
            try_broadcast( sss, &cmn, eth.srcMac, rec_buf, encx );
        }
        break;
    case MSG_TYPE_QUERY_PEER:
        decode_QUERY_PEER( &query, &cmn, udp_buf, &rem, &idx );

        LogPrint("sn", "Rx QUERY_PEER from %s for %s\n",
                    macaddr_str( mac_buf,  query.srcMac ),
                    macaddr_str( mac_buf2, query.targetMac ) );

        scan = find_peer_by_mac( sss->edges, query.targetMac );
        if (scan && 0 == memcmp(cmn.community, scan->community_name,
                                sizeof(n2n_community_t))) {
            cmn2.ttl = N2N_DEFAULT_TTL;
            cmn2.pc = n2n_peer_info;
            cmn2.flags = N2N_FLAGS_FROM_SUPERNODE;
            memcpy( cmn2.community, cmn.community, sizeof(n2n_community_t) );

            pi.aflags = 0;
            pi.timeout = scan->timeout;
            memcpy( pi.mac, query.targetMac, sizeof(n2n_mac_t) );
            for(i=0; i<scan->num_sockets; i++)
                pi.sockets[i] = scan->sockets[i];
            if(scan->num_sockets > 1)
                pi.aflags |= N2N_AFLAGS_LOCAL_SOCKET;

            encode_PEER_INFO( encbuf, &encx, &cmn2, &pi );

            sendto( sss->sock, encbuf, encx, 0, 
                    (struct sockaddr *)sender_sock, sizeof(struct sockaddr_in) );

            LogPrint("sn", "Tx PEER_INFO to %s\n",
                        macaddr_str( mac_buf, query.srcMac ) );
        } else {
            LogPrint("sn", "Ignoring QUERY_PEER for unknown edge %s\n",
                        macaddr_str( mac_buf, query.targetMac ) );
        }

        break;
    case MSG_TYPE_REGISTER:
        /* Forwarding a REGISTER from one edge to the next */


        sss->stats.last_fwd=now;
        decode_REGISTER( &reg, &cmn, udp_buf, &rem, &idx );

        unicast = (0 == is_multi_broadcast(reg.dstMac) );
        
        if ( unicast )
        {
        LogPrint("sn", "Rx REGISTER %s -> %s %s\n",
                    macaddr_str( mac_buf, reg.srcMac ),
                    macaddr_str( mac_buf2, reg.dstMac ),
                    ((cmn.flags & N2N_FLAGS_FROM_SUPERNODE)?"from sn":"local") );

        if ( 0 != (cmn.flags & N2N_FLAGS_FROM_SUPERNODE) )
        {
            memcpy( &cmn2, &cmn, sizeof( n2n_common_t ) );

            /* We are going to add socket even if it was not there before */
            cmn2.flags |= N2N_FLAGS_FROM_SUPERNODE;

            reg.sock.family = AF_INET;
            reg.sock.port = ntohs(sender_sock->sin_port);
            memcpy( reg.sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );

            rec_buf = encbuf;

            /* Re-encode the header. */
            encode_REGISTER( encbuf, &encx, &cmn2, &reg );

            /* Copy the original payload unchanged */
            encode_buf( encbuf, &encx, (udp_buf + idx), (udp_size - idx ) );
        }
        else
        {
            /* Already from a supernode. Nothing to modify, just pass to
             * destination. */

            rec_buf = udp_buf;
            encx = udp_size;
        }

        try_forward( sss, &cmn, reg.dstMac, rec_buf, encx ); /* unicast only */
        }
        else
        {
            LogPrintf( "Rx REGISTER with multicast destination\n" );
        }
        break;
    case MSG_TYPE_REGISTER_ACK:
        LogPrint("sn", "Rx REGISTER_ACK (NOT IMPLEMENTED) SHould not be via supernode\n" );
        break;
    case MSG_TYPE_REGISTER_SUPER:
        /* Edge requesting registration with us.  */
        
        sss->stats.last_reg_super=now;
        ++(sss->stats.reg_super);
        decode_REGISTER_SUPER( &regs, &cmn, udp_buf, &rem, &idx );

        cmn2.ttl = N2N_DEFAULT_TTL;
        cmn2.pc = n2n_register_super_ack;
        cmn2.flags = N2N_FLAGS_FROM_SUPERNODE;
        memcpy( cmn2.community, cmn.community, sizeof(n2n_community_t) );

        memcpy( &(ack.cookie), &(regs.cookie), sizeof(n2n_cookie_t) );
        memcpy( ack.edgeMac, regs.edgeMac, sizeof(n2n_mac_t) );
        ack.lifetime = reg_lifetime( sss );

        ack.sock.family = AF_INET;
        ack.sock.port = ntohs(sender_sock->sin_port);
        memcpy( ack.sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE );

        ack.num_sn=0; /* No backup */
        memset( &(ack.sn_bak), 0, sizeof(n2n_sock_t) );

        LogPrint("sn", "Rx REGISTER_SUPER for %s [%s]\n",
                    macaddr_str( mac_buf, regs.edgeMac ),
                    sock_to_cstr( sockbuf, &(ack.sock) ) );

        update_edge( sss, &regs, cmn.community, &(ack.sock), now );

        encode_REGISTER_SUPER_ACK( encbuf, &encx, &cmn2, &ack );

        sendto( sss->sock, encbuf, encx, 0, 
                (struct sockaddr *)sender_sock, sizeof(struct sockaddr_in) );

        LogPrint("sn", "Tx REGISTER_SUPER_ACK for %s [%s]\n",
                    macaddr_str( mac_buf, regs.edgeMac ),
                    sock_to_cstr( sockbuf, &(ack.sock) ) );
        break;
    default:
        /* Not a known message type */
        LogPrintf( "Unable to handle packet type %d: ignored\n", (signed int)msg_type);
        break;
    }

    return 0;
}


static int run_loop( n2n_sn_t * sss );

/* *********************************************** */

static const struct option long_options[] = {
  { "foreground",      no_argument,       NULL, 'f' },
  { "local-port",      required_argument, NULL, 'l' },
  { "help"   ,         no_argument,       NULL, 'h' },
  { "verbose",         no_argument,       NULL, 'v' },
  { NULL,              0,                 NULL,  0  }
};
/** Main program entry point from kernel. */
void ThreadSnStart()
{

    RenameThread("massgrid-sn");

    LogPrintf( "ThreadSnStart\n" );
    n2n_sn_t sss;
#ifndef WIN32
    uid_t   userid=0; /* root is the only guaranteed ID */
    gid_t   groupid=0; /* root is the only guaranteed ID */
#endif

    init_sn( &sss );



#if defined(N2N_HAVE_DAEMON)
    if (sss.daemon)
    {
        useSyslog=1; /* traceEvent output now goes to syslog. */
        if ( -1 == daemon( 0, 0 ) )
        {
            LogPrintf( "Failed to become daemon.\n" );
            exit(-5);
        }
    }
#endif /* #if defined(N2N_HAVE_DAEMON) */

    LogPrint("sn", "traceLevel is %d\n", traceLevel);

    sss.sock = open_socket(sss.lport, 1 /*bind ANY*/ );
    if ( -1 == sss.sock )
    {
        LogPrintf( "Failed to open main socket. %s\n", strerror(errno) );
        exit(-2);
    }
    else
    {
        LogPrint("sn", "supernode is listening on UDP %u (main)\n", sss.lport );
    }

    sss.mgmt_sock = open_socket(N2N_SN_MGMT_PORT, 0 /* bind LOOPBACK */ );
    if ( -1 == sss.mgmt_sock )
    {
        LogPrintf( "Failed to open management socket. %s\n", strerror(errno) );
        exit(-2);
    }
    else
    {
        LogPrint("sn", "supernode is listening on UDP %u (management)\n", N2N_SN_MGMT_PORT );
    }

#ifndef WIN32
    if ( (userid != 0) || (groupid != 0 ) ) {
	    LogPrint("sn", "Interface up. Dropping privileges to uid=%d, gid=%d\n",
			    (signed int)userid, (signed int)groupid);

	    /* Finished with the need for root privileges. Drop to unprivileged user. */
	    setreuid( userid, userid );
	    setregid( groupid, groupid );
    }
#endif

    LogPrintf( "ThreadSnStop\n" );
    run_loop(&sss);
}


/** Long lived processing entry point. Split out from main to simply
 *  daemonisation on some platforms. */
static int run_loop( n2n_sn_t * sss )
{
    uint8_t pktbuf[N2N_SN_PKTBUF_SIZE];
    int keep_running=1;

    sss->start_time = time(NULL);

    while(keep_running) 
    {
        int rc;
        ssize_t bread;
        int max_sock;
        fd_set socket_mask;
        struct timeval wait_time;
        time_t now=0;

        FD_ZERO(&socket_mask);
        max_sock = MAX(sss->sock, sss->mgmt_sock);

        FD_SET(sss->sock, &socket_mask);
        FD_SET(sss->mgmt_sock, &socket_mask);

        wait_time.tv_sec = 10; wait_time.tv_usec = 0;
        rc = select(max_sock+1, &socket_mask, NULL, NULL, &wait_time);

        now = time(NULL);

        if(rc > 0) 
        {
            if (FD_ISSET(sss->sock, &socket_mask)) 
            {
                struct sockaddr_in  sender_sock;
                socklen_t           i;

                i = sizeof(sender_sock);
                bread = recvfrom( sss->sock, pktbuf, N2N_SN_PKTBUF_SIZE, 0/*flags*/,
				  (struct sockaddr *)&sender_sock, (socklen_t*)&i);

                if ( bread < 0 ) /* For UDP bread of zero just means no data (unlike TCP). */
                {
                    /* The fd is no good now. Maybe we lost our interface. */
                    LogPrintf( "recvfrom() failed %d errno %d (%s)\n", bread, errno, strerror(errno) );
                    keep_running=0;
                    break;
                }

                /* We have a datagram to process */
                if ( bread > 0 )
                {
                    /* And the datagram has data (not just a header) */
                    process_udp( sss, &sender_sock, pktbuf, bread, now );
                }
            }

            if (FD_ISSET(sss->mgmt_sock, &socket_mask)) 
            {
                struct sockaddr_in  sender_sock;
                size_t              i;

                i = sizeof(sender_sock);
                bread = recvfrom( sss->mgmt_sock, pktbuf, N2N_SN_PKTBUF_SIZE, 0/*flags*/,
				  (struct sockaddr *)&sender_sock, (socklen_t*)&i);

                if ( bread <= 0 )
                {
                    LogPrintf( "recvfrom() failed %d errno %d (%s)\n", bread, errno, strerror(errno) );
                    keep_running=0;
                    break;
                }

                /* We have a datagram to process */
                process_mgmt( sss, &sender_sock, pktbuf, bread, now );
            }
        }
        else
        {
            LogPrint("sn", "timeout\n" );
        }
        if ((now - sss->last_purge) >= PURGE_REGISTRATION_FREQUENCY) {
            hashed_purge_expired_registrations( sss->edges );
            sss->last_purge = now;
        }

        boost::this_thread::interruption_point();
    } /* while */

    deinit_sn( sss );

    return 0;
}


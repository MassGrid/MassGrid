/**
 * (C) 2007-18 - ntop.org and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not see see <http://www.gnu.org/licenses/>
 *
 */

#ifndef _N2N_H_
#define _N2N_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
  tunctl -t tun0
  tunctl -t tun1
  ifconfig tun0 1.2.3.4 up
  ifconfig tun1 1.2.3.5 up
  ./edge -d tun0 -l 2000 -r 127.0.0.1:3000 -c hello
  ./edge -d tun1 -l 3000 -r 127.0.0.1:2000 -c hello


  tunctl -u UID -t tunX
*/


/* #define N2N_CAN_NAME_IFACE */

/* Moved here to define _CRT_SECURE_NO_WARNINGS before all the including takes place */
#ifdef WIN32
#include "win32/n2n_win32.h"
#include "win32/winconfig.h"
#undef N2N_HAVE_DAEMON
#undef N2N_HAVE_SETUID
#else
#include "../config.h"
#endif


#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef WIN32
#include <netdb.h>
#endif

#ifndef _MSC_VER
#include <getopt.h>
#endif /* #ifndef _MSC_VER */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <pthread.h>

#ifdef __linux__
#include <linux/if.h>
#include <linux/if_tun.h>
#define N2N_CAN_NAME_IFACE 1
#endif /* #ifdef __linux__ */

#ifdef __FreeBSD__
#include <netinet/in_systm.h>
#endif /* #ifdef __FreeBSD__ */

#include <syslog.h>
#include <sys/wait.h>

#define ETH_ADDR_LEN 6
struct ether_hdr
{
  uint8_t  dhost[ETH_ADDR_LEN];
  uint8_t  shost[ETH_ADDR_LEN];
  uint16_t type;                /* higher layer protocol encapsulated */
} __attribute__ ((__packed__));

typedef struct ether_hdr ether_hdr_t;

#ifdef __ANDROID_NDK__
#undef N2N_HAVE_DAEMON
#undef N2N_HAVE_SETUID
#undef N2N_CAN_NAME_IFACE
#endif /* #ifdef __ANDROID_NDK__ */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include "minilzo.h"

#define closesocket(a) close(a)
#endif /* #ifndef WIN32 */

#include <string.h>

#include <stdarg.h>

#include "uthash.h"

#ifdef WIN32
#include "win32/wintap.h"
#endif /* #ifdef WIN32 */

#include "n2n_wire.h"
#include "n2n_transforms.h"

/* N2N_IFNAMSIZ is needed on win32 even if dev_name is not used after declaration */
#define N2N_IFNAMSIZ            16 /* 15 chars * NULL */
#ifndef WIN32
typedef struct tuntap_dev {
  int           fd;
  uint8_t       mac_addr[6];
  uint32_t      ip_addr, device_mask;
  uint16_t      mtu;
  char          dev_name[N2N_IFNAMSIZ];
} tuntap_dev;

#define SOCKET int
#endif /* #ifndef WIN32 */

#define QUICKLZ               1

/* N2N packet header indicators. */
#define MSG_TYPE_REGISTER               1
#define MSG_TYPE_DEREGISTER             2
#define MSG_TYPE_PACKET                 3
#define MSG_TYPE_REGISTER_ACK           4
#define MSG_TYPE_REGISTER_SUPER         5
#define MSG_TYPE_REGISTER_SUPER_ACK     6
#define MSG_TYPE_REGISTER_SUPER_NAK     7
#define MSG_TYPE_FEDERATION             8

/* Set N2N_COMPRESSION_ENABLED to 0 to disable lzo1x compression of ethernet
 * frames. Doing this will break compatibility with the standard n2n packet
 * format so do it only for experimentation. All edges must be built with the
 * same value if they are to understand each other. */
#define N2N_COMPRESSION_ENABLED 1

#define DEFAULT_MTU   1400

/** Common type used to hold stringified IP addresses. */
typedef char ipstr_t[32];

/** Common type used to hold stringified MAC addresses. */
#define N2N_MACSTR_SIZE 32
typedef char macstr_t[N2N_MACSTR_SIZE];

struct peer_info {
  struct peer_info *  next;
  n2n_community_t     community_name;
  n2n_mac_t           mac_addr;
  n2n_sock_t          sock;
  time_t              last_seen;
};

struct n2n_edge; /* defined in edge.c */
typedef struct n2n_edge         n2n_edge_t;

#define N2N_EDGE_SN_HOST_SIZE   48
#define N2N_EDGE_NUM_SUPERNODES 2
#define N2N_EDGE_SUP_ATTEMPTS   3       /* Number of failed attmpts before moving on to next supernode. */
#define N2N_PATHNAME_MAXLEN     256
#define N2N_MAX_TRANSFORMS      16
#define N2N_EDGE_MGMT_PORT      5644


typedef char n2n_sn_name_t[N2N_EDGE_SN_HOST_SIZE];

struct n2n_edge {
  int                 daemon;                 /**< Non-zero if edge should detach and run in the background. */
  int                 preferred_aes;          /**< Non-zero if AES is the preferred encryption meothd. */
  uint8_t             re_resolve_supernode_ip;

  n2n_sock_t          supernode;

  size_t              sn_idx;                 /**< Currently active supernode. */
  size_t              sn_num;                 /**< Number of supernode addresses defined. */
  n2n_sn_name_t       sn_ip_array[N2N_EDGE_NUM_SUPERNODES];
  int                 sn_wait;                /**< Whether we are waiting for a supernode response. */

  n2n_community_t     community_name;         /**< The community. 16 full octets. */
  char                keyschedule[N2N_PATHNAME_MAXLEN];
  int                 null_transop;           /**< Only allowed if no key sources defined. */

  int                 udp_sock;
  int                 udp_mgmt_sock;          /**< socket for status info. */
  int                 udp_multicast_sock;     /**< socket for local multicast registrations. */

  tuntap_dev          device;                 /**< All about the TUNTAP device */
  int                 dyn_ip_mode;            /**< Interface IP address is dynamically allocated, eg. DHCP. */
  int                 allow_routing;          /**< Accept packet no to interface address. */
  int                 drop_multicast;         /**< Multicast ethernet addresses. */

  n2n_trans_op_t      transop[N2N_MAX_TRANSFORMS]; /* one for each transform at fixed positions */
  size_t              tx_transop_idx;         /**< The transop to use when encoding. */
  n2n_sock_t          multicast_peer;         /**< Multicast peer group (for local edges) */
  struct peer_info *  known_peers;            /**< Edges we are connected to. */
  struct peer_info *  pending_peers;          /**< Edges we have tried to register with. */
  time_t              last_register_req;      /**< Check if time to re-register with super*/
  size_t              register_lifetime;      /**< Time distance after last_register_req at which to re-register. */
  time_t              last_p2p;               /**< Last time p2p traffic was received. */
  time_t              last_sup;               /**< Last time a packet arrived from supernode. */
  size_t              sup_attempts;           /**< Number of remaining attempts to this supernode. */
  n2n_cookie_t        last_cookie;            /**< Cookie sent in last REGISTER_SUPER. */

  time_t              start_time;             /**< For calculating uptime */

  /* Statistics */
  size_t              tx_p2p;
  size_t              rx_p2p;
  size_t              tx_sup;
  size_t              rx_sup;
};

/* ************************************** */

#ifdef __ANDROID_NDK__
#include <android/log.h>
#endif /* #ifdef __ANDROID_NDK__ */
#ifndef TRACE_ERROR
#define TRACE_ERROR     0, __FILE__, __LINE__
#define TRACE_WARNING   1, __FILE__, __LINE__
#define TRACE_NORMAL    2, __FILE__, __LINE__
#define TRACE_INFO      3, __FILE__, __LINE__
#define TRACE_DEBUG     4, __FILE__, __LINE__
#endif

/* ************************************** */

#define SUPERNODE_IP    "127.0.0.1"
#define SUPERNODE_PORT  1234

/* ************************************** */

#ifndef max_s
#define max_s(a, b) ((a < b) ? b : a)
#endif

#ifndef min_s
#define min_s(a, b) ((a > b) ? b : a)
#endif

/* ************************************** */

/* Variables */
/* extern TWOFISH *tf; */
extern int traceLevel;
extern int useSyslog;
extern const uint8_t broadcast_addr[6];
extern const uint8_t multicast_addr[6];

/* Functions */
extern void traceEvent(int eventTraceLevel, char* file, int line, char * format, ...);
extern int  tuntap_open(tuntap_dev *device, char *dev, const char *address_mode, char *device_ip, 
			char *device_mask, const char * device_mac, int mtu);
extern int  tuntap_read(struct tuntap_dev *tuntap, unsigned char *buf, int len);
extern int  tuntap_write(struct tuntap_dev *tuntap, unsigned char *buf, int len);
extern void tuntap_close(struct tuntap_dev *tuntap);
extern void tuntap_get_address(struct tuntap_dev *tuntap);

extern SOCKET open_socket(int local_port, int bind_any);

extern char* intoa(uint32_t addr, char* buf, uint16_t buf_len);
extern char* macaddr_str(macstr_t buf, const n2n_mac_t mac);
extern int   str2mac( uint8_t * outmac /* 6 bytes */, const char * s );
extern char * sock_to_cstr( n2n_sock_str_t out,
                            const n2n_sock_t * sock );

extern int sock_equal( const n2n_sock_t * a, 
                       const n2n_sock_t * b );

extern uint8_t is_multi_broadcast(const uint8_t * dest_mac);
extern char* msg_type2str(uint16_t msg_type);
extern void hexdump(const uint8_t * buf, size_t len);

void print_n2n_version();


/* Operations on peer_info lists. */
struct peer_info * find_peer_by_mac( struct peer_info * list,
                                     const n2n_mac_t mac );
void   peer_list_add( struct peer_info * * list,
                      struct peer_info * newp );
size_t peer_list_size( const struct peer_info * list );
size_t purge_peer_list( struct peer_info ** peer_list, 
                        time_t purge_before );
size_t clear_peer_list( struct peer_info ** peer_list );
size_t purge_expired_registrations( struct peer_info ** peer_list );

/* version.c */
extern char *n2n_sw_version, *n2n_sw_osName, *n2n_sw_buildDate;

/* egde_utils.c */
int edge_init(n2n_edge_t * eee);
void supernode2addr(n2n_sock_t * sn, const n2n_sn_name_t addrIn);
void update_supernode_reg(n2n_edge_t * eee, time_t nowTime);
int is_empty_ip_address(const n2n_sock_t * sock);
void update_peer_address(n2n_edge_t * eee,
			 uint8_t from_supernode,
			 const n2n_mac_t mac,
			 const n2n_sock_t * peer,
			 time_t when);
int transop_enum_to_index(n2n_transform_t id);
int edge_init_keyschedule(n2n_edge_t * eee);
void update_peer_address(n2n_edge_t * eee,
			 uint8_t from_supernode,
			 const n2n_mac_t mac,
			 const n2n_sock_t * peer,
			 time_t when);
int is_empty_ip_address(const n2n_sock_t * sock);
void send_register(n2n_edge_t * eee,
		   const n2n_sock_t * remote_peer);
void send_packet2net(n2n_edge_t * eee,
		     uint8_t *tap_pkt, size_t len);
void check_peer(n2n_edge_t * eee,
		uint8_t from_supernode,
		const n2n_mac_t mac,
		const n2n_sock_t * peer);
void set_peer_operational(n2n_edge_t * eee,
			  const n2n_mac_t mac,
			  const n2n_sock_t * peer);
const char * supernode_ip(const n2n_edge_t * eee);
int edge_init_twofish_psk(n2n_edge_t * eee, uint8_t *encrypt_pwd,
		      uint32_t encrypt_pwd_len);
int edge_init_aes_psk(n2n_edge_t * eee, uint8_t *encrypt_pwd,
		      uint32_t encrypt_pwd_len);
int run_edge_loop(n2n_edge_t * eee, int *keep_running);
void edge_term(n2n_edge_t * eee);
const char *random_device_mac(void);
int quick_edge_init(char *device_name, char *community_name,
		    char *encrypt_key, char *device_mac,
		    char *local_ip_address,
		    char *supernode_ip_address_port,
		    int *keep_on_running);
  
#ifdef __cplusplus
}
#endif
#endif /* _N2N_H_ */

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

/* Supernode for n2n-2.x */


#include "dockersupernode.h"
#include "init.h"
#include "n2n/include/n2n.h"

#ifdef WIN32
#include <signal.h>
#endif

#define N2N_SN_LPORT_DEFAULT 8999
#define N2N_SN_PKTBUF_SIZE 2048

#define N2N_SN_MGMT_PORT 5645

struct sn_stats {
    size_t errors;         /* Number of errors encountered. */
    size_t reg_super;      /* Number of REGISTER_SUPER requests received. */
    size_t reg_super_nak;  /* Number of REGISTER_SUPER requests declined. */
    size_t fwd;            /* Number of messages forwarded. */
    size_t broadcast;      /* Number of messages broadcast to a community. */
    time_t last_fwd;       /* Time when last message was forwarded. */
    time_t last_reg_super; /* Time when last REGISTER_SUPER was received. */
};

typedef struct sn_stats sn_stats_t;

struct n2n_sn {
    time_t start_time; /* Used to measure uptime. */
    sn_stats_t stats;
    int daemon;              /* If non-zero then daemonise. */
    uint16_t lport;          /* Local UDP port to bind to. */
    int sock;                /* Main socket for UDP traffic with edges. */
    int mgmt_sock;           /* management socket. */
    struct peer_info* edges; /* Link list of registered edges. */
};

typedef struct n2n_sn n2n_sn_t;

struct n2n_allowed_communities {
    char community[N2N_COMMUNITY_SIZE];
    UT_hash_handle hh; /* makes this structure hashable */
};

static struct n2n_allowed_communities* allowed_communities = NULL;

static int try_forward(n2n_sn_t* sss,
    const n2n_common_t* cmn,
    const n2n_mac_t dstMac,
    const uint8_t* pktbuf,
    size_t pktsize);

static int try_broadcast(n2n_sn_t* sss,
    const n2n_common_t* cmn,
    const n2n_mac_t srcMac,
    const uint8_t* pktbuf,
    size_t pktsize);

static n2n_sn_t sss_node;

/** Initialise the supernode structure */
static int init_sn(n2n_sn_t* sss)
{
#ifdef WIN32
    initWin32();
#endif
    memset(sss, 0, sizeof(n2n_sn_t));

    sss->daemon = 0; /* By defult run as a daemon. */
    sss->lport = N2N_SN_LPORT_DEFAULT;
    sss->sock = -1;
    sss->mgmt_sock = -1;
    sss->edges = NULL;

    return 0; /* OK */
}

/** Deinitialise the supernode structure and deallocate any memory owned by
 *  it. */
static void deinit_sn(n2n_sn_t* sss)
{
    if (sss->sock >= 0) {
        closesocket(sss->sock);
    }
    sss->sock = -1;

    if (sss->mgmt_sock >= 0) {
        closesocket(sss->mgmt_sock);
    }
    sss->mgmt_sock = -1;

    purge_peer_list(&(sss->edges), 0xffffffff);
}


/** Determine the appropriate lifetime for new registrations.
 *
 *  If the supernode has been put into a pre-shutdown phase then this lifetime
 *  should not allow registrations to continue beyond the shutdown point.
 */
static uint16_t reg_lifetime(n2n_sn_t* sss)
{
    /* NOTE: UDP firewalls usually have a 30 seconds timeout */
    return 15;
}


/** Update the edge table with the details of the edge which contacted the
 *  supernode. */
static int update_edge(n2n_sn_t* sss,
    const n2n_mac_t edgeMac,
    const n2n_community_t community,
    const n2n_sock_t* sender_sock,
    time_t now)
{
    macstr_t mac_buf;
    n2n_sock_str_t sockbuf;
    struct peer_info* scan;

   LogPrint("sn", "update_edge for %s [%s]\n",
        macaddr_str(mac_buf, edgeMac),
        sock_to_cstr(sockbuf, sender_sock));

    scan = find_peer_by_mac(sss->edges, edgeMac);

    if (NULL == scan) {
        /* Not known */

        scan = (struct peer_info*)calloc(1, sizeof(struct peer_info)); /* deallocated in purge_expired_registrations */

        memcpy(scan->community_name, community, sizeof(n2n_community_t));
        memcpy(&(scan->mac_addr), edgeMac, sizeof(n2n_mac_t));
        memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));

        /* insert this guy at the head of the edges list */
        scan->next = sss->edges; /* first in list */
        sss->edges = scan;       /* head of list points to new scan */

        LogPrint("sn", "update_edge created   %s ==> %s",
            macaddr_str(mac_buf, edgeMac),
            sock_to_cstr(sockbuf, sender_sock));
    } else {
        /* Known */
        if ((0 != memcmp(community, scan->community_name, sizeof(n2n_community_t))) ||
            (0 != sock_equal(sender_sock, &(scan->sock)))) {
            memcpy(scan->community_name, community, sizeof(n2n_community_t));
            memcpy(&(scan->sock), sender_sock, sizeof(n2n_sock_t));

            LogPrint("sn", "update_edge updated   %s ==> %s\n",
                macaddr_str(mac_buf, edgeMac),
                sock_to_cstr(sockbuf, sender_sock));
        } else {
           LogPrint("sn", "update_edge unchanged %s ==> %s\n",
                macaddr_str(mac_buf, edgeMac),
                sock_to_cstr(sockbuf, sender_sock));
        }
    }

    scan->last_seen = now;
    return 0;
}


/** Send a datagram to the destination embodied in a n2n_sock_t.
 *
 *  @return -1 on error otherwise number of bytes sent
 */
static ssize_t sendto_sock(n2n_sn_t* sss,
    const n2n_sock_t* sock,
    const uint8_t* pktbuf,
    size_t pktsize)
{
    n2n_sock_str_t sockbuf;

    if (AF_INET == sock->family) {
        struct sockaddr_in udpsock;

        udpsock.sin_family = AF_INET;
        udpsock.sin_port = htons(sock->port);
        memcpy(&(udpsock.sin_addr.s_addr), &(sock->addr.v4), IPV4_SIZE);

       LogPrint("sn", "sendto_sock %lu to [%s]\n",
            pktsize,
            sock_to_cstr(sockbuf, sock));

        return sendto(sss->sock, pktbuf, pktsize, 0,
            (const struct sockaddr*)&udpsock, sizeof(struct sockaddr_in));
    } else {
        /* AF_INET6 not implemented */
        errno = EAFNOSUPPORT;
        return -1;
    }
}


/** Try to forward a message to a unicast MAC. If the MAC is unknown then
 *  broadcast to all edges in the destination community.
 */
static int try_forward(n2n_sn_t* sss,
    const n2n_common_t* cmn,
    const n2n_mac_t dstMac,
    const uint8_t* pktbuf,
    size_t pktsize)
{
    struct peer_info* scan;
    macstr_t mac_buf;
    n2n_sock_str_t sockbuf;

    scan = find_peer_by_mac(sss->edges, dstMac);

    if (NULL != scan) {
        int data_sent_len;
        data_sent_len = sendto_sock(sss, &(scan->sock), pktbuf, pktsize);

        if (data_sent_len == pktsize) {
            ++(sss->stats.fwd);
           LogPrint("sn", "unicast %lu to [%s] %s\n",
                pktsize,
                sock_to_cstr(sockbuf, &(scan->sock)),
                macaddr_str(mac_buf, scan->mac_addr));
        } else {
            ++(sss->stats.errors);
            LogPrintf( "unicast %lu to [%s] %s FAILED (%d: %s)\n",
                pktsize,
                sock_to_cstr(sockbuf, &(scan->sock)),
                macaddr_str(mac_buf, scan->mac_addr),
                errno, strerror(errno));
        }
    } else {
       LogPrint("sn", "try_forward unknown MAC\n");

        /* Not a known MAC so drop. */
    }

    return 0;
}


/** Try and broadcast a message to all edges in the community.
 *
 *  This will send the exact same datagram to zero or more edges registered to
 *  the supernode.
 */
static int try_broadcast(n2n_sn_t* sss,
    const n2n_common_t* cmn,
    const n2n_mac_t srcMac,
    const uint8_t* pktbuf,
    size_t pktsize)
{
    struct peer_info* scan;
    macstr_t mac_buf;
    n2n_sock_str_t sockbuf;

   LogPrint("sn", "try_broadcast\n");

    scan = sss->edges;
    while (scan != NULL) {
        if (0 == (memcmp(scan->community_name, cmn->community, sizeof(n2n_community_t))) && (0 != memcmp(srcMac, scan->mac_addr, sizeof(n2n_mac_t))))
        /* REVISIT: exclude if the destination socket is where the packet came from. */
        {
            int data_sent_len;

            data_sent_len = sendto_sock(sss, &(scan->sock), pktbuf, pktsize);

            if (data_sent_len != pktsize) {
                ++(sss->stats.errors);
                LogPrintf( "multicast %lu to [%s] %s failed %s\n",
                    pktsize,
                    sock_to_cstr(sockbuf, &(scan->sock)),
                    macaddr_str(mac_buf, scan->mac_addr),
                    strerror(errno));
            } else {
                ++(sss->stats.broadcast);
               LogPrint("sn", "multicast %lu to [%s] %s\n",
                    pktsize,
                    sock_to_cstr(sockbuf, &(scan->sock)),
                    macaddr_str(mac_buf, scan->mac_addr));
            }
        }

        scan = scan->next;
    } /* while */

    return 0;
}


static int process_mgmt(n2n_sn_t* sss,
    const struct sockaddr_in* sender_sock,
    const uint8_t* mgmt_buf,
    size_t mgmt_size,
    time_t now)
{
    char resbuf[N2N_SN_PKTBUF_SIZE];
    size_t ressize = 0;
    ssize_t r;

   LogPrint("sn", "process_mgmt\n");

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "----------------\n");

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "uptime    %lu\n", (now - sss->start_time));

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "edges     %u\n",
        (unsigned int)peer_list_size(sss->edges));

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "errors    %u\n",
        (unsigned int)sss->stats.errors);

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "reg_sup   %u\n",
        (unsigned int)sss->stats.reg_super);

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "reg_nak   %u\n",
        (unsigned int)sss->stats.reg_super_nak);

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "fwd       %u\n",
        (unsigned int)sss->stats.fwd);

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "broadcast %u\n",
        (unsigned int)sss->stats.broadcast);

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "last fwd  %lu sec ago\n",
        (long unsigned int)(now - sss->stats.last_fwd));

    ressize += snprintf(resbuf + ressize, N2N_SN_PKTBUF_SIZE - ressize,
        "last reg  %lu sec ago\n",
        (long unsigned int)(now - sss->stats.last_reg_super));


    r = sendto(sss->mgmt_sock, resbuf, ressize, 0 /*flags*/,
        (struct sockaddr*)sender_sock, sizeof(struct sockaddr_in));

    if (r <= 0) {
        ++(sss->stats.errors);
        LogPrintf( "process_mgmt : sendto failed. %s\n", strerror(errno));
    }

    return 0;
}

/** Check if the specified community is allowed by the
 *  supernode configuration
 *  @return 0 = community not allowed, 1 = community allowed
 *
 */
static int allowed_n2n_community(n2n_common_t* cmn)
{
    if (allowed_communities != NULL) {
        struct n2n_allowed_communities* c;

        HASH_FIND_STR(allowed_communities, (const char*)cmn->community, c);
        return ((c == NULL) ? 0 : 1);
    } else {
        /* If no allowed community is defined, all communities are allowed */
    }

    return (1);
}

/** Load the list of allowed communities. Existing/previous ones will be removed
 *
 */
static int load_allowed_n2n_communities(char* path)
{
    char buffer[4096], *line;
    FILE* fd = fopen(path, "r");
    struct n2n_allowed_communities *s, *tmp;
    u_int32_t num_communities = 0;

    if (fd == NULL) {
        LogPrintf( "File %s not found\n", path);
        return -1;
    }

    HASH_ITER(hh, allowed_communities, s, tmp)
    free(s);

    while ((line = fgets(buffer, sizeof(buffer), fd)) != NULL) {
        int len = strlen(line);

        if ((len < 2) || line[0] == '#')
            continue;

        len--;
        while (len > 0) {
            if ((line[len] == '\n') || (line[len] == '\r')) {
                line[len] = '\0';
                len--;
            } else
                break;
        }

        s = (struct n2n_allowed_communities*)malloc(sizeof(struct n2n_allowed_communities));

        if (s != NULL) {
            strncpy((char*)s->community, line, N2N_COMMUNITY_SIZE);
            HASH_ADD_STR(allowed_communities, community, s);
            num_communities++;
            LogPrint("sn", "Added allowed community '%s' [total: %u]\n",
                (char*)s->community, num_communities);
        }
    }

    fclose(fd);

    LogPrint("sn", "Loaded %u communities from %s\n",
        num_communities, path);

    return (0);
}

/** Examine a datagram and determine what to do with it.
 *
 */
static int process_udp(n2n_sn_t* sss,
    const struct sockaddr_in* sender_sock,
    const uint8_t* udp_buf,
    size_t udp_size,
    time_t now)
{
    n2n_common_t cmn; /* common fields in the packet header */
    size_t rem;
    size_t idx;
    size_t msg_type;
    uint8_t from_supernode;
    macstr_t mac_buf;
    macstr_t mac_buf2;
    n2n_sock_str_t sockbuf;
    char buf[32];

   LogPrint("sn", "Processing incoming UDP packet [len: %lu][sender: %s:%u]\n",
        udp_size, intoa(ntohl(sender_sock->sin_addr.s_addr), buf, sizeof(buf)),
        ntohs(sender_sock->sin_port));

    /* Use decode_common() to determine the kind of packet then process it:
   *
   * REGISTER_SUPER adds an edge and generate a return REGISTER_SUPER_ACK
   *
   * REGISTER, REGISTER_ACK and PACKET messages are forwarded to their
   * destination edge. If the destination is not known then PACKETs are
   * broadcast.
   */

    rem = udp_size; /* Counts down bytes of packet to protect against buffer overruns. */
    idx = 0;        /* marches through packet header as parts are decoded. */
    if (decode_common(&cmn, udp_buf, &rem, &idx) < 0) {
        LogPrintf( "Failed to decode common section\n");
        return -1; /* failed to decode packet */
    }

    msg_type = cmn.pc; /* packet code */
    from_supernode = cmn.flags & N2N_FLAGS_FROM_SUPERNODE;

    if (cmn.ttl < 1) {
        LogPrintf( "Expired TTL\n");
        return 0; /* Don't process further */
    }

    --(cmn.ttl); /* The value copied into all forwarded packets. */

    if (msg_type == MSG_TYPE_PACKET) {
        /* PACKET from one edge to another edge via supernode. */

        /* pkt will be modified in place and recoded to an output of potentially
     * different size due to addition of the socket.*/
        n2n_PACKET_t pkt;
        n2n_common_t cmn2;
        uint8_t encbuf[N2N_SN_PKTBUF_SIZE];
        size_t encx = 0;
        int unicast;            /* non-zero if unicast */
        const uint8_t* rec_buf; /* either udp_buf or encbuf */


        sss->stats.last_fwd = now;
        decode_PACKET(&pkt, &cmn, udp_buf, &rem, &idx);

        unicast = (0 == is_multi_broadcast(pkt.dstMac));

       LogPrint("sn", "RX PACKET (%s) %s -> %s %s\n",
            (unicast ? "unicast" : "multicast"),
            macaddr_str(mac_buf, pkt.srcMac),
            macaddr_str(mac_buf2, pkt.dstMac),
            (from_supernode ? "from sn" : "local"));

        if (!from_supernode) {
            memcpy(&cmn2, &cmn, sizeof(n2n_common_t));

            /* We are going to add socket even if it was not there before */
            cmn2.flags |= N2N_FLAGS_SOCKET | N2N_FLAGS_FROM_SUPERNODE;

            pkt.sock.family = AF_INET;
            pkt.sock.port = ntohs(sender_sock->sin_port);
            memcpy(pkt.sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE);

            rec_buf = encbuf;

            /* Re-encode the header. */
            encode_PACKET(encbuf, &encx, &cmn2, &pkt);

            /* Copy the original payload unchanged */
            encode_buf(encbuf, &encx, (udp_buf + idx), (udp_size - idx));
        } else {
            /* Already from a supernode. Nothing to modify, just pass to
       * destination. */

           LogPrint("sn", "Rx PACKET fwd unmodified\n");

            rec_buf = udp_buf;
            encx = udp_size;
        }

        /* Common section to forward the final product. */
        if (unicast)
            try_forward(sss, &cmn, pkt.dstMac, rec_buf, encx);
        else
            try_broadcast(sss, &cmn, pkt.srcMac, rec_buf, encx);
    } /* MSG_TYPE_PACKET */
    else if (msg_type == MSG_TYPE_REGISTER) {
        /* Forwarding a REGISTER from one edge to the next */

        n2n_REGISTER_t reg;
        n2n_common_t cmn2;
        uint8_t encbuf[N2N_SN_PKTBUF_SIZE];
        size_t encx = 0;
        int unicast;            /* non-zero if unicast */
        const uint8_t* rec_buf; /* either udp_buf or encbuf */

        sss->stats.last_fwd = now;
        decode_REGISTER(&reg, &cmn, udp_buf, &rem, &idx);

        unicast = (0 == is_multi_broadcast(reg.dstMac));

        if (unicast) {
           LogPrint("sn", "Rx REGISTER %s -> %s %s\n",
                macaddr_str(mac_buf, reg.srcMac),
                macaddr_str(mac_buf2, reg.dstMac),
                ((cmn.flags & N2N_FLAGS_FROM_SUPERNODE) ? "from sn" : "local"));

            if (0 != (cmn.flags & N2N_FLAGS_FROM_SUPERNODE)) {
                memcpy(&cmn2, &cmn, sizeof(n2n_common_t));

                /* We are going to add socket even if it was not there before */
                cmn2.flags |= N2N_FLAGS_SOCKET | N2N_FLAGS_FROM_SUPERNODE;

                reg.sock.family = AF_INET;
                reg.sock.port = ntohs(sender_sock->sin_port);
                memcpy(reg.sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE);

                rec_buf = encbuf;

                /* Re-encode the header. */
                encode_REGISTER(encbuf, &encx, &cmn2, &reg);

                /* Copy the original payload unchanged */
                encode_buf(encbuf, &encx, (udp_buf + idx), (udp_size - idx));
            } else {
                /* Already from a supernode. Nothing to modify, just pass to
	 * destination. */

                rec_buf = udp_buf;
                encx = udp_size;
            }

            try_forward(sss, &cmn, reg.dstMac, rec_buf, encx); /* unicast only */
        } else
            LogPrintf( "Rx REGISTER with multicast destination\n");
    } else if (msg_type == MSG_TYPE_REGISTER_ACK)
       LogPrint("sn", "Rx REGISTER_ACK (NOT IMPLEMENTED) SHould not be via supernode\n");
    else if (msg_type == MSG_TYPE_REGISTER_SUPER) {
        n2n_REGISTER_SUPER_t reg;
        n2n_REGISTER_SUPER_ACK_t ack;
        n2n_common_t cmn2;
        uint8_t ackbuf[N2N_SN_PKTBUF_SIZE];
        size_t encx = 0;

        /* Edge requesting registration with us.  */
        sss->stats.last_reg_super = now;
        ++(sss->stats.reg_super);
        decode_REGISTER_SUPER(&reg, &cmn, udp_buf, &rem, &idx);

        /*
      Before we move any further, we need to check if the requested
      community is allowed by the supernode. In case it is not we do
      not report any message back to the edge to hide the supernode
      existance (better from the security standpoint)
    */
        if (allowed_n2n_community(&cmn)) {
            cmn2.ttl = N2N_DEFAULT_TTL;
            cmn2.pc = n2n_register_super_ack;
            cmn2.flags = N2N_FLAGS_SOCKET | N2N_FLAGS_FROM_SUPERNODE;
            memcpy(cmn2.community, cmn.community, sizeof(n2n_community_t));

            memcpy(&(ack.cookie), &(reg.cookie), sizeof(n2n_cookie_t));
            memcpy(ack.edgeMac, reg.edgeMac, sizeof(n2n_mac_t));
            ack.lifetime = reg_lifetime(sss);

            ack.sock.family = AF_INET;
            ack.sock.port = ntohs(sender_sock->sin_port);
            memcpy(ack.sock.addr.v4, &(sender_sock->sin_addr.s_addr), IPV4_SIZE);

            ack.num_sn = 0; /* No backup */
            memset(&(ack.sn_bak), 0, sizeof(n2n_sock_t));

           LogPrint("sn", "Rx REGISTER_SUPER for %s [%s]\n",
                macaddr_str(mac_buf, reg.edgeMac),
                sock_to_cstr(sockbuf, &(ack.sock)));

            update_edge(sss, reg.edgeMac, cmn.community, &(ack.sock), now);

            encode_REGISTER_SUPER_ACK(ackbuf, &encx, &cmn2, &ack);

            sendto(sss->sock, ackbuf, encx, 0,
                (struct sockaddr*)sender_sock, sizeof(struct sockaddr_in));

           LogPrint("sn", "Tx REGISTER_SUPER_ACK for %s [%s]\n",
                macaddr_str(mac_buf, reg.edgeMac),
                sock_to_cstr(sockbuf, &(ack.sock)));
        } else {
            LogPrint("sn", "Discarded registration: unallowed community '%s'\n",
                (char*)cmn.community);
        }
    }

    return 0;
}


static int run_loop(n2n_sn_t* sss);

/* *********************************************** */

static void dump_registrations(int signo)
{
    struct peer_info* list = sss_node.edges;
    char buf[32];
    time_t now = time(NULL);
    u_int num = 0;

    LogPrint("sn", "====================================\n");

    while (list != NULL) {
        if (list->sock.family == AF_INET)
            LogPrint("sn", "[id: %u][MAC: %s][edge: %u.%u.%u.%u:%u][community: %s][last seen: %u sec ago]\n",
                ++num, macaddr_str(buf, list->mac_addr),
                list->sock.addr.v4[0], list->sock.addr.v4[1], list->sock.addr.v4[2], list->sock.addr.v4[3],
                list->sock.port,
                (char*)list->community_name,
                now - list->last_seen);
        else
            LogPrint("sn", "[id: %u][MAC: %s][edge: IPv6:%u][community: %s][last seen: %u sec ago]\n",
                ++num, macaddr_str(buf, list->mac_addr), list->sock.port,
                (char*)list->community_name,
                now - list->last_seen);

        list = list->next;
    }

    LogPrint("sn", "====================================\n");
}

/* *************************************************** */

/** Main program entry point from kernel. */
void ThreadSnStart()
{
    RenameThread("massgrid-sn");

    LogPrintf("ThreadSnStart\n");

    init_sn(&sss_node);

    sss_node.sock = open_socket(sss_node.lport, 1 /*bind ANY*/);
    if (-1 == sss_node.sock) {
        LogPrintf("Failed to open main socket. %s\n", strerror(errno));
        exit(-2);
    } else {
        LogPrint("sn", "supernode is listening on UDP %u (main)\n", sss_node.lport);
    }

    sss_node.mgmt_sock = open_socket(N2N_SN_MGMT_PORT, 0 /* bind LOOPBACK */);
    if (-1 == sss_node.mgmt_sock) {
        LogPrintf("Failed to open management socket. %s\n", strerror(errno));
        exit(-2);
    } else
        LogPrint("sn", "supernode is listening on UDP %u (management)\n", N2N_SN_MGMT_PORT);

    LogPrintf("supernode started\n");

#ifndef WIN32
    signal(SIGHUP, dump_registrations);
#endif

    run_loop(&sss_node);
    LogPrintf("ThreadSnStop\n");
}


/** Long lived processing entry point. Split out from main to simply
 *  daemonisation on some platforms. */
static int run_loop(n2n_sn_t* sss)
{
    uint8_t pktbuf[N2N_SN_PKTBUF_SIZE];
    int keep_running = 1;

    sss->start_time = time(NULL);

    while (keep_running) {
        int rc;
        ssize_t bread;
        int max_sock;
        fd_set socket_mask;
        struct timeval wait_time;
        time_t now = 0;

        FD_ZERO(&socket_mask);
        max_sock = MAX(sss->sock, sss->mgmt_sock);

        FD_SET(sss->sock, &socket_mask);
        FD_SET(sss->mgmt_sock, &socket_mask);

        wait_time.tv_sec = 10;
        wait_time.tv_usec = 0;
        rc = select(max_sock + 1, &socket_mask, NULL, NULL, &wait_time);

        now = time(NULL);

        if (rc > 0) {
            if (FD_ISSET(sss->sock, &socket_mask)) {
                struct sockaddr_in sender_sock;
                socklen_t i;

                i = sizeof(sender_sock);
                bread = recvfrom(sss->sock, pktbuf, N2N_SN_PKTBUF_SIZE, 0 /*flags*/,
                    (struct sockaddr*)&sender_sock, (socklen_t*)&i);

                if ((bread < 0)
#ifdef WIN32
                    && (WSAGetLastError() != WSAECONNRESET)
#endif
                ) {
                    /* For UDP bread of zero just means no data (unlike TCP). */
                    /* The fd is no good now. Maybe we lost our interface. */
                    LogPrintf("recvfrom() failed %d errno %d (%s)\n", bread, errno, strerror(errno));
#ifdef WIN32
                    LogPrintf("WSAGetLastError(): %u\n", WSAGetLastError());
#endif
                    keep_running = 0;
                    break;
                }

                /* We have a datagram to process */
                if (bread > 0) {
                    /* And the datagram has data (not just a header) */
                    process_udp(sss, &sender_sock, pktbuf, bread, now);
                }
            }

            if (FD_ISSET(sss->mgmt_sock, &socket_mask)) {
                struct sockaddr_in sender_sock;
                size_t i;

                i = sizeof(sender_sock);
                bread = recvfrom(sss->mgmt_sock, pktbuf, N2N_SN_PKTBUF_SIZE, 0 /*flags*/,
                    (struct sockaddr*)&sender_sock, (socklen_t*)&i);

                if (bread <= 0) {
                    LogPrintf("recvfrom() failed %d errno %d (%s)\n", bread, errno, strerror(errno));
                    keep_running = 0;
                    break;
                }

                /* We have a datagram to process */
                process_mgmt(sss, &sender_sock, pktbuf, bread, now);
            }
        } else {
            LogPrint("sn", "timeout\n");
        }

        purge_expired_registrations(&(sss->edges));
        boost::this_thread::interruption_point();

    } /* while */

    deinit_sn(sss);

    return 0;
}
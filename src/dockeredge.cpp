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

#include "dockeredge.h"
#include "n2n/include/n2n.h"
#include <boost/thread/thread.hpp>
#ifdef WIN32
#include <sys/stat.h>
#endif

#define N2N_NETMASK_STR_SIZE    16 /* dotted decimal 12 numbers + 3 dots */
#define N2N_MACNAMSIZ           18 /* AA:BB:CC:DD:EE:FF + NULL*/
#define N2N_IF_MODE_SIZE        16 /* static | dhcp */

/* *************************************************** */

/** maximum length of command line arguments */
#define MAX_CMDLINE_BUFFER_LENGTH    4096

/** maximum length of a line in the configuration file */
#define MAX_CONFFILE_LINE_LENGTH     1024
std::function<void(bool)> fstart = nullptr;
boost::thread* thrd = NULL;
int     keep_on_running = 1;
std::string community;
std::string localip;
std::string snip;
/* ***************************************************** */

typedef struct {
  int     local_port;
  int     mgmt_port;
  char    tuntap_dev_name[N2N_IFNAMSIZ];
  char    ip_mode[N2N_IF_MODE_SIZE];
  char    ip_addr[N2N_NETMASK_STR_SIZE];
  char    netmask[N2N_NETMASK_STR_SIZE];
  int     mtu;
  int     got_s;
  char    device_mac[N2N_MACNAMSIZ];
  char *  encrypt_key;
#ifndef WIN32
  uid_t   userid;
  gid_t   groupid;
#endif
} edge_conf_t;

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
 *  ip_add and ip_mode are NULL terminated if modified.
 *
 *  return 0 on success and -1 on error
 */
static int scan_address(char * ip_addr, size_t addr_size,
			char * ip_mode, size_t mode_size,
			const char * s) {
  int retval = -1;
  char * p;

  if((NULL == s) || (NULL == ip_addr))
    {
      return -1;
    }

  memset(ip_addr, 0, addr_size);

  p = strpbrk(const_cast<char *>(s), ":");

  if(p)
    {
      /* colon is present */
      if(ip_mode)
        {
	  size_t end=0;

	  memset(ip_mode, 0, mode_size);
	  end = MIN(p-s, (ssize_t)(mode_size-1)); /* ensure NULL term */
	  strncpy(ip_mode, s, end);
	  strncpy(ip_addr, p+1, addr_size-1); /* ensure NULL term */
	  retval = 0;
        }
    }
  else
    {
      /* colon is not present */
      strncpy(ip_addr, s, addr_size);
    }

  return retval;
}

/* ************************************** */

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

/* ************************************** */

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

/* ************************************** */

/** Called from update_supernode_reg to periodically send gratuitous ARP
 *  broadcasts. */
static void send_grat_arps(n2n_edge_t * eee,) {
  char buffer[48];
  size_t len;

  LogPrint("edge", "Sending gratuitous ARP...\n");
  len = build_gratuitous_arp(buffer, sizeof(buffer));
  send_packet2net(eee, buffer, len);
  send_packet2net(eee, buffer, len); /* Two is better than one :-) */
}

#endif /* #if defined(DUMMY_ID_00001) */

/* ************************************** */

static void daemonize() {
#ifndef WIN32
  int childpid;

  LogPrint("edge", "Parent process is exiting (this is normal)\n");

  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP,  SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  if((childpid = fork()) < 0)
    LogPrintf( "Occurred while daemonizing (errno=%d)\n",
	       errno);
  else {
    if(!childpid) { /* child */
      int rc;

      //LogPrint("edge", "Bye bye: I'm becoming a daemon...");
      rc = chdir("/");
      if(rc != 0)
	LogPrintf( "Error while moving to / directory\n");

      setsid();  /* detach from the terminal */

      fclose(stdin);
      fclose(stdout);
      /* fclose(stderr); */

      /*
       * clear any inherited file mode creation mask
       */
      //umask(0);

      /*
       * Use line buffered stdout
       */
      /* setlinebuf (stdout); */
      setvbuf(stdout, (char *)NULL, _IOLBF, 0);
    } else /* father */
      exit(0);
  }
#endif
}

/* *************************************************** */

/** Entry point to program from kernel. */
int edgeStart() {
  keep_on_running = 1;
  int     rc;
  int     i;
  n2n_edge_t eee; /* single instance for this program */
  edge_conf_t ec;
  
  ec.local_port = 0 /* any port */;
  ec.mgmt_port = N2N_EDGE_MGMT_PORT; /* 5644 by default */
  snprintf(ec.tuntap_dev_name, sizeof(ec.tuntap_dev_name), "edge0");
  snprintf(ec.ip_mode, sizeof(ec.ip_mode), "static");
  snprintf(ec.netmask, sizeof(ec.netmask), "255.255.0.0");
  ec.ip_addr[0] = '\0';
  ec.device_mac[0] = '\0';
  ec.mtu = DEFAULT_MTU;
  ec.got_s = 0;        
  ec.encrypt_key = strdup("massgridn2n");
#ifndef WIN32
  ec.userid = 0; /* root is the only guaranteed ID */
  ec.groupid = 0; /* root is the only guaranteed ID */
#endif

  if(-1 == edge_init(&eee)) {
    LogPrintf( "Failed in edge_init\n");
    exit(1);
  }
  
  eee.allow_routing = 1;
  memset(eee.community_name, 0, N2N_COMMUNITY_SIZE);
  strncpy((char *)eee.community_name, community.c_str(), N2N_COMMUNITY_SIZE);
  scan_address(ec.ip_addr, N2N_NETMASK_STR_SIZE,
		   ec.ip_mode, N2N_IF_MODE_SIZE,
		   localip.c_str());
  strncpy((eee.sn_ip_array[eee.sn_num]), snip.c_str(), N2N_EDGE_SN_HOST_SIZE);
  LogPrint("edge","Adding supernode[%u] = %s\n", (unsigned int)eee.sn_num, (eee.sn_ip_array[eee.sn_num]));
  ++eee.sn_num;

#ifdef WIN32
  ec.tuntap_dev_name[0] = '\0';
#endif
  memset(&(eee.supernode), 0, sizeof(eee.supernode));
  eee.supernode.family = AF_INET;
  
  LogPrint("edge", "Starting n2n edge %s %s\n", n2n_sw_version, n2n_sw_buildDate);

  for (i=0; i<eee.sn_num; ++i)
    LogPrint("edge", "supernode %u => %s\n", i, (eee.sn_ip_array[i]));

  supernode2addr(&(eee.supernode), eee.sn_ip_array[eee.sn_idx]);

  if(!(
#ifdef __linux__
       (ec.tuntap_dev_name[0] != 0) &&
#endif
       (eee.community_name[0] != 0) &&
       (ec.ip_addr[0] != 0)
       ))
  
  if((NULL == ec.encrypt_key) && (0 == strlen(eee.keyschedule)))
    {
      LogPrintf("Encryption is disabled in edge.\n");
      
      eee.null_transop = 1;
    }
  
#ifndef WIN32
  /* If running suid root then we need to setuid before using the force. */
  setuid(0);
  /* setgid(0); */
#endif

  if(0 == strcmp("dhcp", ec.ip_mode)) {
    LogPrint("edge", "Dynamic IP address assignment enabled.\n");
    
    eee.dyn_ip_mode = 1;
  } else
    LogPrint("edge", "ip_mode='%s'\n", ec.ip_mode);    

  if(tuntap_open(&(eee.device), ec.tuntap_dev_name, ec.ip_mode, ec.ip_addr, ec.netmask, ec.device_mac, ec.mtu) < 0)
    return(-1);

#ifndef WIN32
  if((ec.userid != 0) || (ec.groupid != 0)) {
    LogPrint("edge", "Interface up. Dropping privileges to uid=%d, gid=%d\n",
	       (signed int)ec.userid, (signed int)ec.groupid);

    /* Finished with the need for root privileges. Drop to unprivileged user. */
    setreuid(ec.userid, ec.userid);
    setregid(ec.groupid, ec.groupid);
  }
#endif

  if(ec.local_port > 0)
    LogPrint("edge", "Binding to local port %d\n", (signed int)ec.local_port);

  if(ec.encrypt_key) {
#ifdef N2N_HAVE_AES
    if(edge_init_aes_psk(&eee, (uint8_t *)(ec.encrypt_key), strlen(ec.encrypt_key)) < 0) {
      fprintf(stderr, "Error: AES PSK setup failed.\n");
      return(-1);
    }
#endif
    
    if(edge_init_twofish_psk(&eee, (uint8_t *)(ec.encrypt_key), strlen(ec.encrypt_key)) < 0) {
      fprintf(stderr, "Error: twofish PSK setup failed.\n");
      return(-1);
    }
    
  } else if(strlen(eee.keyschedule) > 0) {
    if(edge_init_keyschedule(&eee) != 0) {
      fprintf(stderr, "Error: keyschedule setup failed.\n");
      return(-1);
    }
  }  
  /* else run in NULL mode */

  /* Populate the multicast group for local edge */
  eee.multicast_peer.family     = AF_INET;
  eee.multicast_peer.port       = N2N_MULTICAST_PORT;
  eee.multicast_peer.addr.v4[0] = 224; /* N2N_MULTICAST_GROUP */
  eee.multicast_peer.addr.v4[1] = 0;
  eee.multicast_peer.addr.v4[2] = 0;
  eee.multicast_peer.addr.v4[3] = 68;
      
  eee.udp_sock = open_socket(ec.local_port, 1 /* bind ANY */);
  if(eee.udp_sock < 0) {
    LogPrintf( "Failed to bind main UDP port %u\n", (signed int)ec.local_port);
    return(-1);
  }
  
  eee.udp_mgmt_sock = open_socket(ec.mgmt_port, 0 /* bind LOOPBACK */);  
  if(eee.udp_mgmt_sock < 0) {
    LogPrintf( "Failed to bind management UDP port %u\n", ec.mgmt_port);
    return(-1);
  }

  eee.udp_multicast_sock = open_socket(N2N_MULTICAST_PORT, 1 /* bind ANY */);
  if(eee.udp_multicast_sock < 0)
    return(-5);
  else {
    /* Bind eee.udp_multicast_sock to multicast group */
    struct ip_mreq mreq;
    u_int enable_reuse = 1;
    
    /* allow multiple sockets to use the same PORT number */
    setsockopt(eee.udp_multicast_sock, SOL_SOCKET, SO_REUSEADDR, &enable_reuse, sizeof(enable_reuse));
#ifndef WIN32 /* no SO_REUSEPORT in Windows */
    setsockopt(eee.udp_multicast_sock, SOL_SOCKET, SO_REUSEPORT, &enable_reuse, sizeof(enable_reuse));
#endif
    
    mreq.imr_multiaddr.s_addr = inet_addr(N2N_MULTICAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(eee.udp_multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
      LogPrintf( "Failed to bind to local multicast group %s:%u [errno %u]\n",
		 N2N_MULTICAST_GROUP, N2N_MULTICAST_PORT, errno);

#ifdef WIN32
      LogPrintf( "WSAGetLastError(): %u\n", WSAGetLastError());
#endif
      return(-6);
    }    
  }

  LogPrint("edge", "edge started\n");

  update_supernode_reg(&eee, time(NULL));
  if(fstart)
    fstart(true);
  int ret = run_edge_loop(&eee, &keep_on_running);
  if(fstart)
    fstart(false);
  return ret;
}

bool ThreadEdgeStart(std::string com,std::string localaddr,std::string snaddr,std::function<void(bool)>start){
    LogPrintf("ThreadEdgeStart\n");
    community = com;
    localip = localaddr;
    snip = snaddr;
    if(start)
      fstart = start;
    else
      fstart = NULL;
    if(thrd && !thrd->timed_join(boost::posix_time::seconds(1))){
        LogPrintf("ThreadEdgeStart existed thread,Please Stop first\n");
        return false;
    }
    thrd=new boost::thread(&edgeStart);
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
    keep_on_running = 0;
    thrd->join();
    delete thrd;
    thrd=NULL;
}

bool IsThreadRunning(){
    if(thrd && !thrd->timed_join(boost::posix_time::seconds(1))){
        LogPrintf("Edge thread is running\n");
        return true;
    }
    return false;
}
/* ************************************** */

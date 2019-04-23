// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERMAN_H
#define DOCKERMAN_H
#include <set>
#include "dockernode.h"
#include "dockerswarm.h"
#include "dockerservice.h"
#include "dockertask.h"
#include "dockerpriceconfig.h"
#include "net.h"
#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#endif
#ifdef WIN32
typedef unsigned long in_addr_t;
#endif
class CDockerMan;
extern CDockerMan dockerman;
static const char* strMethod[] = {
    "METHOD_NODES_LISTS",
    "METHOD_NODES_INSPECT",
    "METHOD_NODES_DELETE",
    "METHOD_NODES_UPDATE",
    "METHOD_SERVICES_LISTS",
    "METHOD_SERVICES_CREATE",
    "METHOD_SERVICES_INSPECT",
    "METHOD_SERVICES_DELETE",
    "METHOD_SERVICES_UPDATE",
    "METHOD_SERVICES_LOGS",
    "METHOD_TASKS_LISTS",
    "METHOD_TASKS_INSPECT",
    "METHOD_SWARM_INSPECT",
    "METHOD_INFO",
    "METHOD_VERSION",
    "MINER_SERVICES_CREATE",
    "MINER_SERVICES_DELETE"};
enum Method{
    METHOD_NODES_LISTS,
    METHOD_NODES_INSPECT,
    METHOD_NODES_DELETE,
    METHOD_NODES_UPDATE,

    METHOD_SERVICES_LISTS,
    METHOD_SERVICES_CREATE,
    METHOD_SERVICES_INSPECT,
    METHOD_SERVICES_DELETE,
    METHOD_SERVICES_UPDATE,
    METHOD_SERVICES_LOGS,

    METHOD_TASKS_LISTS,
    METHOD_TASKS_INSPECT,

    METHOD_SWARM_INSPECT,
    METHOD_INFO,
    METHOD_VERSION,
    MINER_SERVICES_CREATE,
    MINER_SERVICES_DELETE
};
enum HttpType{
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_DELETE
};

/*
* assign ip address to docker engine
* n2n default netmask 0xff000000,
* Birthday Paradox client n=1000 d=255x255x254 
* Probability is 0.0298193952119
*/
class IpSet{
    set<in_addr_t> ip_set{};
    const in_addr_t ip_start = htonl(inet_addr("10.0.0.1"));   //10.0.0.1
    const in_addr_t ip_end = htonl(inet_addr("10.0.255.254"));     //10.1.255.254
    const in_addr_t netmask = htonl(inet_addr("255.0.0.0"));    //255.0.0.0
public:
    std::string GetFreeIP();
    std::string GetNetMask();
    bool IsFree(std::string str);
    bool IsVaild(std::string s);

    void Insert(std::string str);
    void Erase(std::string str);
    void Clear(){ip_set.clear();}
};

class CDockerMan{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
private:
    //docker connection ip address and port
    const char* address = "localhost";
    uint32_t apiPort = 2375;

    Swarm swarm;
    std::map<std::string,Node> mapDockerNodeLists;
    std::map<std::string,Service> mapDockerServiceLists;
    map<Item,Value_price> priceList;
public:
    union docker_Version version;
    IpSet serviceIpList;
    /*
    * get all running services by pubkey
    */
    map<std::string ,Service> GetServiceFromPubkey(CPubKey pubkey);
    /*
    * Push Message to docker api,
    * isClearService default to clear all local caches
    */
    bool PushMessage(Method mtd,std::string id,std::string requestData,bool isClearService = true);
    bool ProcessMessage(Method mtd,std::string url,int ret,std::string responseData,bool isClearService = true);
    
    // update all local caches;
    bool Update(bool isClear=true);
    bool UpdateSwarmAndNodeList();
    bool UpdateServicesList();
    bool UpdateService(std::string serviceid);
    void UpdateIPfromServicelist(std::map<std::string,Service>& map);
    map<Item,Value_price> GetPriceListFromNodelist();
    void UpdatePriceListFromNodelist();

    uint64_t GetDockerNodeCount();
    // Get all activity node sum
    uint64_t GetDockerNodeActiveCount();   
    uint64_t GetDockerServiceCount();
    uint64_t GetDockerTaskCount(); 
    void GetVersionAndJoinToken();
    bool GetItemFromNodeID(std::string nodeid,Item &item);
    // Get service list which not be rented
    set<std::string> GetFreeNodeFromService();
    // Get a nodeid by such a item
    std::string GetNodeFromItem(Item item);
    std::string GetMasterIp();


    std::string GetSwarmJoinToken(){
        return swarm.joinWorkerTokens + " " + swarm.ip_port;
    }
    bool GetServiceidFromNode(std::string nodeid,std::string &serviceid){
        LOCK(cs);
        for(auto it = mapDockerServiceLists.begin();it != mapDockerServiceLists.end();++it){
            auto mapTask= it->second.mapDockerTaskLists.begin();
            if( mapTask != it->second.mapDockerTaskLists.end() && mapTask->second.nodeID == nodeid){
                serviceid = it->second.ID;
                return true;
            }
        }
        return false;
    }
    bool GetServiceFromList(std::string serviceid,Service& service){
        LOCK(cs);
        if(!mapDockerServiceLists.count(serviceid))
            return false;
            service = mapDockerServiceLists[serviceid];
        return true;
    }
    bool GetServiceFromTxId(uint256 txid,Service& service){
        LOCK(cs);
        for(auto it = mapDockerServiceLists.begin();it != mapDockerServiceLists.end();++it){
            if(it->second.txid == txid){
                service = it->second;
                return true;
            }
        }
        return false;
    }
    bool GetSerivceFromServiceID(std::string serviceid,Service& service){
        LOCK(cs);
        if(!mapDockerServiceLists.count(serviceid))
            return false;
        service = mapDockerServiceLists[serviceid];
        return true;
    }
    bool GetNodeFromList(std::string nodeid,Node& node){
        LOCK(cs);
        if(!mapDockerNodeLists.count(nodeid))
            return false;
            node = mapDockerNodeLists[nodeid];
        return true;
    }
    void SetPort(uint32_t p){apiPort = p;}
    uint32_t GetPort(){return apiPort;}

    bool IsExistSerivce(uint256 txid){
        LOCK(cs);
        for(auto it = mapDockerServiceLists.begin();it != mapDockerServiceLists.end();++it){
            if(it->second.txid == txid)
                return true;
        }
        return false;
    }
    bool IsExistSerivceFromID(std::string serverid){
        LOCK(cs);
        if(!mapDockerServiceLists.count(serverid))
            return false;
        return true;
    }
};
#endif //DOCKERMAN_H

// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERMAN_H
#define DOCKERMAN_H
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
#include <set>
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
    "METHOD_VERSION"};
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
    METHOD_VERSION
};
enum HttpType{
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_DELETE
};
// class CollectUsefulNode{
// public:
//     map<std::string,int> usefulEngine{};
//     void IncreaseNode(Node& node){
//         string gputype = node.engine.labels["gputype"];
//         map[node.engine.labels["gputype"]+"_"+node.engine.labels["gpucount"]] += 1;
//     }
//     void DecreaseNode(Node& node){
//         string gputype = node.engine.labels["gputype"];
//         if(map[node.engine.labels["gputype"]+"_"+node.engine.labels["gpucount"]] > 0)
//             map[node.engine.labels["gputype"]+"_"+node.engine.labels["gpucount"]] -= 1;
//     }
// };
class IpSet{

    set<in_addr_t> ip_set{};
public:
    static const int MAX_SIZE = 255;
    const char* ip_start = "10.1."; //10.1.1.1

    std::string GetFreeIP();
    bool IsFree(std::string str);
    void Erase(std::string str);
    bool IsVaild(std::string s);
    void Clear(){
        ip_set.clear();
    }
    void Insert(std::string str);
};
class CDockerMan{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
private:
    const char* address = "localhost";
    uint32_t apiPort = 2375;

    Swarm swarm;
    std::map<std::string,Node> mapDockerNodeLists;
    std::map<std::string,Service> mapDockerServiceLists;
public:
    IpSet serviceIp;
    const uint32_t n2nServerPort = 8999;
    union docker_Version version;
    map<std::string ,Service> GetServiceFromPubkey(CPubKey pubkey){
        map<std::string ,Service> map{};
        for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
            if(it->second.customer == pubkey.ToString().substr(0,65)){
                map.insert(*it);
            }
        }

    }
    bool ProcessMessage(Method mtd,std::string url,int ret,std::string responsedata,bool isClearService = true);
    bool PushMessage(Method mtd,std::string id,std::string pushdata,bool isClearService = true);
    bool Update(); //update all data;

    bool UpdateSwarmAndNodeList();
    bool UpdateServicesList();
    bool UpdateService(std::string serviceid);
    std::string GetSwarmJoinToken(){
        return swarm.joinWorkerTokens + " " + swarm.ip_port;
    }
    
    std::string GetMasterIp();
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
            return false;
        }
    }
    bool GetNodeFromList(std::string nodeid,Node& node){
        LOCK(cs);
        if(!mapDockerNodeLists.count(nodeid))
            return false;
            node = mapDockerNodeLists[nodeid];
        return true;
    }
    uint64_t GetDockerNodeCount();
    uint64_t GetDockerNodeActiveCount();
    uint64_t GetDockerServiceCount();
    uint64_t GetDockerTaskCount(); 
    void SetPort(uint32_t p){apiPort = p;}
    uint32_t GetPort(){return apiPort;}
    void GetVersionAndJoinToken();
    void UpdateIPfromServicelist(std::map<std::string,Service>& map);
    map<Item,std::pair<CAmount,int>> GetPriceListFromNodelist();
};
#endif //DOCKERMAN_H

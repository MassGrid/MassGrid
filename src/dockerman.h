// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERMAN_H
#define DOCKERMAN_H
#include "dockernode.h"
#include "dockerswarm.h"
#include "dockerservice.h"
#include "dockertask.h"
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
class CDockerMan{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
private:
    const char* address = "localhost";
    uint32_t apiPort = 2375;
    uint32_t swarmPort = 2377;
    bool isExistIP[255][255]{};
public:

    const uint32_t n2nServerPort = 8999;
    union docker_Version version;
    std::map<std::string,Node> mapDockerNodeLists;
    std::map<std::string,Service> mapDockerServiceLists;
    Swarm swarm;
    std::string JoinToken;
    std::string managerAddr;    //no port


    bool ProcessMessage(Method mtd,std::string url,int ret,std::string responsedata);
    bool PushMessage(Method mtd,std::string id,std::string pushdata);
    bool Update(); //update all data;

    bool UpdateSwarmAndNodeList();
    bool UpdateServicesList();
    bool UpdateService(std::string serviceid);

    uint64_t GetDockerNodeCount();
    uint64_t GetDockerNodeActiveCount();
    uint64_t GetDockerServiceCount();
    uint64_t GetDockerTaskCount();    
    void SetPort(uint32_t p){apiPort = p;}
    uint32_t GetPort(){return apiPort;}
    void GetVersionAndJoinToken();
    std::string GetFreeIP();
    void SetIPBook(std::string ip,bool isused);
    bool IsFreeIP(std::string ip);
    void UpdateIPfromServicelist();
};

#endif //DOCKERMAN_H

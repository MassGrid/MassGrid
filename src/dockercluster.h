// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERCLUSTER_H
#define DOCKERCLUSTER_H

#include <vector>
#include "net.h"
#include "key.h"
#include "base58.h"
#include "primitives/transaction.h"
#include "dockertask.h"
#include "prizes_node.h"
#include "prizes_service.h"
#include "dockerserverman.h"
#define DOCKER_CPU_UNIT 1000000000
#define DOCKER_MEMORY_UNIT 1000000000
class Cluster;
class CPubKey;
extern Cluster dockercluster;
class Cluster{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    ResponseMachines machines{};
    DockerServiceInfo vecServiceInfo{};
    CService connectDockerAddr{};
    CNode* connectNode = nullptr;

    CPubKey DefaultPubkey{};
    void setDefaultPubkey(CPubKey pubkey);
    CMassGridAddress DefaultAddress{};

    bool SetConnectDockerAddress(std::string address_port);
    bool ProcessDockernodeConnections();
    bool ProcessDockernodeDisconnections(const std::string& strNode);
    
    void AskForDNData();
    void AskForService(COutPoint);
    void AskForServices(int64_t start,int64_t count,bool full);
    bool CreateAndSendSeriveSpec(DockerCreateService sspec);     //send message
    bool UpdateAndSendSeriveSpec(DockerUpdateService sspec);     //send message
    bool DeleteAndSendServiceSpec(DockerDeleteService delServic);

    //get dockertransaction
    bool SetConnectDockerAddr(std::string address_port);

    void saveServiceData(ServiceInfo serviceInfo);
    void saveReletServiceData(const std::string& serviceID,DockerUpdateService sspec);

};
#endif  //DOCKERCLUSTER_H
#ifndef DOCKERCLUSTER_H
#define DOCKERCLUSTER_H

#include <vector>
#include "net.h"
#include "key.h"
#include "base58.h"
#include "primitives/transaction.h"
#include "dockerservice.h"
#include "dockertask.h"
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
        DockerGetData dndata;
        DockerTransData dtdata;
        CService connectDockerAddr{};
        CNode* connectNode = nullptr;

        CPubKey DefaultPubkey{};
        void setDefaultPubkey(CPubKey pubkey);
        CMassGridAddress DefaultAddress{};

        bool SetConnectDockerAddress(std::string address_port);
        bool ProcessDockernodeConnections();
        bool ProcessDockernodeDisconnections(const std::string& strNode);
        

        void AskForDNData();
        bool CreateAndSendSeriveSpec(DockerCreateService sspec);     //send message
        bool DeleteAndSendServiceSpec(DockerDeleteService delServic);

        //get dockertransaction
        bool SetConnectDockerAddr(std::string address_port);
        void AskForTransData(std::string txid);

};
#endif  //DOCKERCLUSTER_H
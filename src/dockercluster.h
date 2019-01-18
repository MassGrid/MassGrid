#ifndef DOCKERCLUSTER_H
#define DOCKERCLUSTER_H

#include "net.h"
#include "key.h"
#include <vector>
#include <dockerservice.h>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"
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
        std::map<std::string,Service> mapDockerServiceLists;
        int64_t sigTime{};  //update time
        std::string n2nLocalIp{};
        CService connectDockerAddr{};
        CNode* connectNode = nullptr;
        // CTxIn masternodeId{};

        CPubKey DefaultPubkey{};
        void setDefaultPubkey(CPubKey pubkey);
        CMassGridAddress DefaultAddress{};

        bool SetConnectDockerAddress(std::string address_port);
        bool ProcessDockernodeConnections();
        

        void AskForDNData();
        bool CreateAndSendSeriveSpec(DockerCreateService sspec);     //send message
        bool UpdateAndSendSeriveSpec(DockerUpdateService sspec);    //send message
        // bool CheckAndUpdate(DockerUpdateService sspec);      //update infomation after check from receive

};
#endif  //DOCKERCLUSTER_H
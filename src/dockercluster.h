#ifndef DOCKERCLUSTER_H
#define DOCKERCLUSTER_H

#include "net.h"
#include "key.h"
#include <vector>
#include <dockerservice.h>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"
class Cluster;
extern Cluster dockercluster;
class Cluster{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
        std::map<std::string,Service> mapDockerServiceLists;
        int64_t sigTime{};  //update time

        CService connectDockerAddr{};
        CNode* connectNode = nullptr;
        // CTxIn masternodeId{};

        CPubKey DefaultPubkey{};
        CMassGridAddress DefaultAddress{};


        bool SetConnectDockerAddress(std::string address_port);
        bool ProcessDockernodeConnections();


        void AskForDNData();
        bool Check();   //send to server must be check;
        bool CreateAndSendSeriveSpec(DockerCreateService sspec);     //send message
        bool UpdateAndSendSeriveSpec(DockerUpdateService sspec);    //send message
        bool CheckAndUpdate();      //update infomation after check from receive

};
#endif  //DOCKERCLUSTER_H
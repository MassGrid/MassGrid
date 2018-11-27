#ifndef DOCKERCLUSTER_H
#define DOCKERCLUSTER_H

#include "net.h"
#include "key.h"
#include <vector>
#include <dockerservice.h>
#include <dockertask.h>

#include "primitives/transaction.h"
class CDockerClusterman;
extern CDockerClusterman dockerClusterman;

#define DNDATAVERSION 10030
#define DNCREATEVERSION 10030
class DockerGetData{
public:
    uint64_t version = DNDATAVERSION;
    CPubKey pubKeyClusterAddress{};
    CTxIn cin{};
    int64_t sigTime{}; //dkct message times
    std::map<std::string,Service> mapDockerServiceLists;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(cin);
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
        READWRITE(mapDockerServiceLists);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << cin;
        ss << version;
        ss << pubKeyClusterAddress;
        return ss.GetHash();
    }
};

class DockerDeploydata{
    uint64_t version = DNCREATEVERSION;
    CPubKey pubKeyClusterAddress{};
    CTxIn cin{};
    int64_t sigTime{};
    Service service{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(cin);
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
        READWRITE(service);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << cin;
        ss << version;
        ss << pubKeyClusterAddress;
        return ss.GetHash();
    }
};
class CDockerClusterman{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    uint64_t version{};
    std::string selectDockernode{};
    DockerGetData dndata;


    CNode* ProcessDockernodeConnections(CConnman& connman);

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    void AskForDNData(CNode* pnode, CConnman& connman);
};
#endif  //DOCKERCLUSTER_H
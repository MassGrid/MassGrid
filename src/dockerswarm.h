#ifndef __DOCKERSWARM__
#define __DOCKERSWARM__
#include "dockerbase.h"
class Swarm :public DockerBase{
public:
    // spec
    std::string joinWorkerTokens;
    std::string joinManagerTokens;

public:
    Swarm() = default;
    Swarm(std::string id,int idx ,uint64_t createdTime ,uint64_t updateTime,
    std::string mjoinWorkerTokens,
    std::string mjoinManagerTokens,
    int version=DEFAULT_CTASK_API_VERSION):joinWorkerTokens(mjoinWorkerTokens),
    joinManagerTokens(mjoinManagerTokens),DockerBase(id,idx,createdTime,updateTime,version)
    {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(index);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(nProtocolVersion);
    }

    Swarm(const Swarm& from){
        ID=from.ID;
        index=from.index;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        joinWorkerTokens=from.joinWorkerTokens;
        joinManagerTokens=from.joinManagerTokens;
    }

    Swarm& operator = (Swarm const& from){
        ID=from.ID;
        index=from.index;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        joinWorkerTokens=from.joinWorkerTokens;
        joinManagerTokens=from.joinManagerTokens;
        return *this;
    }

    void Update();
    std::string ToString();
};

void dockerswarm(const string& swarmData,std::vector<Swarm> &swarms);
Swarm *DockerSwarmJson(const UniValue& data);
#endif //__DOCKERSERVICE__

#ifndef DOCKERSWARM_H
#define DOCKERSWARM_H
#include "dockerbase.h"
class Swarm :public DockerBase{
    static bool DockerSwarmJson(const UniValue& data, Swarm& swarm);
public:
    static void DockerSwarm(const string& swarmData,Swarm &swarms);

    // spec
    std::string joinWorkerTokens;
    std::string joinManagerTokens;

public:
    Swarm() = default;
    Swarm(std::string id,Config::Version version ,uint64_t createdTime ,uint64_t updateTime,
    std::string mjoinWorkerTokens,
    std::string mjoinManagerTokens,
    int protocolVersion=DEFAULT_CTASK_API_VERSION):joinWorkerTokens(mjoinWorkerTokens),
    joinManagerTokens(mjoinManagerTokens),DockerBase(id,version,createdTime,updateTime,protocolVersion)
    {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(version);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(nProtocolVersion);
    }
    Swarm(const Swarm& from){
        ID=from.ID;
        version=from.version;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        joinWorkerTokens=from.joinWorkerTokens;
        joinManagerTokens=from.joinManagerTokens;
    }

    Swarm& operator = (Swarm const& from){
        ID=from.ID;
        version=from.version;
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

#endif //__DOCKERSWARM__
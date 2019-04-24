// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
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
    std::string ip_port;

public:

    Swarm() = default;
    Swarm(std::string id,Config::Version version ,uint64_t createdTime ,uint64_t updateTime,
    std::string mjoinWorkerTokens,
    std::string mjoinManagerTokens,
    int protocolVersion=DEFAULT_CDOCKER_API_VERSION):joinWorkerTokens(mjoinWorkerTokens),
    joinManagerTokens(mjoinManagerTokens),DockerBase(id,version,createdTime,updateTime,protocolVersion)
    {}
    std::string GetJoin_token(){
        return joinWorkerTokens + " " + ip_port;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(version);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(nProtocolVersion);
        READWRITE(joinWorkerTokens);
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
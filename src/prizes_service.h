// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_SERVICE_H
#define PRIZES_SERVICE_H
#include "prizes_service_create.h"
#include "prizes_service_update.h"
#include "prizes_service_delete.h"
#include "prizes_order.h"
#include "dockertask.h"
class DockerGetService {
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    int64_t sigTime{};

    COutPoint OutPoint{};
    CPubKey pubKeyClusterAddress{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(sigTime);
        READWRITE(version);
        READWRITE(OutPoint);
        READWRITE(pubKeyClusterAddress);
    }
};
struct ServiceInfo {
    std::string ServiceID{};
    int64_t CreatedAt{};
    int64_t DeleteAt{};
    int64_t NextCheckTime{};
    std::vector<ServiceOrder> Order{};
    ServiceCreate CreateSpec{};
    std::vector<ServiceUpdate> UpdateSpec{};
    std::string State{};
    std::vector<Task> TaskInfo{};
    UniValue jsonUniValue{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(ServiceID);
        READWRITE(CreatedAt);
        READWRITE(DeleteAt);
        READWRITE(NextCheckTime);
        READWRITE(Order);
        READWRITE(CreateSpec);
        READWRITE(UpdateSpec);
        READWRITE(State);
        READWRITE(TaskInfo);
    }

    static void DecodeFromJson(const UniValue& data, ServiceInfo& serviceInfo);
};

class DockerServiceInfo{
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    int64_t sigTime = GetAdjustedTime();
    std::map<std::string,ServiceInfo> servicesInfo{};
    std::string err{};
    std::string msg{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(sigTime);
        READWRITE(version);
        READWRITE(servicesInfo);
        READWRITE(err);
        READWRITE(msg);
    }
};
#endif
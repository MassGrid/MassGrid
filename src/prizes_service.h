// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_SERVICE_H
#define PRIZES_SERVICE_H
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "key.h"
#include "sync.h"
#include "util.h"
#include "utilstrencodings.h"
#include <algorithm>
#include <cstring>
#include <univalue.h>

#include "dockertask.h"
#include "prizes_node.h"
#include "prizes_order.h"
#include "prizes_statement.h"

struct ClusterServiceCreate_t {
    std::string ServiceName{};
    std::string Image{};
    std::string SSHPubkey{};
    CPubKey pubKeyClusterAddress{};
    COutPoint OutPoint{};
    HardWare hardware{};
    std::map<std::string, std::string> ENV{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(ServiceName);
        READWRITE(Image);
        READWRITE(SSHPubkey);
        READWRITE(pubKeyClusterAddress);
        READWRITE(OutPoint);
        READWRITE(hardware);
        READWRITE(ENV);
    }

    std::string ToString();
};
struct ServiceCreate : public ClusterServiceCreate_t {
    CAmount Amount{};
    CAmount ServicePrice{};
    std::string MasterNodeN2NAddr{};
    std::string Drawee{};
    std::string MasterNodeFeeAddress{};
    std::string DevFeeAddress{};
    int64_t MasterNodeFeeRate{};
    int64_t DevFeeRate{};


    static void DecodeFromJson(const UniValue& data, ServiceCreate& serviceCreate);
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(ServiceName);
        READWRITE(Image);
        READWRITE(SSHPubkey);
        READWRITE(pubKeyClusterAddress);
        READWRITE(OutPoint);
        READWRITE(hardware);
        READWRITE(ENV);
        READWRITE(Amount);
        READWRITE(ServicePrice);
        READWRITE(MasterNodeN2NAddr);
        READWRITE(Drawee);
        READWRITE(MasterNodeFeeAddress);
        READWRITE(DevFeeAddress);
        READWRITE(MasterNodeFeeRate);
        READWRITE(DevFeeRate);
    }
    static UniValue ToJson(ServiceCreate& serviceCreate);
};

class DockerCreateService
{
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    int64_t sigTime{};
    ClusterServiceCreate_t clusterServiceCreate{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vchSig);
        READWRITE(sigTime);
        READWRITE(version);
        READWRITE(clusterServiceCreate);
    }
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);
};
struct ClusterServiceUpdate_t {
    COutPoint CrerateOutPoint{};
    CPubKey pubKeyClusterAddress{};
    COutPoint OutPoint{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(CrerateOutPoint);
        READWRITE(pubKeyClusterAddress);
        READWRITE(OutPoint);
    }
    std::string ToString();
};
struct ServiceUpdate : public ClusterServiceUpdate_t {
    std::string ServiceID{};
    CAmount Amount{};
    CAmount ServicePrice{};
    std::string Drawee{};
    std::string MasterNodeFeeAddress{};
    std::string DevFeeAddress{};
    int64_t MasterNodeFeeRate{};
    int64_t DevFeeRate{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(CrerateOutPoint);
        READWRITE(pubKeyClusterAddress);
        READWRITE(OutPoint);
        READWRITE(ServiceID);
        READWRITE(Amount);
        READWRITE(ServicePrice);
        READWRITE(Drawee);
        READWRITE(MasterNodeFeeAddress);
        READWRITE(DevFeeAddress);
        READWRITE(MasterNodeFeeRate);
        READWRITE(DevFeeRate);
    }

    static void DecodeFromJson(const UniValue& data, ServiceUpdate& serviceUpdate);
    static UniValue ToJson(ServiceUpdate& serviceUpdate);
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


class DockerUpdateService {
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    int64_t sigTime{};
    ClusterServiceUpdate_t clusterServiceUpdate{};
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vchSig);
        READWRITE(sigTime);
        READWRITE(version);
        READWRITE(clusterServiceUpdate);
    }
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);
};
class DockerDeleteService
{
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    int64_t sigTime{};
    std::string ServiceID{};
    COutPoint CrerateOutPoint{};
    CPubKey pubKeyClusterAddress{};
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vchSig);
        READWRITE(sigTime);
        READWRITE(version);
        READWRITE(ServiceID);
        READWRITE(CrerateOutPoint);
        READWRITE(pubKeyClusterAddress);
    }
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);
};
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
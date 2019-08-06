// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_SERVICE_CREATE_H
#define PRIZES_SERVICE_CREATE_H

#include "dockerbase.h"
#include "prizes_node.h"
#include "primitives/transaction.h"
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
#endif
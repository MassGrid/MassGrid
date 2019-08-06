// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_SERVICE_UPDATE_H
#define PRIZES_SERVICE_UPDATE_H

#include "dockerbase.h"

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
#endif
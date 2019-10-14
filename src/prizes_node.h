// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_NODE_H
#define PRIZES_NODE_H
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <ostream>
#include <sstream>

#include "key.h"
#include "sync.h"
#include "util.h"
#include "utilstrencodings.h"
#include <algorithm>
#include <cstring>
#include <univalue.h>
#include "prizes_prices.h"
#include "timedata.h"
#define DOCKERREQUEST_API_VERSION 10071
struct HardWare {
    std::string CPUType{};
    int CPUThread{};
    std::string MemoryType{};
    int MemoryCount{};
    std::string GPUType{};
    int GPUCount{};
    std::string PersistentStore{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(CPUType);
        READWRITE(CPUThread);
        READWRITE(MemoryType);
        READWRITE(MemoryCount);
        READWRITE(GPUType);
        READWRITE(GPUCount);
        READWRITE(PersistentStore);
    }
    static void DecodeFromJson(const UniValue& data, HardWare& hardware);
    static UniValue ToJson(HardWare& hardware);
    std::string ToString();
};

struct NodeInfo {
    std::string NodeID{};
    std::string NodeState{};
    std::map<std::string, std::string> Labels{};
    std::string ReachAddress{};
    HardWare hardware{};
    bool OnWorking{};
    static void DecodeFromJson(const UniValue& data, NodeInfo& node);
};

struct NodeListStatistics {
    std::string Token{};
    int TotalCount{};
    int AvailabilityCount{};
    int UsableCount{};
    std::vector<NodeInfo> list{};
    static void DecodeFromJson(const UniValue& data, NodeListStatistics& nodelist);
};
struct ResponseMachines {
    uint64_t version = DOCKERREQUEST_API_VERSION;
    int64_t sigTime{};
    std::string err{};
    std::string Token{};
    int TotalCount{};
    int AvailabilityCount{};
    int UsableCount{};
    std::map<Item, Value_price> items{};
    std::string masternodeAddress{};
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(version);
        READWRITE(sigTime);
        READWRITE(err);
        READWRITE(items);
        READWRITE(masternodeAddress);
        READWRITE(AvailabilityCount);
        READWRITE(UsableCount);
        READWRITE(TotalCount);
        READWRITE(Token);
    }
};

void ParseLabels(const UniValue& data, std::map<std::string, std::string>& labels);
#endif //PRIZES_NODE_H
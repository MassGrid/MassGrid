// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_SERVICE_DELETE_H
#define PRIZES_SERVICE_DELETE_H

#include "dockerbase.h"

class DockerDeleteService
{
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    int64_t sigTime{};
    COutPoint CrerateOutPoint{};
    CPubKey pubKeyClusterAddress{};
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vchSig);
        READWRITE(sigTime);
        READWRITE(version);
        READWRITE(CrerateOutPoint);
        READWRITE(pubKeyClusterAddress);
    }
    std::string ToString();
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);
};
#endif
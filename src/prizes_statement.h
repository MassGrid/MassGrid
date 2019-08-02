// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_STATEMENT_H
#define PRIZES_STATEMENT_H
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
#include "base58.h"
#include "dockerbase.h"
struct PrizeStatement {
    std::string StatementID{};
    int64_t CreatedAt{};
    int64_t StatementStartAt{};
    int64_t StatementEndAt{};
	CAmount TotalAmount{};
	int64_t MasterNodeFeeRate{};
	int64_t DevFeeRate{};
	std::string MasterNodeFeeAddress{};
	std::string DevFeeAddress{};
	std::string StatementTransaction{};

    static void DecodeFromJson(const UniValue& data, PrizeStatement& statement);

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(StatementID);
        READWRITE(CreatedAt);
        READWRITE(StatementStartAt);
        READWRITE(StatementEndAt);
        READWRITE(TotalAmount);
        READWRITE(MasterNodeFeeRate);
        READWRITE(DevFeeRate);
        READWRITE(MasterNodeFeeAddress);
        READWRITE(DevFeeAddress);
        READWRITE(StatementTransaction);
    }
};


#endif //PRIZES_STATEMENT_H
// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_ORDER_H
#define PRIZES_ORDER_H
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "key.h"
#include "prizes_statement.h"
#include "sync.h"
#include "util.h"
#include "utilstrencodings.h"
#include <algorithm>
#include <cstring>
#include <univalue.h>

struct RefundPayment  {
	std::string RefundID{};
    int64_t CreatedAt{};
    CAmount TotalAmount{};
    std::string RefundTransaction{};
    static void DecodeFromJson(const UniValue& data, RefundPayment& refund);
    static UniValue ToJson(RefundPayment& refund);

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(RefundID);
        READWRITE(CreatedAt);
        READWRITE(TotalAmount);
        READWRITE(RefundTransaction);
    }
};
struct ServiceOrder {
    std::string OriderID{};
    COutPoint OutPoint{};
    int64_t CreatedAt{};
    std::string OrderState{};
    CAmount ServicePrice{};
    int64_t MasterNodeFeeRate{};
	int64_t DevFeeRate{};
	std::string MasterNodeFeeAddress{};
	std::string DevFeeAddress{};
    std::string Drawee{};
    CAmount Balance{};
    CAmount PayAmount{};
    int64_t RemainingTimeDuration{};
	int64_t TotalTimeDuration{};
    int64_t LastStatementTime{};
    std::vector<PrizeStatement> Statement{};

    static void DecodeFromJson(const UniValue& data, ServiceOrder& order);
    static UniValue ToJson(ServiceOrder& serviceOrder);

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(OriderID);
        READWRITE(OutPoint);
        READWRITE(CreatedAt);
        READWRITE(OrderState);
        READWRITE(ServicePrice);
        READWRITE(Drawee);
        READWRITE(MasterNodeFeeRate);
        READWRITE(DevFeeRate);
        READWRITE(MasterNodeFeeAddress);
        READWRITE(DevFeeAddress);
        READWRITE(Balance);
        READWRITE(PayAmount);
        READWRITE(RemainingTimeDuration);
        READWRITE(TotalTimeDuration);
        READWRITE(LastStatementTime);
        READWRITE(Statement);
    }
};
struct RefundInfo {
    std::vector<RefundPayment> RefundPay{};
    PrizeStatement Statement{};
    static void DecodeFromJson(const UniValue& data, RefundInfo& refundInfo);
};

#endif //PRIZES_ORDER_H
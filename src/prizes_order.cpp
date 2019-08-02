#include "prizes_order.h"
#include "boost/lexical_cast.hpp"
#include "prizes_service.h"
#include <boost/algorithm/string.hpp>
void ServiceOrder::DecodeFromJson(const UniValue& data, ServiceOrder& order)
{
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "order_id")
            order.OriderID = tdata.get_str();
        else if (vKeys[i] == "out_point")
            order.OutPoint = String2OutPoint(tdata.get_str());
        else if (vKeys[i] == "created_at")
            order.CreatedAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "remoce_at")
            order.RemoveAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "order_state")
            order.OrderState = tdata.get_str();
        else if (vKeys[i] == "service_price")
            order.ServicePrice = CAmount(tdata.get_int64());
        else if (vKeys[i] == "drawee")
            order.Drawee = tdata.get_str();
        else if (vKeys[i] == "balance")
            order.Balance = CAmount(tdata.get_int64());
        else if (vKeys[i] == "last_statement_time")
            order.LastStatementTime = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "refund")
            RefundPayment::DecodeFromJson(tdata, order.Refund);
        else if (vKeys[i] == "statement") {
            for (int j = 0; j < tdata.size(); ++j) {
                PrizeStatement statement{};
                PrizeStatement::DecodeFromJson(tdata[j], statement);
                order.Statement.push_back(statement);
            }
        }
    }
}
void RefundPayment::DecodeFromJson(const UniValue& data, RefundPayment& refund)
{
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        ;
        if (vKeys[i] == "refund_id")
            refund.RefundID = tdata.get_str();
        else if (vKeys[i] == "created_at")
            refund.CreatedAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "total_amount")
            refund.TotalAmount = CAmount(tdata.get_int64());
        else if (vKeys[i] == "drawee")
            refund.Drawee = tdata.get_str();
        else if (vKeys[i] == "refund_transaction")
            refund.RefundTransaction = tdata.get_str();
    }
}
void RefundInfo::DecodeFromJson(const UniValue& data, RefundInfo& refundInfo)
{
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        ;
        if (vKeys[i] == "refund_pay") {
            for (int j = 0; j < tdata.size(); ++j) {
                RefundPayment refundPayment{};
                RefundPayment::DecodeFromJson(tdata[j], refundPayment);
                refundInfo.RefundPay.push_back(refundPayment);
            }
        } else if (vKeys[i] == "statement")
            PrizeStatement::DecodeFromJson(tdata, refundInfo.Statement);
    }
}
COutPoint String2OutPoint(std::string strOutput)
{
    std::vector<std::string> vstrsplit{};
    boost::split(vstrsplit, strOutput, boost::is_any_of("-"));
    return COutPoint(uint256S(vstrsplit[0]), boost::lexical_cast<uint32_t>(vstrsplit[1]));
}

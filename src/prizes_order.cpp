#include "prizes_order.h"
#include "prizes_service.h"
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
        else if (vKeys[i] == "order_state")
            order.OrderState = tdata.get_str();
        else if (vKeys[i] == "service_price")
            order.ServicePrice = CAmount(tdata.get_int64());
        else if (vKeys[i] == "master_node_fee_rate")
            order.MasterNodeFeeRate = tdata.get_int64();
        else if (vKeys[i] == "dev_fee_rate")
            order.DevFeeRate = tdata.get_int64();
        else if (vKeys[i] == "master_node_fee_address")
            order.MasterNodeFeeAddress = tdata.get_str();
        else if (vKeys[i] == "dev_fee_address")
            order.DevFeeAddress = tdata.get_str();
        else if (vKeys[i] == "drawee")
            order.Drawee = tdata.get_str();
        else if (vKeys[i] == "balance")
            order.Balance = CAmount(tdata.get_int64());
        else if (vKeys[i] == "pay_amount")
            order.PayAmount = CAmount(tdata.get_int64());
        else if (vKeys[i] == "remaining_time_duration")
            order.RemainingTimeDuration = tdata.get_int64();
        else if (vKeys[i] == "total_time_duration")
            order.TotalTimeDuration = tdata.get_int64();
        else if (vKeys[i] == "last_statement_time")
            order.LastStatementTime = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "statement") {
            for (int j = 0; j < tdata.size(); ++j) {
                PrizeStatement statement{};
                PrizeStatement::DecodeFromJson(tdata[j], statement);
                order.Statement.push_back(statement);
            }
        }
    }
}
UniValue ServiceOrder::ToJson(ServiceOrder& serviceOrder){
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("order_id", serviceOrder.OriderID));
        data.push_back(Pair("out_point", serviceOrder.OutPoint.ToStringShort()));
        data.push_back(Pair("created_at", serviceOrder.CreatedAt));
        data.push_back(Pair("order_state", serviceOrder.OrderState));
        data.push_back(Pair("service_price", serviceOrder.ServicePrice));
        data.push_back(Pair("master_node_fee_rate", serviceOrder.MasterNodeFeeRate));
        data.push_back(Pair("dev_fee_rate", serviceOrder.DevFeeRate));
        data.push_back(Pair("master_node_fee_address", serviceOrder.MasterNodeFeeAddress));
        data.push_back(Pair("dev_fee_address", serviceOrder.DevFeeAddress));
        data.push_back(Pair("drawee", serviceOrder.Drawee));
        data.push_back(Pair("balance", serviceOrder.Balance));
        data.push_back(Pair("pay_amount", serviceOrder.PayAmount));
        data.push_back(Pair("remaining_time_duration", serviceOrder.RemainingTimeDuration));
        data.push_back(Pair("total_time_duration", serviceOrder.TotalTimeDuration));
        data.push_back(Pair("last_statement_time", serviceOrder.LastStatementTime));
    }
    {
        UniValue obj(UniValue::VOBJ);

        for(int i=0;i<serviceOrder.Statement.size();++i){
            obj = PrizeStatement::ToJson(serviceOrder.Statement[i]);
            data.push_back(Pair("statement", obj));
        }
    }
    return data;
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
        else if (vKeys[i] == "refund_transaction")
            refund.RefundTransaction = tdata.get_str();
    }
}

UniValue RefundPayment::ToJson(RefundPayment& refund){
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("refund_id", refund.RefundID));
        data.push_back(Pair("created_at", refund.CreatedAt));
        data.push_back(Pair("total_amount", refund.TotalAmount));
        data.push_back(Pair("refund_transaction", refund.RefundTransaction));
    }

    return data;
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

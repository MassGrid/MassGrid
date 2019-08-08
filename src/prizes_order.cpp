#include "prizes_order.h"
#include "prizes_service.h"
void ServiceOrder::DecodeFromJson(const UniValue& data, ServiceOrder& order)
{
    LogPrint("docker","ServiceOrder::DecodeFromJson\n");
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "order_id")
            order.OriderID = tdata.get_str();
        else if (vKeys[i] == "out_point")
            order.OutPoint = String2OutPoint(tdata.get_str());
        else if (vKeys[i] == "created_at")
            order.CreatedAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "remove_at")
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
UniValue ServiceOrder::ToJson(ServiceOrder& serviceOrder){
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("order_id", serviceOrder.OriderID));
        data.push_back(Pair("out_point", serviceOrder.OutPoint.ToStringShort()));
        data.push_back(Pair("created_at", serviceOrder.CreatedAt));
        data.push_back(Pair("remove_at", serviceOrder.RemoveAt));
        data.push_back(Pair("order_state", serviceOrder.OrderState));
        data.push_back(Pair("service_price", serviceOrder.ServicePrice));
        data.push_back(Pair("drawee", serviceOrder.Drawee));
        data.push_back(Pair("balance", serviceOrder.Balance));
        data.push_back(Pair("last_statement_time", serviceOrder.LastStatementTime));
    }
    {
        UniValue obj(UniValue::VOBJ);

        obj = RefundPayment::ToJson(serviceOrder.Refund);
        data.push_back(Pair("refund", obj));

        for(int i=0;i<serviceOrder.Statement.size();++i){
            obj = PrizeStatement::ToJson(serviceOrder.Statement[i]);
            data.push_back(Pair("statement", obj));
        }
    }
    return data;
}
void RefundPayment::DecodeFromJson(const UniValue& data, RefundPayment& refund)
{
    LogPrint("docker","RefundPayment::DecodeFromJson\n");
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

UniValue RefundPayment::ToJson(RefundPayment& refund){
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("refund_id", refund.RefundID));
        data.push_back(Pair("created_at", refund.CreatedAt));
        data.push_back(Pair("total_amount", refund.TotalAmount));
        data.push_back(Pair("drawee", refund.Drawee));
        data.push_back(Pair("refund_transaction", refund.RefundTransaction));
    }

    return data;
}
void RefundInfo::DecodeFromJson(const UniValue& data, RefundInfo& refundInfo)
{
    LogPrint("docker","RefundInfo::DecodeFromJson\n");
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

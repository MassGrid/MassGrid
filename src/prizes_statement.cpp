#include "prizes_statement.h"
void PrizeStatement::DecodeFromJson(const UniValue& data, PrizeStatement& statement)
{
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "statement_id")
            statement.StatementID = tdata.get_str();
        else if (vKeys[i] == "created_at")
            statement.CreatedAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "statement_start_at")
            statement.StatementStartAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "statement_end_at")
            statement.StatementEndAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "total_amount")
            statement.TotalAmount = CAmount(tdata.get_int64());
        else if (vKeys[i] == "master_node_fee_rate")
            statement.MasterNodeFeeRate = tdata.get_int64();
        else if (vKeys[i] == "dev_fee_rate")
            statement.DevFeeRate = tdata.get_int64();
        else if (vKeys[i] == "master_node_fee_address")
            statement.MasterNodeFeeAddress = tdata.get_str();
        else if (vKeys[i] == "dev_fee_address")
            statement.DevFeeAddress = tdata.get_str();
        else if (vKeys[i] == "statement_transaction")
            statement.StatementTransaction = tdata.get_str();
    }
}

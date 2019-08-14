#include "prizes_service.h"
#include "boost/lexical_cast.hpp"
#include "messagesigner.h"
#include <boost/algorithm/string.hpp>
void ServiceUpdate::DecodeFromJson(const UniValue& data, ServiceUpdate& serviceUpdate)
{
    LogPrint("docker","ServiceUpdate::DecodeFromJson\n");
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "service_id")
            serviceUpdate.ServiceID = tdata.get_str();
        else if (vKeys[i] == "amount")
            serviceUpdate.Amount = CAmount(tdata.get_int64());
        else if (vKeys[i] == "out_point")
            serviceUpdate.OutPoint = String2OutPoint(tdata.get_str());
        else if (vKeys[i] == "service_price")
            serviceUpdate.ServicePrice = CAmount(tdata.get_int64());
        else if (vKeys[i] == "drawee")
            serviceUpdate.Drawee = tdata.get_str();
        else if (vKeys[i] == "pubkey") {
            std::string strPubkey = tdata.get_str();
            serviceUpdate.pubKeyClusterAddress = CPubKey(strPubkey.begin(), strPubkey.end());
        }
    }
}
UniValue ServiceUpdate::ToJson(ServiceUpdate& serviceUpdate)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("service_id", serviceUpdate.ServiceID));
        data.push_back(Pair("out_point", serviceUpdate.OutPoint.ToStringShort()));
        data.push_back(Pair("amount", serviceUpdate.Amount));
        data.push_back(Pair("service_price", serviceUpdate.ServicePrice));
        data.push_back(Pair("drawee", serviceUpdate.Drawee));
        data.push_back(Pair("pubkey", serviceUpdate.pubKeyClusterAddress.ToString().substr(0, 66)));
        data.push_back(Pair("master_node_fee_address", serviceUpdate.MasterNodeFeeAddress));
        data.push_back(Pair("dev_fee_address", serviceUpdate.DevFeeAddress));
        data.push_back(Pair("master_node_fee_rate", serviceUpdate.MasterNodeFeeRate));
        data.push_back(Pair("dev_fee_rate", serviceUpdate.DevFeeRate));
    }
    return data;
}
std::string ClusterServiceUpdate_t::ToString()
{
    std::string str = pubKeyClusterAddress.ToString().substr(0, 66) +OutPoint.ToStringShort() + CrerateOutPoint.ToStringShort();
    return str;
}

bool DockerUpdateService::Sign(const CKey& keyClusterAddress, const CPubKey& pubKeyClusterAddress)
{
    std::string strError;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = clusterServiceUpdate.ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);

    if (!CMessageSigner::SignMessage(strMessage, vchSig, keyClusterAddress)) {
        LogPrintf("DockerUpdateService::Sign -- SignMessage() failed\n");
        return false;
    }
    if (!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerUpdateService::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool DockerUpdateService::CheckSignature(CPubKey& pubKeyClusterAddress)
{
    // TODO: add sentinel data
    std::string strMessage = clusterServiceUpdate.ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";

    if (!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerUpdateService::CheckSignature -- Got bad signature, error: %s\n", strError);
        return false;
    }
    return true;
}

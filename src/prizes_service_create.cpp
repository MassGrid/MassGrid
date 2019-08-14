#include "prizes_service_create.h"
#include "boost/lexical_cast.hpp"
#include "messagesigner.h"
#include <boost/algorithm/string.hpp>
void ServiceCreate::DecodeFromJson(const UniValue& data, ServiceCreate& serviceCreate)
{
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "service_name")
            serviceCreate.ServiceName = tdata.get_str();
        else if (vKeys[i] == "image")
            serviceCreate.Image = tdata.get_str();
        else if (vKeys[i] == "ssh_pubkey")
            serviceCreate.SSHPubkey = tdata.get_str();
        else if (vKeys[i] == "out_point")
            serviceCreate.OutPoint = String2OutPoint(tdata.get_str());
        else if (vKeys[i] == "hardware")
            HardWare::DecodeFromJson(tdata, serviceCreate.hardware);
        else if (vKeys[i] == "env")
            ParseLabels(tdata, serviceCreate.ENV);
        else if (vKeys[i] == "amount")
            serviceCreate.Amount = tdata.get_int64();
        else if (vKeys[i] == "service_price")
            serviceCreate.ServicePrice = tdata.get_int64();
        else if (vKeys[i] == "master_node_n2n_addr")
            serviceCreate.MasterNodeN2NAddr = tdata.get_str();
        else if (vKeys[i] == "drawee")
            serviceCreate.Drawee = tdata.get_str();
        else if (vKeys[i] == "master_node_fee_rate")
            serviceCreate.MasterNodeFeeRate = tdata.get_int64();
        else if (vKeys[i] == "dev_fee_rate")
            serviceCreate.DevFeeRate = tdata.get_int64();
        else if (vKeys[i] == "master_node_fee_address")
            serviceCreate.MasterNodeFeeAddress = tdata.get_str();
        else if (vKeys[i] == "dev_fee_address")
            serviceCreate.DevFeeAddress = tdata.get_str();
        else if (vKeys[i] == "pubkey") {
            std::string strPubkey = tdata.get_str();
            serviceCreate.pubKeyClusterAddress = CPubKey(strPubkey.begin(), strPubkey.end());
        }
    }
}
UniValue ServiceCreate::ToJson(ServiceCreate& serviceCreate)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("service_name", serviceCreate.ServiceName));
        data.push_back(Pair("image", serviceCreate.Image));
        data.push_back(Pair("ssh_pubkey", serviceCreate.SSHPubkey));
        data.push_back(Pair("out_point", serviceCreate.OutPoint.ToStringShort()));
        data.push_back(Pair("amount", serviceCreate.Amount));
        data.push_back(Pair("service_price", serviceCreate.ServicePrice));
        data.push_back(Pair("master_node_n2n_addr", serviceCreate.MasterNodeN2NAddr));
        data.push_back(Pair("drawee", serviceCreate.Drawee));
        data.push_back(Pair("master_node_fee_rate", serviceCreate.MasterNodeFeeRate));
        data.push_back(Pair("dev_fee_rate", serviceCreate.DevFeeRate));
        data.push_back(Pair("master_node_fee_address", serviceCreate.MasterNodeFeeAddress));
        data.push_back(Pair("dev_fee_address", serviceCreate.DevFeeAddress));
        data.push_back(Pair("pubkey", serviceCreate.pubKeyClusterAddress.ToString().substr(0, 66)));
    }
    {
        UniValue obj(UniValue::VOBJ);

        obj = HardWare::ToJson(serviceCreate.hardware);
        data.push_back(Pair("hardware", obj));


        obj = SpecLabelsToJson(serviceCreate.ENV);
        data.push_back(Pair("env", obj));
    }
    return data;
}

std::string ClusterServiceCreate_t::ToString()
{
    std::string str = ServiceName + Image + SSHPubkey + pubKeyClusterAddress.ToString().substr(0, 66) +
                      OutPoint.ToStringShort()  + hardware.ToString();
    for (auto& pair : ENV) {
        str += (pair.first + pair.second);
    }
    return str;
}
bool DockerCreateService::Sign(const CKey& keyClusterAddress, const CPubKey& pubKeyClusterAddress)
{
    std::string strError;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = clusterServiceCreate.ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);

    if (!CMessageSigner::SignMessage(strMessage, vchSig, keyClusterAddress)) {
        LogPrintf("DockerCreateService::Sign -- SignMessage() failed\n");
        return false;
    }
    if (!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerCreateService::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool DockerCreateService::CheckSignature(CPubKey& pubKeyClusterAddress)
{
    // TODO: add sentinel data
    std::string strMessage = clusterServiceCreate.ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";

    if (!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerCreateService::CheckSignature -- Got bad signature, error: %s\n", strError);
        return false;
    }
    return true;
}
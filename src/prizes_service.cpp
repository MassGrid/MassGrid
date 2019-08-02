#include "prizes_service.h"
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
            serviceCreate.Drawee = tdata.get_int64();
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
void ServiceUpdate::DecodeFromJson(const UniValue& data, ServiceUpdate& serviceUpdate)
{
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
    }
    return data;
}
std::string ClusterServiceUpdate_t::ToString()
{
    std::string str = pubKeyClusterAddress.ToString().substr(0, 66) +OutPoint.ToStringShort() + CrerateOutPoint.ToStringShort();
    return str;
}
void ServiceInfo::DecodeFromJson(const UniValue& data, ServiceInfo& serviceInfo)
{
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "service_id")
            serviceInfo.ServiceID = tdata.get_str();
        else if (vKeys[i] == "create_at")
            serviceInfo.CreatedAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "delete_at")
            serviceInfo.DeleteAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "next_check_time")
            serviceInfo.NextCheckTime = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "order") {
            for (int64_t j = 0; j < tdata.size(); ++j) {
                ServiceOrder serviceOrder{};
                ServiceOrder::DecodeFromJson(tdata[j], serviceOrder);
                serviceInfo.Order.push_back(serviceOrder);
            }
        } else if (vKeys[i] == "create_spec") {
            ServiceCreate::DecodeFromJson(tdata,serviceInfo.CreateSpec);
        } else if (vKeys[i] == "update_spec") {
            for (int64_t j = 0; j < tdata.size(); ++j) {
                ServiceUpdate serviceUpdate{};
                ServiceUpdate::DecodeFromJson(tdata[j], serviceUpdate);
                serviceInfo.UpdateSpec.push_back(serviceUpdate);
            }
        } else if (vKeys[i] == "state")
            serviceInfo.State = tdata.get_str();
        else if (vKeys[i] == "task_info"){
            Task task{};
            Task::DecodeFromJson(tdata,task);
            serviceInfo.TaskInfo.push_back(task);
        }
    }
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
#include"prizes_client.h"
#include "http.h"
PrizesClient prizesClient;
void PrizesClient::SetDockerApiConnection(std::string in){
        SplitHostPort(in,APIport,APIAddr);
        LogPrintf("PrizesClient::SetDockerApiConnection connection address %s:%u\n",APIAddr,APIport);
}
bool whetherError(std::string responseData,UniValue& resultData,std::string& err){

    UniValue jsondata(UniValue::VOBJ);
    if (responseData.empty()) {
        LogPrintf("PrizesClient::whetherError request error responseData empty\n");
        return true;
    }
    jsondata.read(responseData);
    UniValue value = jsondata["error"];
    if (!value.isNull()) {
        err = value.get_str();
        LogPrintf("PrizesClient::whetherError request error %s\n",err);
        return true;
    }
    resultData = jsondata["result"];
    return false;
}

void PrizesClient::SetDockerUnixSock(std::string in){
    unix_sock_address = in;
}
bool PrizesClient::PostServiceCreate(ServiceCreate& serviceCreate, std::string& ServiceID, std::string& err){
    LogPrint("docker", "PrizesClient::PostServiceCreate\n");
    std::string url = "/servicecreate";
    std::string requestData = ServiceCreate::ToJson(serviceCreate).write();

    LogPrint("docker", "PrizesClient::PostServiceCreate requestData %s\n", requestData);
    HttpRequest http(APIAddr, APIport, url, requestData, unix_sock_address);
    int ret;
    ret = http.HttpPost();

    if (ret < 200 || ret >= 300) {
        LogPrint("docker", "PrizesClient::PostServiceCreate Http_Error error_code: %d\n", ret);
        err = "ret error" +to_string(ret);
        return false;
    }
    std::string responseData = http.getReponseData();

    UniValue jsondata(UniValue::VOBJ);
    if (whetherError(responseData, jsondata,err)){
        LogPrint("docker", "PrizesClient::PostServiceCreate request error %s\n", err);
        return false;
    }
    ServiceID = jsondata["ID"].get_str();
    return true;
}
bool PrizesClient::PostServiceUpdate(ServiceUpdate& serviceUpdate,std::string& err){
    LogPrint("docker", "PrizesClient::PostServiceUpdate");
    std::string url = "/serviceupdate/" + serviceUpdate.ServiceID;
    std::string requestData = ServiceUpdate::ToJson(serviceUpdate).write();
    LogPrint("docker", "PrizesClient::PostServiceUpdate requestData %s\n", requestData);
    HttpRequest http(APIAddr, APIport, url, requestData, unix_sock_address);
    int ret;
    ret = http.HttpPost();

    if (ret < 200 || ret >= 300) {
        LogPrint("docker", "PrizesClient::PostServiceUpdate Http_Error error_code: %d\n", ret);
        err = "ret error" + to_string(ret);
        return false;
    }
    std::string responseData = http.getReponseData();

    UniValue jsondata(UniValue::VOBJ);
    if (whetherError(responseData, jsondata,err)) {
        LogPrint("docker", "PrizesClient::PostServiceUpdate request error %s\n", err);
        return false;
    }
    return true;
}

bool PrizesClient::GetService(std::string ServiceID,ServiceInfo& serviceInfo, std::string& err){
    LogPrint("docker", "PrizesClient::GetService");
    std::string url = "/getservice/" + ServiceID;
    std::string requestData{};
    HttpRequest http(APIAddr, APIport, url, requestData, unix_sock_address);
    int ret;
    ret = http.HttpGet();

    if (ret < 200 || ret >= 300) {
        LogPrint("docker", "PrizesClient::GetService Http_Error error_code: %d\n", ret);
        return false;
    }
    std::string responseData = http.getReponseData();

    UniValue jsondata(UniValue::VOBJ);
    if (whetherError(responseData, jsondata,err)) {
        LogPrint("docker", "PrizesClient::GetService request error %s\n", err);
        return false;
    }
    ServiceInfo::DecodeFromJson(jsondata, serviceInfo);
    return true;
}
bool PrizesClient::GetServiceFromPubkey(std::string strPubkey, std::vector<ServiceInfo>& vecServiceInfo, std::string& err){
    LogPrint("docker", "PrizesClient::GetServiceFromPubkey");
    std::string url = "/getservicesfrompubkey/" + strPubkey;
    std::string requestData{};
    HttpRequest http(APIAddr, APIport, url, requestData, unix_sock_address);
    int ret;
    ret = http.HttpGet();

    if (ret < 200 || ret >= 300) {
        LogPrint("docker", "PrizesClient::GetServiceFromPubkey Http_Error error_code: %d\n", ret);
        return false;
    }
    std::string responseData = http.getReponseData();
    
    UniValue jsondata(UniValue::VARR);
    if (whetherError(responseData, jsondata,err)) {
        LogPrint("docker", "PrizesClient::GetServiceFromPubkey request error %s\n", err);
        return false;
    }
    for(int i=0;i<jsondata.size();++i){
        ServiceInfo serviceInfo{};
        ServiceInfo::DecodeFromJson(jsondata[i], serviceInfo);
        vecServiceInfo.push_back(serviceInfo);
    }
    return true;
}

bool PrizesClient::GetServiceDelete(std::string ServiceID,uint256 statementid, std::string& err){
    LogPrint("docker", "PrizesClient::GetServiceDelete");
    std::string url = "/servicerefund/"+ServiceID;
    std::string requestData{};
    HttpRequest http(APIAddr, APIport, url, requestData, unix_sock_address);
    int ret;
    ret = http.HttpGet();

    if (ret < 200 || ret >= 300) {
        LogPrint("docker", "PrizesClient::GetServiceDelete Http_Error error_code: %d\n", ret);
        return false;
    }
    std::string responseData = http.getReponseData();

    UniValue jsondata(UniValue::VOBJ);
    if (whetherError(responseData, jsondata,err)) {
        LogPrint("docker", "PrizesClient::GetServiceDelete request error %s\n", err);
        return false;
    }
    statementid = uint256S(jsondata["txid"].get_str());
    return true;
}

bool PrizesClient::GetNodeList(NodeListStatistics& nodeListStatistics, std::string& err)
{
    LogPrint("docker", "PrizesClient::GetNodeList");
    std::string url = "/getnodes";
    std::string requestData{};
    HttpRequest http(APIAddr, APIport, url, requestData, unix_sock_address);
    int ret;
    ret = http.HttpGet();

    if (ret < 200 || ret >= 300) {
        LogPrint("docker", "PrizesClient::GetNodeList Http_Error error_code: %d\n", ret);
        return false;
    }
    std::string responseData = http.getReponseData();

    UniValue jsondata(UniValue::VOBJ);
    if (whetherError(responseData, jsondata,err)) {
        LogPrint("docker", "PrizesClient::PostServiceCreate request error %s\n", err);
        return false;
    }
    NodeListStatistics::DecodeFromJson(jsondata, nodeListStatistics);
    return true;
}
bool PrizesClient::GetMachines(ResponseMachines& machines, std::string& err)
{
    LogPrint("docker", "PrizesClient::GetMachines\n");
    NodeListStatistics nodeListStatistics{};
    std::string url = "/getnodes";
    std::string requestData{};
    HttpRequest http(APIAddr, APIport, url, requestData, unix_sock_address);
    int ret;
    ret = http.HttpGet();

    if (ret < 200 || ret >= 300) {
        LogPrint("docker", "PrizesClient::GetMachines Http_Error error_code: %d\n", ret);
        return false;
    }
    std::string responseData = http.getReponseData();

    UniValue jsondata(UniValue::VOBJ);
    if (whetherError(responseData, jsondata,err)) {
        LogPrint("docker", "PrizesClient::PostServiceCreate request error %s\n", err);
        return false;
    }
    NodeListStatistics::DecodeFromJson(jsondata, nodeListStatistics);
    for (auto& nodeinfo : nodeListStatistics.list) {
        Item item(nodeinfo.hardware.CPUType, nodeinfo.hardware.CPUThread, nodeinfo.hardware.MemoryType, nodeinfo.hardware.MemoryCount, nodeinfo.hardware.GPUType, nodeinfo.hardware.GPUCount);
        if (machines.items.count(item)) {
            machines.items[item].count++;
        } else {
            CAmount price = dockerPriceConfig.getPrice(item.cpu.Type, item.cpu.Name) * item.cpu.Count +
                            dockerPriceConfig.getPrice(item.mem.Type, item.mem.Name) * item.mem.Count +
                            dockerPriceConfig.getPrice(item.gpu.Type, item.gpu.Name) * item.gpu.Count;
            machines.items[item] = Value_price(price, 1);
        }
    }
    machines.TotalCount = nodeListStatistics.TotalCount;
    machines.AvailabilityCount = nodeListStatistics.TotalCount;
    machines.UsableCount = nodeListStatistics.TotalCount;
    machines.Token = nodeListStatistics.Token;
    return true;
}
#include "boost/lexical_cast.hpp"
#include "dockernode.h"
#include "univalue.h"
#include "dockerpriceconfig.h"
namespace Config{
    const char* strRole[]={"worker","manager"};
    const char* strNodeStatusState[]={"unknown","down","ready","disconnected"};
    const char* strAvailability[]={"active","pause","drain"};
    const char* strRechability[]={"unknown","unreachable","rechable"};
};
std::string dockernodefilter::ToJsonString(){
    UniValue data(UniValue::VOBJ);
    if(!id.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter = id.begin();iter!=id.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("id",arr));
    }   
    if(!label.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter = label.begin();iter!=label.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("label",arr));
    }
    if(!name.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter = name.begin();iter!=name.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("name",arr));
    }

    {
        UniValue arr(UniValue::VARR);
        if(Membership_accepted){
            arr.push_back("accepted"); 
        }
        if(Membership_pending){
            arr.push_back("pending"); 
        }
        data.push_back(Pair("membership",arr));
    }

    {
        UniValue arr(UniValue::VARR);
        if(Role_manager){
            arr.push_back("manager"); 
        }
        if(Role_worker){
            arr.push_back("worker"); 
        }
        data.push_back(Pair("role",arr));
    }
    return data.write();
}
void Node::NodeListUpdateAll(const string& nodeData,std::map<std::string,Node> &nodes)
{
    LogPrint("docker","Node::NodeListUpdateAll docker json node\n");
    try{
        UniValue dataArry(UniValue::VARR);
        if(!dataArry.read(nodeData)){
            LogPrint("docker","Node::NodeListUpdateAll docker json error\n");
            return;
        }

        for(size_t i=0;i<dataArry.size();i++){
            UniValue data(dataArry[i]);
            string id = find_value(data,"ID").get_str();
            UniValue tdata = find_value(data,"Version").get_obj();
            int64_t index=find_value(tdata,"Index").get_int64();
            auto it = nodes.find(id);
            if(it!=nodes.end() && it->second.version.index == index)    //if the elem existed and needn't update
                continue;
            Node node;
            bool fSuccess = DecodeFromJson(data,node);
            if(fSuccess)
                nodes[node.ID]=node;
        }
    }catch(std::exception& e){
        LogPrint("docker","Node::NodeListUpdateAll JSON read error,%s\n",string(e.what()).c_str());
    }catch(...){
        LogPrint("docker","Node::NodeListUpdateAll unkonw exception\n");
    }
}
void Node::NodeListUpdate(const string& nodeData,std::map<std::string,Node> &nodes)
{
    LogPrint("docker","Node::NodeListUpdate docker json node\n");
    try{
        UniValue data(UniValue::VOBJ);
        if(!data.read(nodeData)){
            LogPrint("docker","Service::NodeListUpdate docker json error\n");
            return;
        }
        string id = find_value(data,"ID").get_str();
        UniValue tdata = find_value(data,"Version").get_obj();
        int64_t index=find_value(tdata,"Index").get_int64();
        auto it = nodes.find(id);
        if(it!=nodes.end() && it->second.version.index == index)    //if the elem existed and needn't update
            return;
        Node node;
        bool fSuccess = DecodeFromJson(data,node);
        if(fSuccess)
            nodes[node.ID]=node;
    
    }catch(std::exception& e){
        LogPrint("docker","Node::NodeListUpdate JSON read error,%s\n",string(e.what()).c_str());
    }catch(...){
        LogPrint("docker","Node::NodeListUpdate unkonw exception\n");
    }
}
bool Node::DecodeFromJson(const UniValue& data, Node& node)
{
    
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID") node.ID=tdata.get_str();
            else if(vKeys[i]=="CreatedAt") node.createdAt=getDockerTime(tdata.get_str());
            else if(vKeys[i]=="UpdatedAt") node.updatedAt=getDockerTime(tdata.get_str());
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Version"){
                node.version.index=find_value(tdata,"Index").get_int64();
            }else if(vKeys[i]=="Spec"){
                ParseNodeSpec(tdata,node.spec);
            }else if(vKeys[i]=="Description"){
                ParseNodeDescription(tdata,node.description);
            }else if(vKeys[i]=="Status"){
                ParseNodeStatus(tdata,node.status);
            }else if(vKeys[i]=="ManagerStatus"){
                ParseNodeManageStatus(tdata,node.managerStatus);
            }
        }
        auto cpuSet = dockerPriceConfig.getNameSet("cpu");
        auto memSet = dockerPriceConfig.getNameSet("mem");
        auto gpuSet = dockerPriceConfig.getNameSet("gpu");
        for(auto it= node.description.engine.labels.begin();it!=node.description.engine.labels.end();++it){
            if(it->first == "com.massgrid.address"){
                node.MGDAddress = it->second;
            }else if(it->first == "cpuname"){
                if(cpuSet.count(it->second))
                    node.engineInfo.cpu.Name = it->second;
            }else if(it->first == "cpucount"){
                node.engineInfo.cpu.Count = boost::lexical_cast<int>(it->second);
            }else if(it->first == "memname"){
                if(memSet.count(it->second))
                    node.engineInfo.mem.Name = it->second;
            }else if(it->first == "memcount"){
                node.engineInfo.mem.Count = boost::lexical_cast<int>(it->second);
            }else if(it->first == "gpuname"){
                if(gpuSet.count(it->second))
                    node.engineInfo.gpu.Name = it->second;
            }else if(it->first == "gpucount"){
                node.engineInfo.gpu.Count = boost::lexical_cast<int>(it->second);
            }
        }
    }
    return true;
}
void Node::ParseNodeSpec(const UniValue& data,Config::NodeSpec &spec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Role"){
                spec.role=GetRoleType(tdata.get_str());
            }else if(vKeys[i]=="Availability"){
                spec.availability=GetAvailabilityType(tdata.get_str());
            }else if(vKeys[i]=="Name"){
                spec.name=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Labels"){
                ParseNodeLabels(tdata,spec.labels);
            }
        }
    }
}
void Node::ParseNodeLabels(const UniValue& data,std::map<std::string,std::string> &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            labels.insert(std::make_pair(vKeys[i],tdata.get_str()));
        }
    }
}
void Node::ParseNodeDescription(const UniValue& data,Config::NodeDescription &decp)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Hostname"){
                decp.hostname=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Platform"){
                ParseNodePlatform(tdata,decp.platform);
            }else if(vKeys[i]=="Resources"){
                ParseNodeResource(tdata,decp.resources);
            }else if(vKeys[i]=="Engine"){
                ParseNodeEngine(tdata,decp.engine);
            }else if(vKeys[i]=="TLSInfo"){
                ParseNodeTLSInfo(tdata,decp.tlsInfo);
            }
        }
    }
}
void Node::ParseNodePlatform(const UniValue& data,Config::Platform &platform)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Architecture") platform.architecture=tdata.get_str();
            else if(vKeys[i]=="OS") platform.OS=tdata.get_str();
        }
    }
}
void Node::ParseNodeResource(const UniValue& data, Config::ResourceObj &resources)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="NanoCPUs"){
                resources.nanoCPUs=tdata.get_int64();
            }else if(vKeys[i]=="MemoryBytes"){
                resources.memoryBytes=tdata.get_int64();
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="GenericResources"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::GenericResources genResources;
                    ParseNodeGenResources(tdata[j],genResources);
                    resources.genericResources.push_back(genResources);
                }
            }
        }
    }
}
void Node::ParseNodeGenResources(const UniValue& data, Config::GenericResources &genResources)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="NamedResourceSpec"){
                ParseNodeGenResNameSpec(tdata,genResources.namedResourceSpec);
            }else if(vKeys[i]=="DiscreteResourceSpec"){
                ParseNodeGenResDiscSpec(tdata,genResources.discreateResourceSpec);
            }
        }
    }
}
void Node::ParseNodeGenResNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Kind"){
                namedResourceSpec.kind=tdata.get_str();
            }else if(vKeys[i]=="Value"){
                namedResourceSpec.value=tdata.get_str();
            }
        }
    }
}
void Node::ParseNodeGenResDiscSpec(const UniValue& data, Config::DiscreteResourceSpec &discResourceSpec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Kind"){
                discResourceSpec.kind=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Value"){
                discResourceSpec.value=tdata.get_int64();
            }
        }
    }
}
void Node::ParseNodeEngine(const UniValue& data, Config::EngineDescription &engine)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="EngineVersion"){
                engine.engineVersion=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Labels"){
                ParseNodeEngineLabels(tdata,engine.labels);//no key
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Plugins"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::Plugins engineplugins;
                    ParseNodeEnginePlugins(tdata[j],engineplugins);
                    engine.plugin.push_back(engineplugins);
                }
            }
        }
    }
}
void Node::ParseNodeEngineLabels(const UniValue& data, map<std::string,std::string> &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            labels.insert(std::make_pair(vKeys[i],tdata.get_str()));
        }
    }
}
void Node::ParseNodeEnginePlugins(const UniValue& data, Config::Plugins &splugin)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Type"){
                splugin.type =tdata.get_str();
            }else if(vKeys[i]=="Name"){
                splugin.name =tdata.get_str();
            }
        }
    }
}
void Node::ParseNodeTLSInfo(const UniValue& data, Config::TLSInfo &tlsinfo)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="TrustRoot"){
                tlsinfo.trustRoot =tdata.get_str();
            }else if(vKeys[i]=="CertIssuerSubject"){
                tlsinfo.certIssuerSubject =tdata.get_str();
            }else if(vKeys[i]=="CertIssuerPublicKey"){
                tlsinfo.certIssuerPublicKey =tdata.get_str();
            }
        }
    }
}
void Node::ParseNodeStatus(const UniValue& data, Config::NodeStatus &status)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="State"){
                status.state =GetNodeStatusStateType(tdata.get_str());
            }else if(vKeys[i]=="Addr"){
                status.addr =tdata.get_str();
            }else if(vKeys[i]=="Message"){
                status.message =tdata.get_str();
            }
        }
    }
}
void Node::ParseNodeManageStatus(const UniValue& data, Config::ManagerStatus &managerStatus)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isBool()){
            if(vKeys[i]=="Leader"){
                managerStatus.Leader =tdata.get_bool();
            }
        }
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Reachability"){
                managerStatus.reachability = GetNodeManageStatusType(tdata.get_str());
            }else if(vKeys[i]=="Addr"){
                managerStatus.addr =tdata.get_str();
            }
        }
    }
}
int Node::GetRoleType(std::string strType)
{
    int type=-1;
    for (int i = 0; i < ARRAYLEN(Config::strRole); i++){
        if (strType == Config::strRole[i]){
            type = i;
            break;
        }
    }
    // if (i == ARRAYLEN(ppszTypeName))
    return type;
}
int Node::GetNodeStatusStateType(std::string strType)
{
    int type=-1;
    for (int i = 0; i < ARRAYLEN(Config::strNodeStatusState); i++){
        if (strType == Config::strNodeStatusState[i]){
            type = i;
            break;
        }
    }
    return type;
}
int Node::GetAvailabilityType(std::string strType)
{
    int type=-1;
    for (int i = 0; i < ARRAYLEN(Config::strAvailability); i++){
        if (strType == Config::strAvailability[i]){
            type = i;
            break;
        }
    }
    return type;
}
int Node::GetNodeManageStatusType(std::string strType)
{
    int type=-1;
    for (int i = 0; i < ARRAYLEN(Config::strRechability); i++){
        if (strType == Config::strRechability[i]){
            type = i;
            break;
        }
    }
    return type;
}
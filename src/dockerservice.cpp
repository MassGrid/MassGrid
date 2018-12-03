#include "dockerservice.h"
#include "univalue.h"
#include "timedata.h"
#include "primitives/transaction.h"
#include <boost/lexical_cast.hpp>
#include "messagesigner.h"
bool DockerCreateService::Sign(const CKey& keyClusterAddress, const CPubKey& pubKeyClusterAddress)
{
    std::string strError;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(version) + sspec.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!CMessageSigner::SignMessage(strMessage, vchSig, keyClusterAddress)) {
        LogPrintf("DockerCreateService::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerCreateService::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool DockerCreateService::CheckSignature(CPubKey& pubKeyClusterAddress)
{
    // TODO: add sentinel data
    std::string strMessage = vin.ToString() + pubKeyClusterAddress.ToString() + boost::lexical_cast<std::string>(version) + sspec.ToString() + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerCreateService::CheckSignature -- Got bad signature, error: %s\n", strError);
        return false;
    }
    return true;
}

std::string dockerservicefilter::ToJsonString(){
    UniValue data(UniValue::VOBJ);
    if(!id.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter;iter!=id.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("id",arr));
    }   
    if(!label.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter;iter!=label.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("label",arr));
    }
    if(!name.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter;iter!=name.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("name",arr));
    }

    {
        UniValue arr(UniValue::VARR);
        if(Mode_replicated){
            arr.push_back("replicated"); 
        }
        if(Mode_global){
            arr.push_back("global"); 
        }
        data.push_back(Pair("mode",arr));
    }

    return data.write();
}
void Service::DockerServiceList(const string& serviceData,std::map<std::string,Service> &services)
{
    // LogPrint("docker","Service::DockerServiceList docker json node\n");
    try{
        UniValue dataArry(UniValue::VARR);
        if(!dataArry.read(serviceData)){
            LogPrint("docker","Service::DockerServiceList docker json error\n");
            return;
        }

        for(size_t i=0;i<dataArry.size();i++){
            UniValue data(dataArry[i]);
            Service service;
            bool fSuccess = DcokerServiceJson(data,service);
            if(fSuccess)
                services[service.ID]=service;
            // break;
        }
    }catch(std::exception& e){
        LogPrint("docker","Service::DockerServiceList JSON read error,%s\n",string(e.what()).c_str());
    }catch(...){
        LogPrint("docker","Service::DockerServiceList unkonw exception\n");
    }
}
bool Service::DcokerServiceJson(const UniValue& data, Service& service)
{
    std::string id;
    Config::Version version;
    uint64_t createdTime;
    uint64_t updateTime;
    Config::ServiceSpec spec;
    Config::ServiceSpec previousSpec;
    Config::Endpoint endpoint;
    Config::UpdateStatus updateStatus;
    int protocolVersion=DEFAULT_CSERVICE_API_VERSION;

    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(tdata.isStr()){
            if(vKeys[i]=="ID") id=tdata.get_str();
            else if(vKeys[i]=="CreatedAt") createdTime=getDockerTime(tdata.get_str());
            else if(vKeys[i]=="UpdatedAt") updateTime=getDockerTime(tdata.get_str());
        }
        if(tdata.isObject()){
            if(vKeys[i]=="Version"){
                version.index=find_value(tdata,"Index").get_int();
            }else if(vKeys[i]=="Spec"){
                ParseSpec(tdata,spec);
            }else if(vKeys[i]=="PreviousSpec"){
                ParseSpec(tdata,previousSpec);
            }else if(vKeys[i]=="Endpoint"){
                ParseEndpoint(tdata,endpoint);
            }else if(vKeys[i]=="UpdateStatus"){
                ParseUpdateStatus(tdata,updateStatus);
            }
        }
    }
    service=Service(id,version,createdTime,updateTime,spec,previousSpec,endpoint,updateStatus,protocolVersion);
    return true;
}

void Service::ParseSpec(const UniValue& data,Config::ServiceSpec &spc)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Name") 
                spc.name=tdata.get_str();
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Labels"){
                ParseSpecLabels(tdata,spc.labels);
            }else if(vKeys[i]=="TaskTemplate"){
                ParseTaskTemplate(tdata,spc.taskTemplate);
            }else if(vKeys[i]=="Mode"){
                ParseMode(tdata,spc.mode);
            }else if(vKeys[i]=="UpdateConfig"){
                ParseUpdateConfig(tdata,spc.updateConfig);
            }else if(vKeys[i]=="RollbackConfig"){
                ParseUpdateConfig(tdata,spc.rollbackConfig);
            }else if(vKeys[i]=="Networks"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::NetWork network;
                    ParseNetwork(tdata[j],network);
                    spc.networks.push_back(network);
                }
            }else if(vKeys[i]=="EndpointSpec"){
                ParseEndpointSpec(tdata,spc.endpointSpec);
            }
        }
    }
}
void Service::ParseSpecLabels(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="pubkey") labels.insert(std::make_pair("com.massgrid.pubkey",tdata.get_str()));
            else if(vKeys[i]=="txid") labels.insert(std::make_pair("com.massgrid.txid",tdata.get_str()));
        }
    }
}
void Service::ParseTaskTemplate(const UniValue& data,Config::TaskSpec &taskTemplate)
{  
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(tdata.isStr()){
            if(vKeys[i]=="Runtime") 
                taskTemplate.runtime=tdata.get_str();
        }
        if(tdata.isNum()){
            if(vKeys[i]=="ForceUpdate") taskTemplate.forceUpdate=tdata.get_int64();
        }
        if(tdata.isObject()){
            if(vKeys[i]=="ContainerSpec"){
                ParseContainerSpec(tdata,taskTemplate.containerSpec);
            }else if(vKeys[i]=="Resources"){
                ParseResource(tdata,taskTemplate.resources);
            }else if(vKeys[i] =="RestartPolicy"){
                ParseRestartPolicy(tdata,taskTemplate.restartPolicy);
            }else if(vKeys[i] =="Placement"){
                ParsePlacement(tdata,taskTemplate.placement);
            }else if(vKeys[i]=="LogDriver"){
                ParseLogDriver(tdata,taskTemplate.logdriver);
            }
        }
        if(tdata.isArray()){
            if(vKeys[i] =="Networks"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::NetWork network;
                    ParseNetwork(tdata[j],network);
                    taskTemplate.netWorks.push_back(network);
                }
            }
        }
    }
}

void Service::ParseResource(const UniValue& data,Config::Resource &resource)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Limits"){
                ParseResourceObj(tdata,resource.limits);
            }else if(vKeys[i]=="Reservation"){
                ParseResourceObj(tdata,resource.reservations);
            }
        }
    }
}
void Service::ParseResourceObj(const UniValue& data, Config::ResourceObj &resources)
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
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="GenericResources"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::GenericResources genResources;
                    ParseResGenRes(tdata[j],genResources);
                    resources.genericResources.push_back(genResources);
                }
            }
        }
    }
}
void Service::ParseResGenRes(const UniValue& data, Config::GenericResources &genResources)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="NamedResourceSpec"){
                ParseResGenNameSpec(tdata,genResources.namedResourceSpec);
            }else if(vKeys[i]=="DiscreteResourceSpec"){
                ParseResGenDiscSpec(tdata,genResources.discreateResourceSpec);
            }
        }
    }
}
void Service::ParseResGenNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Kind"){
                namedResourceSpec.kind=tdata.get_str();
            }else if(vKeys[i]=="Vaule"){
                namedResourceSpec.value=tdata.get_str();
            }
        }
    }
}
void Service::ParseResGenDiscSpec(const UniValue& data, Config::DiscreteResourceSpec &discResourceSpec)
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
            if(vKeys[i]=="Vaule"){
                discResourceSpec.value=tdata.get_int64();
            }
        }
    }
}
void Service::ParseRestartPolicy(const UniValue& data,Config::RestartPolicy &repoly)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Condition"){
                repoly.condition=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="MaxAttempts"){
                repoly.maxAttempts=tdata.get_int64();
            }else if(vKeys[i]=="Delay"){
                repoly.delay=tdata.get_int64();
            }else if(vKeys[i]=="Window"){
                repoly.window=tdata.get_int64();
            }
        }
    }
}
void Service::ParseUpdateConfig(const UniValue& data,Config::UpdateConfig &upconfig)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Parallelism"){
                upconfig.parallelism=tdata.get_int64();
            }else if(vKeys[i]=="Delay"){
                upconfig.delay=tdata.get_int64();
            }else if(vKeys[i]=="Monitor"){
                upconfig.monitor=tdata.get_int64();
            }else if(vKeys[i]=="MaxFailureRatio"){
                upconfig.maxFailureRatio=tdata.get_real();
            }
        }
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="FailureAction"){
                upconfig.failureAction=tdata.get_str();
            }else if(vKeys[i]=="Order"){
                upconfig.order=tdata.get_str();
            }
        }
    }
}
void Service::ParsePlacement(const UniValue& data,Config::Placement &placement)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Constraints"){
                ParseArray(tdata,placement.constraints);
            }else if(vKeys[i]=="Preferences"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::Preferences preference;
                    ParsePreferences(tdata[j],preference);
                    placement.preferences.push_back(preference);
                }
            }else if(vKeys[i]=="Platforms"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::Platform platform;
                    ParsePlatforms(tdata[j],platform);
                    placement.platforms.push_back(platform);
                }
            }
        }
    }
}
void Service::ParsePreferences(const UniValue& data,Config::Preferences &preference)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Spread"){
                ParsePreferencesSpread(tdata,preference.spread);
            }
        }
    }
}
void Service::ParsePreferencesSpread(const UniValue& data,Config::Spread &spread)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="SpreadDescriptor"){
                spread.spreadDescriptor=tdata.get_str();
            }
        }
    }
}
void Service::ParsePlatforms(const UniValue& data,Config::Platform &platform)
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
void Service::ParseNetwork(const UniValue& data,Config::NetWork &network)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Target") 
                network.target=tdata.get_str();
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Aliases"){
                ParseArray(tdata,network.aliases);
            }
        }
    }
}
void Service::ParseLogDriver(const UniValue& data,Config::LogDriver &logdriver)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Name"){
                logdriver.name=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Options"){//key?
                ParseLogDriverOpt(tdata,logdriver.options);
            }
        }
    }
}
void Service::ParseLogDriverOpt(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="key") labels.insert(std::make_pair("com.massgrid.key",tdata.get_str()));
        }
    }
}

void Service::ParseMode(const UniValue& data,Config::Mode &mode)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Replicated"){
                ParseModeReplicated(tdata,mode.replicated);
            }
        }
        if(data[vKeys[i]].isBool()){
            if(vKeys[i]=="Global"){
                mode.global=tdata.get_bool();
            }
        }
    }
}
void Service::ParseModeReplicated(const UniValue& data,Config::Replicated &rep)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Replicas"){
                rep.replicas=tdata.get_int64();
            }
        }
    }
}
void Service::ParseEndpointSpec(const UniValue& data,Config::EndpointSpec &endpointSpec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Ports"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::EndpointPortConfig port;
                    ParseEndSpecPort(tdata[j],port);
                    endpointSpec.ports.push_back(port);
                }
            }
        }
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Mode"){
                endpointSpec.mode=tdata.get_str();
            }
        }
    }
}
void Service::ParseEndSpecPort(const UniValue& data,Config::EndpointPortConfig &port)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Name"){
                port.name=tdata.get_str();
            }else if(vKeys[i]=="Protocol"){
                port.protocol=tdata.get_str();
            }else if(vKeys[i]=="PublishMode"){
                port.publishMode=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="TargetPort"){
                port.targetPort=tdata.get_int64();
            }else if(vKeys[i]=="PublishedPort"){
                port.publishedPort=tdata.get_int64();
            }
        }
    }
}



void Service::ParseEndpoint(const UniValue& data,Config::Endpoint &endpoint)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Spec"){
                ParseEndpointSpec(tdata,endpoint.spec);
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Ports"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::EndpointPortConfig port;
                    ParseEndSpecPort(tdata[j],port);
                    endpoint.ports.push_back(port);
                }
            }else if(vKeys[i]=="VirtualIPs"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::VirtualIP virtualip;
                    ParseVirtualIPs(tdata[j],virtualip);
                    endpoint.virtualIPs.push_back(virtualip);
                }
            }
        }
    }
}
void Service::ParseVirtualIPs(const UniValue& data,Config::VirtualIP &virtualip)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="NetworkID"){
                virtualip.networkID=tdata.get_str();
            }else if(vKeys[i]=="Addr"){
                virtualip.addr=tdata.get_str();
            }
        }
    }
}
void Service::ParseUpdateStatus(const UniValue& data,Config::UpdateStatus &updateStatus)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="State"){
                updateStatus.state =tdata.get_str();
            }else if(vKeys[i]=="Message"){
                updateStatus.message =tdata.get_str();
            }else if(vKeys[i]=="StartedAt"){
                updateStatus.createdAt=getDockerTime(tdata.get_str());
            }else if(vKeys[i]=="CompletedAt"){
                updateStatus.completedAt =getDockerTime(tdata.get_str());
            }
        }
    }
}
void Service::ParseArray(const UniValue& data,vector<std::string> &array)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        array.push_back(data[vKeys[i]].get_str());
    }
}
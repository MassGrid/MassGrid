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
    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(version) + n2n_community +
        serviceName + image + gpuname + ssh_pubkey + boost::lexical_cast<std::string>(cpu) + boost::lexical_cast<std::string>(gpu) +
        boost::lexical_cast<std::string>(memory_byte) + boost::lexical_cast<std::string>(sigTime);

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
    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(version) + n2n_community +
        serviceName + image + gpuname + ssh_pubkey + boost::lexical_cast<std::string>(cpu) + boost::lexical_cast<std::string>(gpu) +
        boost::lexical_cast<std::string>(memory_byte) + boost::lexical_cast<std::string>(sigTime);

    std::string strError = "";

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
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
    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(version) + n2n_community +
        serviceid + serviceName + image + gpuname + ssh_pubkey + boost::lexical_cast<std::string>(cpu) + boost::lexical_cast<std::string>(gpu) +
        boost::lexical_cast<std::string>(memory_byte) + boost::lexical_cast<std::string>(sigTime);

    if(!CMessageSigner::SignMessage(strMessage, vchSig, keyClusterAddress)) {
        LogPrintf("DockerUpdateService::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerUpdateService::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool DockerUpdateService::CheckSignature(CPubKey& pubKeyClusterAddress)
{
    // TODO: add sentinel data
    std::string strMessage = vin.ToString() + boost::lexical_cast<std::string>(version) + n2n_community +
        serviceid + serviceName + image + gpuname + ssh_pubkey + boost::lexical_cast<std::string>(cpu) + boost::lexical_cast<std::string>(gpu) +
        boost::lexical_cast<std::string>(memory_byte) + boost::lexical_cast<std::string>(sigTime);

    std::string strError = "";

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerUpdateService::CheckSignature -- Got bad signature, error: %s\n", strError);
        return false;
    }
    return true;
}
std::string dockerservicefilter::ToJsonString(){
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
    LogPrint("docker","Service::DockerServiceList docker json node %s\n",serviceData);
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

void Service::DockerServiceInspect(const string& serviceData,std::map<std::string,Service> &services)
{
    LogPrint("docker","Service::DockerServiceInspect docker json node %s\n",serviceData);
    try{
        UniValue data(UniValue::VOBJ);
        if(!data.read(serviceData)){
            LogPrint("docker","Service::DockerServiceInspect docker json error\n");
            return;
        }
            Service service;
            bool fSuccess = DcokerServiceJson(data,service);
            if(fSuccess)
                services[service.ID]=service;
    }catch(std::exception& e){
        LogPrint("docker","Service::DockerServiceInspect JSON read error,%s\n",string(e.what()).c_str());
    }catch(...){
        LogPrint("docker","Service::DockerServiceInspect unkonw exception\n");
    }
}
UniValue Service::DockerServToJson(Service &service)
{

    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("ID",service.ID));
        data.push_back(Pair("CreatedAt",service.createdAt));
        data.push_back(Pair("UpdatedAt",service.updatedAt));
    }
    {
        UniValue obj(UniValue::VOBJ);
        
        obj=ServVerToJson(service.version);
        data.push_back(Pair("Version", obj));

        obj=SerSpecToJson(service.spec);
        data.push_back(Pair("Spec",obj));
        
        obj=EndpointToJson(service.endpoint);
        data.push_back(Pair("Endpoint", obj));
        
        obj=UpdateStatusToJson(service.updateStatus);
        data.push_back(Pair("UpdateStatus", obj));
    }
    return data;
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
UniValue Service::ServVerToJson(Config::Version &version)
{
    UniValue data(UniValue::VOBJ);
    data.push_back(Pair("Index", version.index));
    return data;
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
UniValue Service::SerSpecToJson(Config::ServiceSpec &spec)
{
    UniValue data(UniValue::VOBJ);
    data.push_back(Pair("Name",spec.name));
    {
        UniValue obj(UniValue::VOBJ);
        
        obj=SpecLabelsToJson(spec.labels);
        data.push_back(Pair("Labels", obj));
        
        obj=TaskTemplateToJson(spec.taskTemplate);
        data.push_back(Pair("TaskTemplate", obj));
        
        obj=ModeToJson(spec.mode);
        data.push_back(Pair("Mode", obj));
        
        obj=UpdateConfigToJson(spec.updateConfig);
        data.push_back(Pair("UpdateConfig", obj));

        obj=UpdateConfigToJson(spec.rollbackConfig);
        data.push_back(Pair("RollbackConfig", obj));

        vector<Config::NetWork> networks=spec.networks;
        UniValue arryNet(UniValue::VARR);
        for(size_t j=0;j<networks.size();j++){
            UniValue objNet(UniValue::VOBJ);
            objNet=NetworkToJson(networks[j]);
            arryNet.push_back(objNet);
        }
        data.push_back(Pair("Networks", arryNet));

        obj=EndpointSpecToJson(spec.endpointSpec);
        data.push_back(Pair("EndpointSpec", obj));
    }
    return data;
}

void Service::ParseSpecLabels(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            labels.insert(std::make_pair(vKeys[i],tdata.get_str()));
        }
    }
}
UniValue Service::SpecLabelsToJson(Config::Labels &labels)
{
    UniValue data(UniValue::VOBJ);

    for(auto &label: labels){
         data.push_back(Pair(label.first,label.second));
    }
    return data;
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
UniValue Service::TaskTemplateToJson(Config::TaskSpec &taskTemplate)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Runtime",taskTemplate.runtime));
        data.push_back(Pair("ForceUpdate",taskTemplate.forceUpdate));    
    }
    {
        UniValue obj(UniValue::VOBJ);
        obj=ContainerSpecToJson(taskTemplate.containerSpec);
        data.push_back(Pair("ContainerSpec",obj));

        obj=ResourceToJson(taskTemplate.resources);
        data.push_back(Pair("Resources",obj));

        obj=RestartPolicyToJson(taskTemplate.restartPolicy);
        data.push_back(Pair("RestartPolicy",obj));

        obj=PlacementToJson(taskTemplate.placement);
        data.push_back(Pair("Placement",obj));

        obj=LogDriverToJson(taskTemplate.logdriver);
        data.push_back(Pair("LogDriver",obj));

    }
    {
        vector<Config::NetWork> networks=taskTemplate.netWorks;
        UniValue arryNet(UniValue::VARR);
        for(size_t j=0;j<networks.size();j++){
            UniValue objNet(UniValue::VOBJ);
            objNet=NetworkToJson(networks[j]);
            arryNet.push_back(objNet);
        }
        data.push_back(Pair("Networks", arryNet));
    }
     return data;

}

void Service::ParseResource(const UniValue& data,Config::Resource &resource)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Limits"){
                ParseResourceObj(tdata,resource.limits);
            }else if(vKeys[i]=="Reservations"){
                ParseResourceObj(tdata,resource.reservations);
            }
        }
    }
}
UniValue Service::ResourceToJson(Config::Resource &resource)
{
    UniValue data(UniValue::VOBJ);
    {
        UniValue objLimit(UniValue::VOBJ);
        objLimit=ResourceObjToJson(resource.limits);
        data.push_back(Pair("Limits",objLimit));

        UniValue objRev(UniValue::VOBJ);
        objRev=ResourceObjToJson(resource.reservations);
        data.push_back(Pair("Reservations",objRev));
    }
    return data;
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
        if(data[vKeys[i]].isArray()){
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
UniValue Service::ResourceObjToJson(Config::ResourceObj &resources)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("NanoCPUs",resources.nanoCPUs));
        data.push_back(Pair("MemoryBytes",resources.memoryBytes));
    }
    {
        vector<Config::GenericResources> genres=resources.genericResources;
        UniValue arryGenRev(UniValue::VARR);
        for(size_t j=0;j<genres.size();j++){
            UniValue objGenRev(UniValue::VOBJ);
            objGenRev=ResGenResToJson(genres[j]);
            arryGenRev.push_back(objGenRev);
        }
        data.push_back(Pair("GenericResources",arryGenRev));
    }
    return data;
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
UniValue Service::ResGenResToJson( Config::GenericResources &genResources)
{
    UniValue data(UniValue::VOBJ);
    {
        UniValue objNameSpec(UniValue::VOBJ);
        objNameSpec=ResGenNameSpecToJson(genResources.namedResourceSpec);
        data.push_back(Pair("NamedResourceSpec",objNameSpec));

        UniValue objDiscSpec(UniValue::VOBJ);
        objDiscSpec=ResGenDiscSpecToJson(genResources.discreateResourceSpec);
        data.push_back(Pair("DiscreteResourceSpec",objDiscSpec));
    }
    return data;
}
void Service::ParseResGenNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec)
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
UniValue Service::ResGenNameSpecToJson(Config::NamedResourceSpec &namedResourceSpec)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Kind",namedResourceSpec.kind));
        data.push_back(Pair("Value",namedResourceSpec.value));
    }
    return data;
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
            if(vKeys[i]=="Value"){
                discResourceSpec.value=tdata.get_int64();
            }
        }
    }
}
UniValue Service::ResGenDiscSpecToJson(Config::DiscreteResourceSpec &discResourceSpec)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Kind",discResourceSpec.kind));
        data.push_back(Pair("Value",discResourceSpec.value));
    }
    return data;
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
UniValue Service::RestartPolicyToJson(Config::RestartPolicy &repoly)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Condition",repoly.condition));
        data.push_back(Pair("MaxAttempts",repoly.maxAttempts));
        data.push_back(Pair("Delay",repoly.delay));
        data.push_back(Pair("Window",repoly.window));
    }
    return data;
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
UniValue Service::PlacementToJson(Config::Placement &placement)
{
    UniValue data(UniValue::VOBJ);
    {
        UniValue arryConst(UniValue::VARR);
        arryConst=ArryToJson(placement.constraints);
        data.push_back(Pair("Constraints",arryConst));
    }
    {
        vector<Config::Preferences> preference=placement.preferences;
        UniValue arryPrefer(UniValue::VARR);
        for(size_t j=0;j<preference.size();j++){
            UniValue objPrefer(UniValue::VOBJ);
            objPrefer=PreferencesToJson(preference[j]);
            arryPrefer.push_back(objPrefer);
        }
        data.push_back(Pair("Preferences",arryPrefer));

        vector<Config::Platform> platform=placement.platforms;
        UniValue arryPlat(UniValue::VARR);
        for(size_t j=0;j<platform.size();j++){
            UniValue objPlat(UniValue::VOBJ);
            objPlat=PlatformsToJson(platform[j]);
            arryPlat.push_back(objPlat);
        }
        data.push_back(Pair("Platforms",arryPlat));
    }
    return data;
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
UniValue Service::PreferencesToJson(Config::Preferences &preference)
{
    UniValue data(UniValue::VOBJ);
    {
        UniValue objSpread(UniValue::VOBJ);
        objSpread=PreferSpreadToJson(preference.spread);
        data.push_back(Pair("Spread",objSpread));
    }
    return data;
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
UniValue Service::PreferSpreadToJson(Config::Spread &spread)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("SpreadDescriptor",spread.spreadDescriptor));
    }
    return data;
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
UniValue Service::PlatformsToJson(Config::Platform &platform)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Architecture",platform.architecture));
        data.push_back(Pair("OS",platform.OS));
    }
    return data;
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
UniValue Service::NetworkToJson(Config::NetWork &network)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Target",network.target));

        UniValue arryAlia(UniValue::VARR);
        arryAlia=ArryToJson(network.aliases);
        data.push_back(Pair("Aliases",arryAlia));    
    }
    return data;
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
UniValue Service::LogDriverToJson(Config::LogDriver &logdriver)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Name",logdriver.name));
    }
    {
        UniValue objLogDri(UniValue::VOBJ);
        objLogDri=LogDriverOptToJson(logdriver.options);
        data.push_back(Pair("Options",objLogDri));
    }
    return data;
}
void Service::ParseLogDriverOpt(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            labels.insert(std::make_pair(vKeys[i],tdata.get_str()));
        }
    }
}

UniValue Service::LogDriverOptToJson(Config::Labels &labels)
{
    UniValue data(UniValue::VOBJ);
    {
        for(auto &iter: labels) {
            data.push_back(Pair(iter.first,iter.second));
        }
    }
    return data;
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
UniValue Service::ModeToJson(Config::Mode &mode)
{
    UniValue data(UniValue::VOBJ);
    {
        if(mode.global){
            data.push_back(Pair("Global",mode.global));
        }else{
            UniValue objRep(UniValue::VOBJ);
            objRep=ModeReplicatedToJson(mode.replicated);
            data.push_back(Pair("Replicated",objRep));
        }
    }
    return data;
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
UniValue Service::ModeReplicatedToJson(Config::Replicated &rep)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Replicas",rep.replicas));
    }
    return data;
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
UniValue Service::UpdateConfigToJson(Config::UpdateConfig &upconfig)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Parallelism",upconfig.parallelism));
        data.push_back(Pair("Delay",upconfig.delay));
        data.push_back(Pair("Monitor",upconfig.monitor));
        data.push_back(Pair("MaxFailureRatio",upconfig.maxFailureRatio));
        data.push_back(Pair("FailureAction",upconfig.failureAction));
        data.push_back(Pair("Order",upconfig.order));
    }
    return data;
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
UniValue Service::EndpointSpecToJson(Config::EndpointSpec &endpointSpec)
{
    UniValue data(UniValue::VOBJ);
    {
        vector<Config::EndpointPortConfig> port=endpointSpec.ports;
        UniValue arryPort(UniValue::VARR);
        for(size_t j=0;j<port.size();j++){
            UniValue objPort(UniValue::VOBJ);
            objPort=EndSpecPortToJson(port[j]);
            arryPort.push_back(objPort);
        }
        data.push_back(Pair("Ports",arryPort));
    }
    {
        data.push_back(Pair("Mode",endpointSpec.mode));
    }
    return data;
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

UniValue Service::EndSpecPortToJson(Config::EndpointPortConfig &port)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Name",port.name));
        data.push_back(Pair("Protocol",port.protocol));
        data.push_back(Pair("PublishMode",port.publishedPort));
        data.push_back(Pair("TargetPort",port.targetPort));
        data.push_back(Pair("PublishedPort",port.publishedPort));
    }
    return data;
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
UniValue Service::EndpointToJson(Config::Endpoint &endpoint)
{
    UniValue data(UniValue::VOBJ);
    {
        UniValue objSpec(UniValue::VOBJ);
        objSpec=EndpointSpecToJson(endpoint.spec);
        data.push_back(Pair("Spec",objSpec));
    }
    {
        vector<Config::EndpointPortConfig> port=endpoint.ports;
        UniValue arryPort(UniValue::VARR);
        for(size_t j=0;j<port.size();j++){
            UniValue objPort(UniValue::VOBJ);
            objPort=EndSpecPortToJson(port[j]);
            arryPort.push_back(objPort);
        }
        data.push_back(Pair("Ports",arryPort));

        vector<Config::VirtualIP> virtualip=endpoint.virtualIPs;
        UniValue arryVir(UniValue::VARR);
        for(size_t j=0;j<virtualip.size();j++){
            UniValue objVir(UniValue::VOBJ);
            objVir=VirtualIPsToJson(virtualip[j]);
            arryVir.push_back(objVir);
        }
        data.push_back(Pair("VirtualIPs",arryVir));
    }
    return data;
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
UniValue Service::VirtualIPsToJson(Config::VirtualIP &virtualip)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("NetworkID",virtualip.networkID));
        data.push_back(Pair("Addr",virtualip.addr));
    }
    return data;
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
UniValue Service::UpdateStatusToJson(Config::UpdateStatus &updateStatus)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("State",updateStatus.state));
        data.push_back(Pair("Message",updateStatus.message));
        data.push_back(Pair("StartedAt",updateStatus.createdAt));
        data.push_back(Pair("CompletedAt",updateStatus.completedAt));
    }
    return data;
}
UniValue Service::ArryToJson(std::vector<std::string> &strArry)
{
    UniValue arry(UniValue::VARR);
    for(size_t i=0;i<strArry.size();i++){
        arry.push_back(strArry[i]);
    }
    return arry;
}
std::string Service::ToJsonString()
{
    UniValue data(UniValue::VOBJ);
    data=Service::DockerServToJson(*this);
    return data.write();
}
std::string Config::ServiceSpec::ToJsonString()
{
    UniValue data(UniValue::VOBJ);
    data=Service::SerSpecToJson(*this);
    return data.write();
}
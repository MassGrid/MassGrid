#include "dockerservice.h"
#include "univalue.h"
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
    int idx;
    uint64_t createdTime;
    uint64_t updateTime;
    Config::ServiceSpec spc;
    Config::ServiceSpec previousSpec;
    Config::Endpoint endpoint;
    Config::UpdateStatus updateStatus;
    int version=DEFAULT_CTASK_API_VERSION;

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
                idx=find_value(tdata,"Index").get_int();
            }else if(vKeys[i]=="Spec"){
                ParseSpec(tdata,spc);
            }else if(vKeys[i]=="PreviousSpec"){
                ParseSpec(tdata,previousSpec);
            }else if(vKeys[i]=="Endpoint"){
                ParseEndpoint(tdata,endpoint);
            }else if(vKeys[i]=="UpdateStatus"){
                ParseUpdateStatus(tdata,updateStatus);
            }
        }
    }
    service=Service(id,idx,createdTime,updateTime,spc,previousSpec,endpoint,updateStatus,version);
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
            if(vKeys[i]=="ForceUpdate") taskTemplate.forceUpdate=tdata.get_int();
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
void Service::ParseContainerSpec(const UniValue& data,Config::ContainerSpec &contTemp)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Isolation") 
                contTemp.isolation=tdata.get_str();
            if(vKeys[i]=="Image") 
                contTemp.image=tdata.get_str();
        }
        if(tdata.isObject()){
            if(vKeys[i]=="Labels"){
                ParseSpecContainerLabels(tdata,contTemp.labels);
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Env"){
                ParseArray(tdata,contTemp.env);
            }else if(vKeys[i]=="Mounts"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::Mount mount;
                    ParseMount(tdata[j],mount);
                    contTemp.mounts.push_back(mount);
                }
            }
        }
    }
}
void Service::ParseSpecContainerLabels(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="key") labels.insert(std::make_pair("com.massgrid.key",tdata.get_str()));
        }
    }
}
void Service::ParseMount(const UniValue& data,Config::Mount &mount)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Type") mount.type=tdata.get_str();
            else if(vKeys[i]=="Source") mount.source=tdata.get_str();
            else if(vKeys[i]=="Target") mount.target=tdata.get_str();
        }
    }
}
void Service::ParseArray(const UniValue& data,vector<std::string> &array)
{
    for(size_t i=0;i<data.size();i++){
        array.push_back(data[i].get_str());
    }
    return;
} 
void Service::ParseResource(const UniValue& data,Config::Resource &resources)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Limits"){
                ParseLimits(tdata,resources.limits);
            }else if(vKeys[i]=="Reservations"){
                ParseReservation(tdata,resources.reservations);
            }
        }
    }
}
void Service::ParseLimits(const UniValue& data,Config::Limits &limits)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="NanoCPUs"){
               limits.nanoCPUs=tdata.get_int64();
            }else if(vKeys[i]=="MemoryBytes"){
                limits.memoryBytes=tdata.get_int64();
            }
        }
    }
}
void Service::ParseReservation(const UniValue& data,Config::Reservation &reservations)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="GenericResources"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::DiscreteResourceSpec disres;
                    ParseDirReservation(tdata[j],disres);
                    reservations.Reservations.push_back(disres);
                }
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="NanoCPUs"){
                reservations.nanoCPUs=tdata.get_int64();
            }else if(vKeys[i]=="MemoryBytes"){
                reservations.memoryBytes=tdata.get_int64();
            }
        }
    }
}
void Service::ParseDirReservation(const UniValue& data,Config::DiscreteResourceSpec &disres)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Kind"){
               disres.kind=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Value"){
                disres.value=tdata.get_int();
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
                repoly.MmaxAttempts=tdata.get_int();
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
void Service::ParseMode(const UniValue& data,Config::Mode &mode)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Replicated"){
                // Config::Replicated rep;
                ParseReplicated(tdata,mode.replicated);
            }
        }
    }
}
void Service::ParseReplicated(const UniValue& data,Config::Replicated &rep)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Replicas"){
                rep.replicas=tdata.get_int();
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
            if(vKeys[i]=="Protocol"){
                port.protocol=tdata.get_str();
            }else if(vKeys[i]=="PublishMode"){
                port.publishMode=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="TargetPort"){
                port.targetPort=tdata.get_int();
            }else if(vKeys[i]=="PublishedPort"){
                port.publishedPort=tdata.get_int();
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
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="StartedAt"){
                updateStatus.startedAt=tdata.get_int64();
            }else if(vKeys[i]=="CompletedAt"){
                updateStatus.completedAt =tdata.get_int64();
            }
        }
    }
}
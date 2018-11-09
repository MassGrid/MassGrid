#include "dockertask.h"

void dockertask(const string& taskData, std::vector<Task> &tasks)
{
    std::cout<<"docker json task"<<std::endl;
    try{
        UniValue dataArry(UniValue::VARR);
        if(!dataArry.read(taskData)){
            std::cout<<"json error\n";
            return;
        }
        for(size_t i=0;i<dataArry.size();i++){
            UniValue data(dataArry[i]);
            Task *task=DcokerTaskJson(data);
            tasks.push_back(*task);
            // break;
        }
     }catch(std::exception& e){
        std::cout<<string(e.what())<<std::endl;
    }catch(...){
        std::cout<<"unkonw exception"<<std::endl;
    }
}
Task *DcokerTaskJson(const UniValue& data)
{
    std::string id;
    int idx;
    uint64_t createdTime;
    uint64_t updateTime;
    vector<pair<std::string, std::string> > lab;
    Config::TaskSpec spec;
    std::string serviceid;
    uint slot;
    std::string nodeid;
    Config::TaskStatus taskstatus;
    Config::eStatus desiredstate;
    vector<Config::NetworkTemplate> networksattachments;
    vector<Config::NamedResourceSpec> genericresources;
    int version=DEFAULT_CTASK_API_VERSION;

    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID"){
                id=data[vKeys[i]].get_str();
            }else if(vKeys[i]=="CreatedAt"){
                createdTime=getDockerTime(tdata.get_str());
            }else if(vKeys[i]=="UpdatedAt"){
                updateTime=getDockerTime(tdata.get_str());
            }else if(vKeys[i]=="ServiceID"){
                serviceid=tdata.get_str();
            }else if(vKeys[i]=="NodeID"){
                nodeid=tdata.get_str();
            }else if(vKeys[i]=="DesiredState"){
                desiredstate=Config::eStatus(getTaskStatus(tdata.get_str()));
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Slot")
            slot=tdata.get_int();
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Version") 
                idx=find_value(tdata,"Index").get_int();
            else if(vKeys[i]=="Status"){
                ParseTaskStatus(tdata,taskstatus);
            }else if(vKeys[i]=="Spec"){
                ParseTaskSpec(tdata,spec);
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="NetworksAttachments"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::NetworkTemplate networktemp;
                    ParseTaskNetworkTemplate(tdata[j],networktemp);
                    networksattachments.push_back(networktemp);
                }
            }else if(vKeys[i]=="GenericResources"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::NamedResourceSpec namerespec;
                    ParseTaskGenericResources(tdata[j],namerespec);
                    genericresources.push_back(namerespec);
                }
            }
        }
    }
    return new Task(id,idx,createdTime,updateTime,lab,spec,serviceid,slot,nodeid,taskstatus,desiredstate,networksattachments,genericresources,version);
}
void ParseTaskStatus(const UniValue& data,Config::TaskStatus &taskstaus)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="ContainerStatus"){
                ParseTaskContainerStatus(tdata,taskstaus.containerStatus);
            }
        }
    }
}
void ParseTaskContainerStatus(const UniValue& data, Config::ContainerStatus &contstatus)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ContainerID"){
                contstatus.containerID=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="PID"){
                contstatus.PID=tdata.get_int();
            }else if(vKeys[i]=="ExitCode"){
                contstatus.exitCode=tdata.get_int();
            }
        }
    }
}
void ParseTaskNetworkTemplate(const UniValue& data,Config::NetworkTemplate &networktemp)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Network"){
                ParseTaskNetwork(tdata,networktemp);
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Addresses"){
                ParseTaskArray(tdata,networktemp.addresses);
            }
        }
    }
}
void ParseTaskNetwork(const UniValue& data,Config::NetworkTemplate &networktemp)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID") networktemp.ID=tdata.get_str();
            else if(vKeys[i]=="CreatedAt") networktemp.createdAt=getDockerTime(tdata.get_str());
            else if(vKeys[i]=="UpdatedAt") networktemp.updatedAt=getDockerTime(tdata.get_str());
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Version") 
                networktemp.index=find_value(tdata,"Index").get_int();
            else if(vKeys[i]=="Spec"){
                ParseTaskNetWorkSpec(tdata,networktemp.Spec);
            }else if(vKeys[i]=="DriverState"){
                ParseTaskDriverState(tdata,networktemp.driverState);
            }else if(vKeys[i]=="IPAMOptions"){
                ParseTaskIPAMOptions(tdata,networktemp.IPAMOptions);
            }
        }
    }
}
void ParseTaskNetWorkSpec(const UniValue& data, Config::NetWorkSpec &Spec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Name"){
                Spec.name=tdata.get_str();
            }else if(vKeys[i]=="Labels"){
                ParseTaskLabels(tdata,Spec.labels);
            }else if(vKeys[i]=="DriverConfiguration"){
                ParseTaskLabels(tdata,Spec.driverConfiguration);
            }else if(vKeys[i]=="Scope"){
                Spec.IPAMOptions.scope=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isBool()){
            if(vKeys[i]=="Ingress"){
                Spec.ingress=tdata.get_bool();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="IPAMOptions"){
                ParseTaskIPAMOptions(tdata,Spec.IPAMOptions);
            }
        }
    }
}
void ParseTaskLabels(const UniValue& data,vector<std::string> &array)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        array.push_back(data[vKeys[i]].get_str());
    }
}
void ParseTaskIPAMOptions(const UniValue& data,Config::IPAMOption &ipamoption)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Driver"){
                ParseTaskIPMOPDriver(tdata,ipamoption.driver);
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Configs"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::ConfigIP configip;
                    ParseTaskIPMOPConfigs(tdata[j],configip);
                    ipamoption.configip.push_back(configip);
                }
            }
        }
    }
}
void ParseTaskIPMOPDriver(const UniValue& data,Config::Driver &driver)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Name"){
                driver.name=tdata.get_str();
            }
        }
    }
}
void ParseTaskIPMOPConfigs(const UniValue& data,Config::ConfigIP &configip)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Subnet"){
                configip.subnet =tdata.get_str();
            }else if(vKeys[i]=="Gateway"){
                configip.gateway=tdata.get_str();
            }
        }
    }
}
void ParseTaskDriverState(const UniValue& data,Config::DriverState &drivstat)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Name"){
                drivstat.name =tdata.get_str();
            }
        }
    }
}
void ParseTaskGenericResources(const UniValue& data, Config::NamedResourceSpec &namerespec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="NamedResourceSpec"){
                ParseTaskGenericResourcesName(tdata,namerespec);
            }
        }
    }
}
void ParseTaskGenericResourcesName(const UniValue& data, Config::NamedResourceSpec &namerespec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Kind"){
                namerespec.kind =tdata.get_str();
            }else if(vKeys[i]=="Value"){
                namerespec.value =tdata.get_str();
            }
        }
    }
}
void ParseTaskSpec(const UniValue& data, Config::TaskSpec &taskspec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="ForceUpdate"){
                taskspec.forceUpdate =tdata.get_int();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="ContainerSpec"){
                ParseTaskSpecContainer(tdata,taskspec.containerTemplate);
            }else if(vKeys[i]=="Resources"){
                ParseTaskSpecResources(tdata,taskspec.resources);
            }else if(vKeys[i]=="RestartPolicy"){
                ParseTaskSpecRestartPolicy(tdata,taskspec.restartPolicy);
            }else if(vKeys[i]=="Placement"){
                ParseTaskSpecPlacement(tdata,taskspec.placement);
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Networks"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::NetWork network;
                    ParseTaskSpecNetWorks(tdata[j],network);
                    taskspec.netWorks.push_back(network);
                }
            }
        }
    } 
}
void ParseTaskSpecContainer(const UniValue& data,  Config::ContainerTemplate &conttemp)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Image"){
                conttemp.image  =tdata.get_str();
            }else if(vKeys[i]=="Isolation"){
                conttemp.isolation=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Env"){
                ParseTaskArray(tdata,conttemp.env);
            }else if(vKeys[i]=="Mounts"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::Mount mount;
                    ParseTaseSpecMount(tdata[j],mount);
                    conttemp.mounts.push_back(mount);
                }
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Labels"){
                ParseTaskLabels(tdata,conttemp.labels);
            }
        }
    }
}
void ParseTaseSpecMount(const UniValue& data,Config::Mount &mount)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Type"){
                mount.type  =tdata.get_str();
            }else if(vKeys[i]=="Source"){
                mount.source =tdata.get_str();
            }else if(vKeys[i]=="Target"){
                mount.target=tdata.get_str();
            }
        }
    }   
}
void ParseTaskSpecResources(const UniValue& data,Config::Resource &resources)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Limits"){
                ParseTaskSpecLimits(tdata,resources.limits);
            }else if(vKeys[i]=="Reservations"){
                ParseTaskSpecReserv(tdata,resources.reservations);
            }
        }
    }    
}
void ParseTaskSpecLimits(const UniValue& data,Config::Limits &limits)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        if(data[vKeys[i]].isNum()){
            UniValue tdata(data[vKeys[i]]);
            if(vKeys[i]=="NanoCPUs"){
               limits.nanoCPUs=tdata.get_int64();
            }else if(vKeys[i]=="MemoryBytes"){
                limits.memoryBytes=tdata.get_int64();
            }
        }
    }
}
void ParseTaskSpecReserv(const UniValue& data,Config::Reservation &reserv)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="NanoCPUs"){
                reserv.nanoCPUs=tdata.get_int64();
            }else if(vKeys[i]=="MemoryBytes"){
                reserv.memoryBytes=tdata.get_int64();
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="GenericResources"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::DiscreteResourceSpec disreserv;
                    ParseTaseSpecGenericRe(tdata[j],disreserv);
                    reserv.Reservations.push_back(disreserv);
                }
            }
        }
    }
}
void ParseTaseSpecGenericRe(const UniValue& data, Config::DiscreteResourceSpec &disreserv)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="DiscreteResourceSpec"){
                ParseTaseSpecDisGenericRe(tdata,disreserv);
            }
        }
    }
}
void ParseTaseSpecDisGenericRe(const UniValue& data, Config::DiscreteResourceSpec &disreserv)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Kind"){
                disreserv.kind=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Value"){
                disreserv.value=tdata.get_int();
            }
        }
    }
}
void ParseTaskSpecRestartPolicy(const UniValue& data,Config::RestartPolicy &repolicy)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Condition"){
                repolicy.condition =tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="MaxAttempts"){
                repolicy.MmaxAttempts =tdata.get_int();
            }
        }
    }
}
void ParseTaskSpecPlacement(const UniValue& data,Config::Placement &placement)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Condition"){
                ParseTaskArray(tdata,placement.constraints);
            }else if(vKeys[i]=="Platforms"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::Platform platform;
                    ParseTaseSpecPlatforms(tdata[j],platform);
                    placement.platforms.push_back(platform);
                }
            }
        }
    }
}
void ParseTaseSpecPlatforms(const UniValue& data,Config::Platform &platform)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Architecture"){
                platform.architecture=tdata.get_str();
            }else if(vKeys[i]=="OS"){
                platform.OS=tdata.get_str();
            }
        }
    }
}
void ParseTaskSpecNetWorks(const UniValue& data,Config::NetWork &network)
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
                ParseTaskArray(tdata,network.aliases);
            }
        }
    }
}
void ParseTaskArray(const UniValue& data,vector<std::string> &array)
{
    for(size_t i=0;i<data.size();i++){
        array.push_back(data[i].get_str());
    }
    return;
} 
int getTaskStatus(const std::string& status)
{
    size_t j=7;
    for(size_t i=0;i<j;i++){
        if(strcmp(status.c_str(),Config::strStatus[i])==0) return i;
    }
    return 0;
}
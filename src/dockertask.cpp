#include "dockertask.h"
#include "univalue.h"
namespace Config{
    const char* strTaskState[]={"new", "allocated","pending","assigned", 
                            "accepted","preparing","ready","starting",
                            "running","complete","shutdown","failed",
                            "rejected","remove","orphaned"};
};
std::string dockertaskfilter::ToJsonString(){
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
    if(!nodeid.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter = nodeid.begin();iter!=nodeid.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("node",arr));
    }
    if(!serviceid.empty()){
        UniValue arr(UniValue::VARR);
        for(vector<std::string>::iterator iter = serviceid.begin();iter!=serviceid.end();++iter)
            arr.push_back(*iter);
        data.push_back(Pair("service",arr));
    }
    {
        UniValue arr(UniValue::VARR);
        if(DesiredState_accepted){
            arr.push_back("accepted"); 
        }
        if(DesiredState_running){
            arr.push_back("running"); 
        }
        if(DesiredState_shutdown){
            arr.push_back("shutdown"); 
        }
        data.push_back(Pair("desired-state",arr));
    }

    return data.write();
}
void Task::TaskListUpdateAll(const string& taskData, std::map<std::string,Task> &tasks)
{
    try{
        UniValue dataArry(UniValue::VARR);
        if(!dataArry.read(taskData)){
            LogPrint("docker","Task::TaskListUpdateAll docker json error\n");
            return;
        }

        for(size_t i=0;i<dataArry.size();i++){
            UniValue data(dataArry[i]);
            string id = find_value(data,"ID").get_str();
            UniValue tdata = find_value(data,"Version").get_obj();
            int64_t index = find_value(tdata,"Index").get_int64();
            auto it = tasks.find(id);
            if(it!=tasks.end() && it->second.version.index == index)    //if the elem existed and needn't update
                continue;
            Task task;
            bool fSuccess = Task::DecodeFromJson(data,task);
            if(fSuccess)
                tasks[task.ID]=task;
        }
    }catch(std::exception& e){
        LogPrint("docker","Task::TaskListUpdateAll JSON read error,%s\n",string(e.what()).c_str());
    }catch(...){
        LogPrint("docker","Task::TaskListUpdateAll unkonw exception\n");
    }
}
bool Task::DecodeFromJson(const UniValue& data,Task& task)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID"){
                task.ID=tdata.get_str();
            }else if(vKeys[i]=="CreatedAt"){
                task.createdAt=getDockerTime(tdata.get_str());
            }else if(vKeys[i]=="UpdatedAt"){
                task.updatedAt=getDockerTime(tdata.get_str());
            }else if(vKeys[i]=="Name"){
                task.name=tdata.get_str();
            }else if(vKeys[i]=="ServiceID"){
                task.serviceID=tdata.get_str();
            }else if(vKeys[i]=="NodeID"){
                task.nodeID=tdata.get_str();
            }else if(vKeys[i]=="DesiredState"){
                task.desiredState=GetTaskStatus(tdata.get_str());
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Slot")
                task.slot=tdata.get_int();
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Version"){
                task.version.index=find_value(tdata,"Index").get_int64();
            }else if(vKeys[i]=="Labels"){//key?
                ParseTaskLabels(tdata,task.labels);
            }else if(vKeys[i]=="Spec"){
                ParseTaskTemplateSpec(tdata,task.spec);
            }else if(vKeys[i]=="Status"){
                ParseTaskStatus(tdata,task.status);
            }else if(vKeys[i]=="AssignedGenericResources"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::GenericResources genspec;
                    ParseGenericResources(tdata[j],genspec);
                    task.genericResources.push_back(genspec);
                }
            }
        }
    }
    return true;
}
void Task::ParseTaskLabels(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            labels.insert(std::make_pair(vKeys[i],tdata.get_str()));
        }
    }
}
void Task::ParseTaskTemplateSpec(const UniValue& data,Config::TaskSpec &taskTemplate)
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
//containnerspec

void Task::ParseResource(const UniValue& data,Config::Resource &resource)
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
void Task::ParseResourceObj(const UniValue& data, Config::ResourceObj &resources)
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
                    ParseGenericResources(tdata[j],genResources);
                    resources.genericResources.push_back(genResources);
                }
            }
        }
    }
}
void Task::ParseGenericResources(const UniValue& data, Config::GenericResources &genResources)
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
void Task::ParseResGenNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec)
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
void Task::ParseResGenDiscSpec(const UniValue& data, Config::DiscreteResourceSpec &discResourceSpec)
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
void Task::ParseRestartPolicy(const UniValue& data,Config::RestartPolicy &repoly)
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
void Task::ParsePlacement(const UniValue& data,Config::Placement &placement)
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
void Task::ParsePreferences(const UniValue& data,Config::Preferences &preference)
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
void Task::ParsePreferencesSpread(const UniValue& data,Config::Spread &spread)
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
void Task::ParsePlatforms(const UniValue& data,Config::Platform &platform)
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
void Task::ParseNetwork(const UniValue& data,Config::NetWork &network)
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
void Task::ParseLogDriver(const UniValue& data,Config::LogDriver &logdriver)
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
void Task::ParseLogDriverOpt(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            labels.insert(std::make_pair(vKeys[i],tdata.get_str()));
        }
    }
}
void Task::ParseTaskStatus(const UniValue& data,Config::TaskStatus &taskstaus)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="ContainerStatus"){
                ParseTaskContainerStatus(tdata,taskstaus.containerStatus);
            }
        }
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Timestamp"){
                taskstaus.timeStamp=TimeestampStr(tdata.get_str().c_str());
            }else if(vKeys[i]=="State"){
                taskstaus.state=GetTaskStatus(tdata.get_str());
            }else if(vKeys[i]=="Message"){
                taskstaus.message=tdata.get_str();
            }else if(vKeys[i]=="Err"){
                taskstaus.err=tdata.get_str();
            }
        }
    }
}
void Task::ParseTaskContainerStatus(const UniValue& data, Config::ContainerStatus &contstatus)
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
void Task::ParseGenResources(const UniValue& data, Config::GenericResources &genResources)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="NamedResourceSpec"){
                ParseGenResNameSpec(tdata,genResources.namedResourceSpec);
            }else if(vKeys[i]=="DiscreteResourceSpec"){
                ParseGenResDiscSpec(tdata,genResources.discreateResourceSpec);
            }
        }
    }
}
void Task::ParseGenResNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec)
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
void Task::ParseGenResDiscSpec(const UniValue& data, Config::DiscreteResourceSpec &discResourceSpec)
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

int Task::GetTaskStatus(std::string strType)
{
    int type=-1;
    for (int i = 0; i < ARRAYLEN(Config::strTaskState); i++){
        if (strType == Config::strTaskState[i]){
            type = i;
            break;
        }
    }
    return type;
}
// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "dockerman.h"
#include "http.h"
#include "init.h"
#include "utiltime.h"
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <queue>
static CCriticalSection cs_serInfoQueue;
CDockerMan dockerman;
std::map<std::string,Node> mapNodeLists;    //temp
std::map<std::string,Service> mapServiceLists;
std::map<std::string,Task> mapTaskLists;
std::priority_queue<ServiceListInfo, std::vector<ServiceListInfo> > serviceInfoQue;
std::string IpSet::GetFreeIP(){
    for(int i = 1;i < MAX_SIZE;++i)
        for(int j = 5;j < MAX_SIZE;j+=5){
            std::string t;
            t = ip_start + boost::lexical_cast<std::string>(i) + "." + boost::lexical_cast<std::string>(j);
            in_addr_t ipaddr = inet_addr(t.c_str());
            if(!ip_set.count(ipaddr)){
                ip_set.insert(ipaddr);
                return t;
            }
        }
}

void IpSet::Insert(std::string str){
    if(!IsVaild(str))
        return;
    in_addr_t ipaddr = inet_addr(str.c_str());
    ip_set.insert(ipaddr);

}
bool IpSet::IsFree(std::string str){
    if(!IsVaild(str))
        return false;
    in_addr_t ipaddr = inet_addr(str.c_str());
    if(!ip_set.count(ipaddr))
        return true;
    return false;
}
void IpSet::Erase(std::string str){
    if(!IsVaild(str))
        return;
    in_addr_t ipaddr = inet_addr(str.c_str());
    auto it = ip_set.find(ipaddr);
    if(it!=ip_set.end())
        ip_set.erase(it);
}
bool IpSet::IsVaild(std::string s){
    vector<string> destination;
    boost::split(destination,s, boost::is_any_of( "." ), boost::token_compress_on );
    if(destination.size() == 4){
        int i = boost::lexical_cast<int>(destination[2]);
        int j = boost::lexical_cast<int>(destination[3]);
        if(i > 0 && i < 255 && j > 0 && j < 255)
            return true;
        return false;
    }
}
bool CDockerMan::PushMessage(Method mtd,std::string id,std::string pushdata,bool isClearService){
    LogPrint("docker","CDockerMan::PushMessage Started Method: %s\n",strMethod[mtd]);
    std::string url;
    HttpType type;
    switch (mtd)
    {
        case Method::METHOD_NODES_LISTS:
            url="/nodes";
            type=HttpType::HTTP_GET;
            if(!pushdata.empty()){
                pushdata = "filters=" + pushdata;
            }
            break;
        case Method::METHOD_NODES_INSPECT:
            url="/nodes/";
            url.append(id);
            type=HttpType::HTTP_GET;
            pushdata.clear();
            break;
        case Method::METHOD_NODES_DELETE:
            url="/nodes/";
            url.append(id);
            type=HttpType::HTTP_DELETE;
            pushdata.clear();
            break;
        case Method::METHOD_NODES_UPDATE:   // not implemented yet
            break;
        case Method::METHOD_SERVICES_LISTS:
            url="/services";
            type=HttpType::HTTP_GET;
            if(!pushdata.empty()){
                pushdata = "filters=" + pushdata;
            }
            break;
        case Method::METHOD_SERVICES_CREATE:
            url="/services/create";
            type=HttpType::HTTP_POST;
            break;
        case Method::METHOD_SERVICES_INSPECT:
            url="/services/";
            url.append(id);
            type=HttpType::HTTP_GET;
            pushdata.clear();
            break;
        case Method::METHOD_SERVICES_DELETE:
            url="/services/";
            url.append(id);
            type=HttpType::HTTP_DELETE;
            pushdata.clear();
            break;
        case Method::METHOD_SERVICES_UPDATE:
            url="/services/";
            url.append(id);
            url.append("/update");
            type=HttpType::HTTP_POST;
            break;
        case Method::METHOD_SERVICES_LOGS:
            url="/services/";
            url.append(id);
            url.append("logs");
            type=HttpType::HTTP_GET;
            pushdata.clear();
        case Method::METHOD_TASKS_LISTS:
            url="/tasks";
            type=HttpType::HTTP_GET;
            if(!pushdata.empty()){
                pushdata = "filters=" + pushdata;
            }
            break;
        case Method::METHOD_TASKS_INSPECT:
            url="/tasks/";
            url.append(id);
            type=HttpType::HTTP_GET;
            pushdata.clear();
            break;
        case Method::METHOD_SWARM_INSPECT:
            url="/swarm";
            type=HttpType::HTTP_GET;
            break;
        case Method::METHOD_INFO:
             url="/info";
            type=HttpType::HTTP_GET;
            break;
        case Method::METHOD_VERSION:
            url="/version";
            type=HttpType::HTTP_GET;
            break;      
        default:
            LogPrint("docker","CDockerMan::PushMessage not support this methed");
            return false;
    }
    HttpRequest http(address,apiPort,url,pushdata,"/var/run/docker.sock");
    LogPrint("docker","CDockerMan::RequestMessages Methed %s Send:%s\n",url,pushdata);
    int ret;
    if(type == HttpType::HTTP_GET){
        ret=http.HttpGet();
    }
    else if(type == HttpType::HTTP_DELETE){
        ret=http.HttpDelete();
    }
    else if(type == HttpType::HTTP_POST){
        ret=http.HttpPost();
    }
    else{
        // err
    }
    if(ret < 0) {
            LogPrint("docker","CDockerMan::RequestMessages Http_Error error_code: %d\n",ret);
            return false;
    }
    std::string reponseData=http.getReponseData();
    return ProcessMessage(mtd,http.url,ret,reponseData);
}

bool CDockerMan::ProcessMessage(Method mtd,std::string url,int ret,std::string responsedata,bool isClearService){
    LogPrint("docker","CDockerMan::ProcessMessage Started Method: %s ProcessMessage: %d Messages %s\n",strMethod[mtd],ret,responsedata);
    std::string strMessage;
    std::string id;
    HttpType type;
    if(ret != 200 && ret !=201){
        return false;
    }
    UniValue jsondata(UniValue::VOBJ);
    jsondata.read(responsedata);
    LOCK(cs);
    switch (mtd)
    {
        case Method::METHOD_NODES_LISTS:
        {
            mapNodeLists.clear();
            Node::DockerNodeList(responsedata,mapNodeLists);
            this->mapDockerNodeLists=mapNodeLists;
            break;
        }
        case Method::METHOD_NODES_INSPECT:  // not use
        {
            Node::DockerNodeList(responsedata,mapDockerNodeLists);
            break;
        }
        case Method::METHOD_NODES_DELETE:   // not implemented yet
        {
            id=url.substr(url.find_last_of("/")+1);
            mapDockerNodeLists.erase(mapDockerNodeLists.find(id));
            break;
        }
        case Method::METHOD_NODES_UPDATE:   // not implemented yet
            break;
        case Method::METHOD_SERVICES_LISTS:
        {
            if(isClearService)
                mapServiceLists.clear();
            Service::DockerServiceList(responsedata,mapServiceLists);

            if(isClearService)
                InitSerListQueue(mapServiceLists);

            dockertaskfilter taskfilter;
            // taskfilter.DesiredState_running=true;
            
            bool ret = PushMessage(Method::METHOD_TASKS_LISTS,"",taskfilter.ToJsonString());

            if(!ret)
                return false;
            break;
        }
        case Method::METHOD_SERVICES_CREATE:
        {
            if(jsondata.exists("ID")){
                id=jsondata["ID"].get_str();
                LogPrintf("CDockerMan::ProcessMessage ServiceCreate: %d Successful\n",id);
                PushMessage(Method::METHOD_SERVICES_INSPECT,id,"");
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage Not exist this ServiceId\n");
                return false;
            }
            break;
        }
        case Method::METHOD_SERVICES_INSPECT:
        {
                Service::DockerServiceInspect(responsedata,mapServiceLists);
            if(jsondata.exists("ID")){
                dockertaskfilter taskfilter;
                taskfilter.serviceid.push_back(jsondata["ID"].get_str());
                // taskfilter.DesiredState_running=true;
                bool ret = PushMessage(Method::METHOD_TASKS_LISTS,"",taskfilter.ToJsonString());

                UpdateSerListQueue(mapServiceLists,jsondata["ID"].get_str());

                if(!ret)
                    return false;
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage Not exist this ServiceId\n");
                return false;
            }    
            break;
        }
        case Method::METHOD_SERVICES_DELETE:
        {
            id=url.substr(url.find_last_of("/")+1);
            auto it = mapServiceLists.begin();
            if((it = mapServiceLists.find(id)) != mapServiceLists.end()){
                auto env =it->second.spec.taskTemplate.containerSpec.env;
                mapServiceLists.erase(it);
                for(auto itenv = env.begin();itenv!=env.end();++itenv){
                    if(itenv->find("N2N_SERVERIP=")!=-1){
                        string str=itenv->substr(13);
                        dockerman.serviceIp.Erase(str);
                        break;
                    }
                }
                this->mapDockerServiceLists=mapServiceLists; 
                LogPrint("docker","CDockerMan::ProcessMessage erase ServiceId %s\n",id);
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage not find ServiceId %s\n",id);
                return false;
            }
            break;
        }
        case Method::METHOD_SERVICES_UPDATE:
        {
            id=url.substr(url.find_last_of("/")+1);
            bool ret = PushMessage(Method::METHOD_SERVICES_INSPECT,id,"");
            if(!ret){
                return false;
                LogPrint("docker","CDockerMan::ProcessMessage update failed ServiceId %s\n",id);
            }
            LogPrint("docker","CDockerMan::ProcessMessage update successful ServiceId %s\n",id);
            break;
        }
        case Method::METHOD_SERVICES_LOGS:  // not implemented yet
        {
            url="/services/";
            url.append(id);
            url.append("logs");
            type=HttpType::HTTP_GET;
            break;
        }
        case Method::METHOD_TASKS_LISTS:
        {
            mapTaskLists.clear();
            Task::DockerTaskList(responsedata,mapTaskLists);
            set<std::string> countServiceID;
            for(auto it = mapTaskLists.begin();it != mapTaskLists.end();++it){
                if(mapServiceLists.find(it->second.serviceID)!= mapServiceLists.end()){
                    if(countServiceID.find(it->second.serviceID) == countServiceID.end()){    //clear tasklist
                        mapServiceLists[it->second.serviceID].mapDockerTasklists.clear();
                        countServiceID.insert(it->second.serviceID);
                    }
                    if(GetAdjustedTime() >= (mapServiceLists.find(it->second.serviceID)->second.createdAt + 180) && it->second.status.state != Config::TaskState::TASKSTATE_RUNNING){
                        LogPrint("docker","CDockerMan::ProcessMessage task id: %s, time: %lu, error: %s, message: %s\n ",it->second.serviceID,it->second.createdAt,it->second.status.err,it->second.status.message);
                        bool ret = dockerman.PushMessage(Method::METHOD_SERVICES_DELETE,it->second.serviceID,"");
                        if(ret)
                            LogPrint("docker","CDockerMan::ProcessMessage delete serviceid %s\n",it->second.serviceID);
                        
                    }else{
                        mapServiceLists[it->second.serviceID].mapDockerTasklists[it->first]=it->second;
                        LogPrint("docker","CDockerMan::ProcessMessage update successful ServiceId %s TaskId %s\n",it->second.serviceID,it->first);
                    }
                }
            }
            this->mapDockerServiceLists=mapServiceLists;
            break;
        }
        case Method::METHOD_TASKS_INSPECT:  //not implemented yet
        {
            Task::DockerTaskList(responsedata,mapTaskLists);
            break;
        }
        case Method::METHOD_SWARM_INSPECT:
        {
            Swarm::DockerSwarm(responsedata,swarm);
            break;
        }
        case Method::METHOD_INFO:   // not implemented yet
        {
            url="/info";
            type=HttpType::HTTP_GET;
            break;
        }
        case Method::METHOD_VERSION:    // not implemented yet
        {
            url="/version";
            type=HttpType::HTTP_GET;
            break;   
        }   
        default:
        {
            LogPrint("docker","CDockerMan::PushMessage not support this methed");
            return false;
        }
    }
    return true; 
}
void CDockerMan::UpdateIPfromServicelist(){
    serviceIp.Clear();
    for(auto it = mapDockerServiceLists.begin();it != mapDockerServiceLists.end();++it){
        auto env =it->second.spec.taskTemplate.containerSpec.env;
        for(auto itenv = env.begin();itenv!=env.end();++itenv){
            if(itenv->find("N2N_SERVERIP=")!=-1){
                string str=itenv->substr(13);
                serviceIp.Insert(str);
            }
        }
    }
}
bool CDockerMan::Update(){
    LOCK(cs);
    LogPrint("docker","CDockerMan::Update start\n");
    dockernodefilter ndfilter;

    if(!PushMessage(Method::METHOD_NODES_LISTS,"",ndfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_NODES_LISTS failed! \n");
        return false;
    }
    dockerservicefilter serfilter;
    if(!PushMessage(Method::METHOD_SERVICES_LISTS,"",serfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_SERVICES_LISTS failed! \n");
        return false;
    }
    if(!PushMessage(Method::METHOD_SWARM_INSPECT,"","")){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_SWARM_INSPECT failed! \n");
        return false;
    }
    GetVersionAndJoinToken();

    UpdateIPfromServicelist();
    LogPrint("docker","CDockerMan::Update Succcessful\n");
    return true;
}
bool CDockerMan::UpdateSwarmAndNodeList(){
    LOCK(cs);
    LogPrint("docker","CDockerMan::UpdateSwarmAndNodeList start\n");
    mapDockerNodeLists.clear();
    dockernodefilter ndfilter;

    if(!PushMessage(Method::METHOD_SWARM_INSPECT,"","")){
        LogPrint("docker","CDockerMan::UpdateSwarmAndNodeList ERROR Get METHOD_SWARM_INSPECT failed! \n");
        return false;
    }

    if(!PushMessage(Method::METHOD_NODES_LISTS,"",ndfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::UpdateSwarmAndNodeList ERROR Get METHOD_NODES_LISTS failed! \n");
        return false;
    }
    GetVersionAndJoinToken();
    LogPrint("docker","CDockerMan::UpdateSwarmAndNodeList Succcessful\n");
    return true;
}
bool CDockerMan::UpdateServicesList(){
    LOCK(cs);
    LogPrint("docker","CDockerMan::UpdateServicesList start\n");
    mapDockerServiceLists.clear();
    dockerservicefilter serfilter;
    if(!PushMessage(Method::METHOD_SERVICES_LISTS,"",serfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::UpdateServicesList ERROR Get METHOD_SERVICES_LISTS failed! \n");
        return false;
    }
    UpdateIPfromServicelist();
    LogPrint("docker","CDockerMan::UpdateServicesList Succcessful\n");

    return true;
}
bool CDockerMan::UpdateService(std::string serviceid){
    LOCK(cs);
    dockerservicefilter serfilter;
    LogPrint("docker","CDockerMan::UpdateService start\n");
    if(!PushMessage(Method::METHOD_SERVICES_INSPECT,serviceid,serfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::UpdateService ERROR Get METHOD_SERVICES_LISTS failed! \n");
        return false;
    }
    LogPrint("docker","CDockerMan::UpdateService Succcessful\n");
    return true;
}
uint64_t CDockerMan::GetDockerNodeCount(){
    return mapDockerNodeLists.size();
}
uint64_t CDockerMan::GetDockerNodeActiveCount(){
    uint64_t count = 0;
    for(map<std::string,Node>::iterator it=mapDockerNodeLists.begin();it!=mapDockerNodeLists.end();++it){
        if(it->second.status.state == Config::NodeStatusState::NODESTATUSSTATE_READY)
            ++count;
    }
    return count;
}
uint64_t CDockerMan::GetDockerServiceCount(){
    return mapDockerServiceLists.size();
}
uint64_t CDockerMan::GetDockerTaskCount(){
    int size=0;
    for(auto it = mapDockerServiceLists.begin();it != mapDockerServiceLists.end();++it ){
        size += it->second.mapDockerTasklists.size();
    }
    return size;
}
void CDockerMan::GetVersionAndJoinToken(){
    map<std::string,Node>::iterator it = mapDockerNodeLists.begin();
    for(;it!=mapDockerNodeLists.end();++it){
        if(it->second.spec.role == Config::Role::ROLE_MANAGER){  //example 18.06.1-ce
            vector<string> destination;
            boost::split(destination,it->second.description.engine.engineVersion, boost::is_any_of( ".-" ), boost::token_compress_on );
            for(int i = 0, j = 3;i < destination.size() && j >= 0;++i){
                try
                {
                    version.unv[j] = boost::lexical_cast<uint32_t>(destination[i]);
                }
                catch(const boost::bad_lexical_cast &)
                {
                    continue;
                }
                --j; 
            }

            JoinToken = swarm.joinWorkerTokens+" "+it->second.managerStatus.addr;   //set JoinToken
            managerAddr = it->second.managerStatus.addr.substr(0,it->second.managerStatus.addr.find_last_of(":"));
            return;
        }
    }
}

void InitSerListQueue(std::map<std::string,Service> &services)
{
    LOCK(cs_serInfoQueue);
    while(!serviceInfoQue.empty()) serviceInfoQue.pop();
    for(auto &service: services){
        ServiceListInfo serverinfo(14400);//7200
        serverinfo.serviceid=service.second.ID;
        serverinfo.timestamp=service.second.createdAt+serverinfo.timespan;
        serviceInfoQue.push(serverinfo);
    }
}
void UpdateSerListQueue(std::map<std::string,Service> &services,std::string id)
{
    LOCK(cs_serInfoQueue);
    for(auto &service: services){
        if(service.second.ID == id){
            ServiceListInfo serverinfo(14400);//7200
            serverinfo.serviceid=service.second.ID;
            serverinfo.timestamp=service.second.createdAt+serverinfo.timespan;
            serviceInfoQue.push(serverinfo);
        }
    }
}
void threadServiceControl()
{
    RenameThread("massgrid-sctrl");
    LogPrintf("ThreadServiceControl Start\n");

    while(true)
    {
        int64_t now_time=GetAdjustedTime();
        LogPrint("docker","threadServiceControl:: now time %lu\n",now_time);
        {
            LOCK(cs_serInfoQueue);
            while(!serviceInfoQue.empty()){
                    ServiceListInfo slist=serviceInfoQue.top();
                    if(now_time >= slist.timestamp){
                        dockerman.PushMessage(Method::METHOD_SERVICES_DELETE,slist.serviceid,"");
                        serviceInfoQue.pop();
                        LogPrint("docker","threadServiceControl:: delete serverice id= %s  timpstamp= %lu\n",slist.serviceid,slist.timestamp);
                    }else{
                        LogPrint("docker","threadServiceControl:: serviceInfoQue size= %d, the earliest serverice time= %lu id= %s\n",serviceInfoQue.size(),slist.timestamp,slist.serviceid);
                        break;
                    }
            }
        }
        // Check for stop or if block needs to be rebuilt
        for(int i=0;i<300;i++){
            boost::this_thread::interruption_point();
            MilliSleep(1000);
        }
    }
}
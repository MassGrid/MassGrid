// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <set>
#include <algorithm>
#include <queue>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "dockerman.h"
#include "http.h"
#include "init.h"
#include "utiltime.h"
#include "timermodule.h"
CDockerMan dockerman;
map<Item,Value_price> CDockerMan::GetPriceListFromNodelist(){
    std::map<std::string, Node> nodelist; 
    {
        LOCK(cs);
        nodelist = mapDockerNodeLists;
    }

    map<Item,Value_price> list;
    for(auto it=nodelist.begin();it!=nodelist.end();++it){
        if(it->second.isuseable == false)
            continue;
        Item item(it->second.engineInfo.cpu.Name,it->second.engineInfo.cpu.Count,it->second.engineInfo.mem.Name,it->second.engineInfo.mem.Count,it->second.engineInfo.gpu.Name,it->second.engineInfo.gpu.Count);
        if(list.count(item)){
            list[item].count++;
        }else
        {
            CAmount price = dockerPriceConfig.getPrice(item.cpu.Type,item.cpu.Name) * item.cpu.Count +
                dockerPriceConfig.getPrice(item.mem.Type,item.mem.Name) * item.mem.Count +
                dockerPriceConfig.getPrice(item.gpu.Type,item.gpu.Name) * item.gpu.Count;
            list[item] = Value_price(price,1);
        }
        LogPrint("docker","item :%s price %d sum %d\n",item.ToString(),list[item].price,list[item].count);
    }
    return list;
}
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
    return ProcessMessage(mtd,http.url,ret,reponseData,isClearService);
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
            if(isClearService)
                mapDockerNodeLists.clear();
            Node::NodeListUpdateAll(responsedata,mapDockerNodeLists);
            GetVersionAndJoinToken();
            break;
        }
        case Method::METHOD_NODES_INSPECT:  // not use
        {
            Node::NodeListUpdate(responsedata,mapDockerNodeLists);
            break;
        }
        case Method::METHOD_NODES_DELETE:   // not implemented yet
        {
            id=url.substr(url.find_last_of("/")+1);
            mapDockerNodeLists.erase(id);
            break;
        }
        case Method::METHOD_NODES_UPDATE:   // not implemented yet
            break;
        case Method::METHOD_SERVICES_LISTS:
        {
            //update servicelist

            if(isClearService)
                mapDockerServiceLists.clear();
            Service::ServiceListUpdateAll(responsedata,mapDockerServiceLists);

            if(isClearService){
                //update service ipset
                UpdateIPfromServicelist(mapDockerServiceLists);
                //Update service Timer
                timerModule.UpdateQueAll(mapDockerServiceLists);
            }

            dockertaskfilter taskfilter;
            
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
            if(jsondata.exists("ID")){
                Service::ServiceListUpdate(responsedata,mapDockerServiceLists);
                dockertaskfilter taskfilter;
                taskfilter.serviceid.push_back(jsondata["ID"].get_str());
                // taskfilter.DesiredState_running=true;
                PushMessage(Method::METHOD_TASKS_LISTS,"",taskfilter.ToJsonString());

                timerModule.UpdateQue(mapDockerServiceLists,jsondata["ID"].get_str());

                return true;
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
            auto it = mapDockerServiceLists.begin();
            if((it = mapDockerServiceLists.find(id)) != mapDockerServiceLists.end()){

                //  delete ip form set
                auto env =it->second.spec.taskTemplate.containerSpec.env;   
                for(auto itenv = env.begin();itenv!=env.end();++itenv){
                    if(itenv->find("N2N_SERVERIP=")!=-1){
                        string str=itenv->substr(13);
                        dockerman.serviceIp.Erase(str);
                        break;
                    }
                }
                // set node usage
                if(it->second.mapDockerTaskLists.size()){
                    string nodeid = it->second.mapDockerTaskLists.begin()->second.nodeID;
                    if(!nodeid.empty()){
                        auto it2 = mapDockerNodeLists.find(nodeid);
                        it2->second.isuseable=true;
                    }
                }
                //  delete service form map
                mapDockerServiceLists.erase(it);
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
            std::set<std::string> serviceidSet;
            Service::UpdateTaskList(responsedata,mapDockerServiceLists,serviceidSet); //updatealldate serviceid is null
            
            for(auto iter = serviceidSet.begin();iter != serviceidSet.end();++iter){
                auto it = mapDockerServiceLists.find(*iter);
                if(it->second.deleteTime <= GetAdjustedTime()) //when the task is always in pedding
                    timerModule.SetTlement(it->first); 
                if(it->second.mapDockerTaskLists.size()){
                    string nodeid =it->second.mapDockerTaskLists.begin()->second.nodeID;
                    if(!nodeid.empty()&&it->second.mapDockerTaskLists.begin()->second.status.state > Config::TaskState::TASKSTATE_PENDING&&
                            it->second.mapDockerTaskLists.begin()->second.status.state < Config::TaskState::TASKSTATE_SHUTDOWN){
                        mapDockerNodeLists[nodeid].isuseable=false;
                    }else
                    {
                        mapDockerNodeLists[nodeid].isuseable=true;
                    }
                }
            }
            break;
        }
        case Method::METHOD_TASKS_INSPECT:  //not implemented yet
        { 
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
void CDockerMan::UpdateIPfromServicelist(std::map<std::string,Service>& map){
    serviceIp.Clear();
    for(auto it = map.begin();it != map.end();++it){
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
    LogPrint("docker","CDockerMan::UpdateServicesList Succcessful\n");

    return true;
}
bool CDockerMan::UpdateService(std::string serviceid){
    LOCK(cs);
    dockerservicefilter serfilter;
    LogPrint("docker","CDockerMan::UpdateService start\n");
    if(!PushMessage(Method::METHOD_SERVICES_INSPECT,serviceid,serfilter.ToJsonString(),false)){
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
        size += it->second.mapDockerTaskLists.size();
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
            swarm.ip_port = it->second.managerStatus.addr;
            return;
        }
    }
}
std::string CDockerMan::GetMasterIp(){
    return swarm.ip_port.substr(0,swarm.ip_port.find_first_of(":"));

}
std::map<std::string ,Service> CDockerMan::GetServiceFromPubkey(CPubKey pubkey){
    map<std::string ,Service> mapServerlist{};
    for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
        if(it->second.customer == pubkey.ToString().substr(0,66)){
            mapServerlist.insert(*it);
        }
    }
    return mapServerlist;
}
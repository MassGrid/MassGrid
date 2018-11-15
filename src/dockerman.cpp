// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "dockerman.h"
#include "http.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

CDockerMan dockerman;
bool CDockerMan::PushMessage(Method mtd,std::string id,std::string pushdata){
    LogPrint("docker","CDockerMan::PushMessage Started Method: %s\n",strMethod[mtd]);
    std::string url;
    HttpType type;
    if(!pushdata.empty()){
        pushdata = "filters=" + pushdata;
    }
    switch (mtd)
    {
        case Method::METHOD_NODES_LISTS:
            url="/nodes";
            type=HttpType::HTTP_GET;
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
            break;
        case Method::METHOD_TASKS_INSPECT:
            url="/tasks/";
            url.append(id);
            type=HttpType::HTTP_GET;
            pushdata.clear();
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
    HttpRequest http(address,port,url,pushdata);
    LogPrint("docker","CDockerMan::RequestMessages methed %s\n send:%s\n",url,pushdata);
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
    if(ret != 0) {
            LogPrint("docker","CDockerMan::RequestMessages Http_Error error_code: %d\n",ret);
            return false;
    }
    std::string strRet=http.getReponseData();
    return ProcessMessage(mtd,http.url,strRet);
}

bool CDockerMan::ProcessMessage(Method mtd,std::string url,std::string responsedata){
    LogPrint("docker","CDockerMan::ProcessMessage Started Method: %s\n",strMethod[mtd]);
    std::string strMessage;
    std::string id;
    HttpType type;
    UniValue data(UniValue::VOBJ);
    data.read(responsedata);
    if(data.exists("message")){
        strMessage=data["message"].get_str();
        LogPrint("docker","CDockerMan::ProcessMessage ERROR ProcessMessage: %s\n",strMessage);
        return false;
    }
    LOCK(cs);
    switch (mtd)
    {
        case Method::METHOD_NODES_LISTS:
            mapDockerNodeLists.clear();
            Node::DockerNodeList(responsedata,mapDockerNodeLists);
            break;
        case Method::METHOD_NODES_INSPECT:
            Node::DockerNodeList(responsedata,mapDockerNodeLists);
            break;
        case Method::METHOD_NODES_DELETE:
            id=url.substr(url.find_last_of("/")+1);
            mapDockerNodeLists.erase(mapDockerNodeLists.find(id));
            break;
        case Method::METHOD_NODES_UPDATE:   // not implemented yet
            break;
        case Method::METHOD_SERVICES_LISTS:
            mapDockerServiceLists.clear();
            Service::DockerServiceList(responsedata,mapDockerServiceLists);
            break;
        case Method::METHOD_SERVICES_CREATE:
            if(data.exists("ID")){
                id=data["ID"].get_str();
                std::string strret;
                PushMessage(Method::METHOD_SERVICES_INSPECT,id,"");
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage Not exist this ID\n");
                return false;
            }
            break;
        case Method::METHOD_SERVICES_INSPECT:
            Service::DockerServiceList(responsedata,mapDockerServiceLists);
            break;
        case Method::METHOD_SERVICES_DELETE:
            id=url.substr(url.find_last_of("/")+1);
            mapDockerServiceLists.erase(mapDockerServiceLists.find(id));
            break;
        case Method::METHOD_SERVICES_UPDATE:
            if(data.exists("ID")){
                id=data["ID"].get_str();
                std::string strret;
                PushMessage(Method::METHOD_SERVICES_INSPECT,id,"");
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage Not exist this ID\n");
                return false;
            }
            break;
        case Method::METHOD_SERVICES_LOGS:  // not implemented yet
            url="/services/";
            url.append(id);
            url.append("logs");
            type=HttpType::HTTP_GET;
            break;
        case Method::METHOD_TASKS_LISTS:
            mapDockerTaskLists.clear();
            Task::DockerTaskList(responsedata,mapDockerTaskLists);
            break;
        case Method::METHOD_TASKS_INSPECT:
            Task::DockerTaskList(responsedata,mapDockerTaskLists);
            break;
        case Method::METHOD_INFO:   // not implemented yet
             url="/info";
            type=HttpType::HTTP_GET;
            break;
        case Method::METHOD_VERSION:    // not implemented yet
            url="/version";
            type=HttpType::HTTP_GET;
            break;      
        default:
            LogPrint("docker","CDockerMan::PushMessage not support this methed");
            return false;
    }
    return true; 
}
bool CDockerMan::Update(){
    LogPrint("docker","CDockerMan::Update start\n");
    mapDockerNodeLists.clear();
    mapDockerServiceLists.clear();
    mapDockerTaskLists.clear();
    LogPrint("docker","CDockerMan::Update ndfilter1 \n");
    dockernodefilter ndfilter;

    LogPrint("docker","CDockerMan::Update ndfilter2 : %s \n",ndfilter.ToJsonString());
    if(!PushMessage(Method::METHOD_NODES_LISTS,"",ndfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_NODES_LISTS failed! \n");
        return false;
    }
    dockerservicefilter serfilter;
    if(!PushMessage(Method::METHOD_SERVICES_LISTS,"",serfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_SERVICES_LISTS failed! \n");
        return false;
    }
    dockertaskfilter taskfilter;
    taskfilter.DesiredState_running=true;
    if(!PushMessage(Method::METHOD_TASKS_LISTS,"",taskfilter.ToJsonString())){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_TASKS_LISTS failed! \n");
        return false;
    }
    GetVersion();
    LogPrint("docker","CDockerMan::Update Succcessful\n");
    return true;
}
uint64_t CDockerMan::GetDockerNodeCount(){
    return mapDockerNodeLists.size();
}
uint64_t CDockerMan::GetDockerNodeActiveCount(){
    uint64_t count = 0;
    for(map<std::string,Node>::iterator it=mapDockerNodeLists.begin();it!=mapDockerNodeLists.end();++it){
        if(it->second.status.state == "ready")
            ++count;
    }
    return count;
}
uint64_t CDockerMan::GetDockerServiceCount(){
    return mapDockerServiceLists.size();
}
uint64_t CDockerMan::GetDoCkerTaskCount(){
    return mapDockerTaskLists.size();
}
void CDockerMan::GetVersion(){
    map<std::string,Node>::iterator it = mapDockerNodeLists.begin();
    for(;it!=mapDockerNodeLists.end();++it){
        if(it->second.spec.role == "manager"){   
            version = it->second.description.engine.engineVersion;
            return;
        }
    }
}
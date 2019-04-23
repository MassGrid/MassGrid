// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <set>
#include <algorithm>
#include <queue>
#include <boost/random.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "base58.h"
#include "dockerman.h"
#include "http.h"
#include "init.h"
#include "utiltime.h"
#include "timermodule.h"
#include "wallet/wallet.h"
#include "dockerserverman.h"
CDockerMan dockerman;
map<Item,Value_price> CDockerMan::GetPriceListFromNodelist(){
    return priceList;
}
void CDockerMan::UpdatePriceListFromNodelist(){
    std::map<std::string, Node> nodelist; 
    {
        LOCK(cs);
        nodelist = mapDockerNodeLists;
    }
    for(auto it=nodelist.begin();it!=nodelist.end();++it){
        if(it->second.isuseable == false || it->second.spec.role == Config::Role::ROLE_MANAGER)
            continue;
        if(it->second.status.state != Config::NodeStatusState::NODESTATUSSTATE_READY)
            continue;
        Item item(it->second.engineInfo.cpu.Name,it->second.engineInfo.cpu.Count,it->second.engineInfo.mem.Name,it->second.engineInfo.mem.Count,it->second.engineInfo.gpu.Name,it->second.engineInfo.gpu.Count);
        if(priceList.count(item)){
            priceList[item].count++;
        }else
        {
            CAmount price = dockerPriceConfig.getPrice(item.cpu.Type,item.cpu.Name) * item.cpu.Count +
                dockerPriceConfig.getPrice(item.mem.Type,item.mem.Name) * item.mem.Count +
                dockerPriceConfig.getPrice(item.gpu.Type,item.gpu.Name) * item.gpu.Count;
            priceList[item] = Value_price(price,1);
        }
        LogPrint("docker","item :%s price %d sum %d\n",item.ToString(),priceList[item].price,priceList[item].count);
    }
}
bool CDockerMan::GetItemFromNodeID(std::string nodeid,Item &item){
    std::map<std::string, Node> nodelist; 
    {
        LOCK(cs);
        nodelist = mapDockerNodeLists;
    }
    for(auto it=nodelist.begin();it!=nodelist.end();++it){
        if(it->second.isuseable == false || it->second.spec.role == Config::Role::ROLE_MANAGER)
            continue;
        if(it->second.status.state != Config::NodeStatusState::NODESTATUSSTATE_READY)
            continue;
        if(nodeid == it->first){
            item = it->second.engineInfo;
            return true;
        }
    }
    return false;
}
std::string CDockerMan::GetNodeFromItem(Item item){
    std::map<std::string, Node> nodelist; 
    {
        LOCK(cs);
        nodelist = mapDockerNodeLists;
    }
    std::string nodeid="";
    for(auto it=nodelist.begin();it!=nodelist.end();++it){
        if(it->second.isuseable == false || it->second.spec.role == Config::Role::ROLE_MANAGER)
            continue;
        if(it->second.status.state != Config::NodeStatusState::NODESTATUSSTATE_READY)
            continue;
        if(item.cpu.Name == it->second.engineInfo.cpu.Name && item.cpu.Count == it->second.engineInfo.cpu.Count 
            && item.mem.Name == it->second.engineInfo.mem.Name && item.mem.Count == it->second.engineInfo.mem.Count
            && item.gpu.Name == it->second.engineInfo.gpu.Name && item.gpu.Count == it->second.engineInfo.gpu.Count){
            LogPrint("docker","CDockerMan::GetNodeFromItem find a item, nodeid %s\n",it->first);
            return it->first;
        }
    }
    return nodeid;
}
std::set<std::string> CDockerMan::GetFreeNodeFromService(){
    std::set<std::string> freenodelist;

    std::map<std::string, Service> serviceList;
    {
        LOCK(cs);
        serviceList = mapDockerServiceLists;
        for(auto &nodelist: mapDockerNodeLists){
            if(nodelist.second.isuseable == false || nodelist.second.spec.role == Config::Role::ROLE_MANAGER)
                continue;
            if(nodelist.second.status.state != Config::NodeStatusState::NODESTATUSSTATE_READY)
                continue;
            freenodelist.insert(nodelist.first);
        }
    }
    LogPrintf("CDockerMan::GetFreeNodeFromService 0\n");
    for(auto &servicelist: serviceList){
        if(servicelist.second.mapDockerTaskLists.size()>0){
            std::string nodeid = servicelist.second.mapDockerTaskLists.begin()->second.nodeID;
            auto it= freenodelist.find(nodeid);
            if(it!=freenodelist.end()){
                freenodelist.erase(it);
            }
        }
    }
    LogPrintf("CDockerMan::GetFreeNodeFromService nodelist:%lld, servicelist: %lld freenodelist: %lld\n",mapDockerNodeLists.size(),mapDockerServiceLists.size(),freenodelist.size());
    return freenodelist;
}
std::string IpSet::GetFreeIP(){
    boost::mt19937 gen(time(0));
    boost::uniform_int<> uni_dist(ip_start, ip_end);
    boost::variate_generator<boost::mt19937&,boost::uniform_int<>>die(gen,uni_dist);
    while(true){
        in_addr_t addr = ntohl(die());
        in_addr ipaddr{};
        ipaddr.s_addr = addr;
        if(!ip_set.count(ipaddr.s_addr)){
            ip_set.insert(ipaddr.s_addr);

            char strip[INET_ADDRSTRLEN+1];
#ifdef WIN32
            sockaddr_in in;
            memcpy(&in.sin_addr,&ipaddr.s_addr,INET_ADDRSTRLEN);
            *strip = inet_ntoa(in.sin_addr);
#else
            if(inet_ntop(AF_INET,&ipaddr.s_addr, strip, INET_ADDRSTRLEN) == NULL){
                strip == NULL;
            }
#endif
            if(strip != NULL){
                LogPrintf("IpSet::GetFreeIP addr %u %s\n",addr,strip);
                return std::string(strip);
            }

        }
    }
}


std::string IpSet::GetNetMask(){
    in_addr mask{};
    mask.s_addr = ntohl(netmask);
    char strnetmask[INET_ADDRSTRLEN+1];

#ifdef WIN32
    sockaddr_in in;
    memcpy(&in.sin_addr,&mask.s_addr,INET_ADDRSTRLEN);
    return std::string(inet_ntoa(in.sin_addr));
#else
    return std::string(inet_ntop(AF_INET,&mask.s_addr, strnetmask, INET_ADDRSTRLEN));
#endif
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
    else{
        LogPrintf("Warning ip_set Remove %s not find\n",str);
    }
}
bool IpSet::IsVaild(std::string s){
    if(inet_addr(s.c_str()) == INADDR_NONE)
        return false;
    else
        return true; 
}

bool CDockerMan::PushMessage(Method mtd,std::string id,std::string requestData,bool isClearService){
    LogPrint("docker","CDockerMan::PushMessage Started Method: %s\n",strMethod[mtd]);
    std::string url;
    HttpType type;
    switch (mtd)
    {
        case Method::METHOD_NODES_LISTS:
            url="/nodes";
            type=HttpType::HTTP_GET;
            if(!requestData.empty()){
                requestData = "filters=" + requestData;
            }
            break;
        case Method::METHOD_NODES_INSPECT:
            url="/nodes/";
            url.append(id);
            type=HttpType::HTTP_GET;
            requestData.clear();
            break;
        case Method::METHOD_NODES_DELETE:
            url="/nodes/";
            url.append(id);
            type=HttpType::HTTP_DELETE;
            break;
        case Method::METHOD_NODES_UPDATE:   // not implemented yet
            break;
        case Method::METHOD_SERVICES_LISTS:
            url="/services";
            type=HttpType::HTTP_GET;
            if(!requestData.empty()){
                requestData = "filters=" + requestData;
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
            requestData.clear();
            break;
        case Method::METHOD_SERVICES_DELETE:
            url="/services/";
            url.append(id);
            type=HttpType::HTTP_DELETE;
            requestData.clear();
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
            requestData.clear();
        case Method::METHOD_TASKS_LISTS:
            url="/tasks";
            type=HttpType::HTTP_GET;
            if(!requestData.empty()){
                requestData = "filters=" + requestData;
            }
            break;
        case Method::METHOD_TASKS_INSPECT:
            url="/tasks/";
            url.append(id);
            type=HttpType::HTTP_GET;
            requestData.clear();
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
        case Method::MINER_SERVICES_CREATE:
            url="/services/create";
            type=HttpType::HTTP_POST;
            break;
        case Method::MINER_SERVICES_DELETE:
            url="/services/";
            url.append(id);
            type=HttpType::HTTP_DELETE;
            requestData.clear();
            break;  
        default:
            LogPrint("docker","CDockerMan::PushMessage not support this methed");
            return false;
    }
    HttpRequest http(address,apiPort,url,requestData,"/var/run/docker.sock");
    LogPrint("docker","CDockerMan::RequestMessages Methed %s Send:%s\n",url,requestData);
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

bool CDockerMan::ProcessMessage(Method mtd,std::string url,int ret,std::string responseData,bool isClearService){

    LogPrint("docker","CDockerMan::ProcessMessage Method: %s  ProcessMessage: %d\n",strMethod[mtd],ret);
    LogPrint("docker","CDockerMan::ProcessMessage Response Messages %s\n",responseData);
    std::string strMessage;
    std::string id;
    HttpType type;
    if(ret != 200 && ret !=201){
        return false;
    }
    UniValue jsondata(UniValue::VOBJ);
    jsondata.read(responseData);
    LOCK(cs);
    switch (mtd)
    {
        case Method::METHOD_NODES_LISTS:
        {
            if(isClearService)
                mapDockerNodeLists.clear();
            Node::NodeListUpdateAll(responseData,mapDockerNodeLists);
            GetVersionAndJoinToken();
            break;
        }
        case Method::METHOD_NODES_INSPECT:  // not use
        {
            Node::NodeListUpdate(responseData,mapDockerNodeLists);
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
            Service::ServiceListUpdateAll(responseData,mapDockerServiceLists);

            if(isClearService){
                //update service ipset
                UpdateIPfromServicelist(mapDockerServiceLists);
                //Update service Timer
                timerModule.UpdateQueAll(mapDockerServiceLists);
                timerModule.UpdateSetAll();
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
                LogPrintf("CDockerMan::ProcessMessage Service Create: %s Successful\n",id);

                PushMessage(Method::METHOD_SERVICES_INSPECT,id,"");

                if (mapDockerServiceLists.find(id) != mapDockerServiceLists.end()){
                    const Service& svi = mapDockerServiceLists[id];
                    if(pwalletMain->mapWallet.count(svi.txid)){
                        CWalletTx& wtx = pwalletMain->mapWallet[svi.txid];  //watch only not check
                        wtx.Setserviceid(svi.ID);
                        wtx.Setverison(std::to_string(WALLET_DATABASE_VERSION));
                        wtx.Setcreatetime(std::to_string(svi.createdAt));
                        wtx.Setprice(std::to_string(svi.price));
                        wtx.Setfeerate(std::to_string(svi.feeRate));
                        wtx.Setcpuname(svi.item.cpu.Name);
                        wtx.Setcpucount(std::to_string(svi.item.cpu.Count));
                        wtx.Setmemname(svi.item.mem.Name);
                        wtx.Setmemcount(std::to_string(svi.item.mem.Count));
                        wtx.Setgpuname(svi.item.gpu.Name);
                        wtx.Setgpucount(std::to_string(svi.item.gpu.Count));
                        wtx.Setmasternodeaddress(CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString());
                        wtx.Setcusteraddress(svi.customer);
                        CWalletDB walletdb(pwalletMain->strWalletFile);
                        wtx.WriteToDisk(&walletdb);
                    }
                }
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage Create Service error no serviceid\n");
                return false;
            }
            break;
        }
        case Method::METHOD_SERVICES_INSPECT:
        {
            if(jsondata.exists("ID")){
                Service::ServiceListUpdate(responseData,mapDockerServiceLists);
                dockertaskfilter taskfilter;
                taskfilter.serviceid.push_back(jsondata["ID"].get_str());
                // taskfilter.DesiredState_running=true;
                PushMessage(Method::METHOD_TASKS_LISTS,"",taskfilter.ToJsonString());

                timerModule.UpdateQue(mapDockerServiceLists,jsondata["ID"].get_str());

                return true;
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage No serviceid\n");
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
                        dockerman.serviceIpList.Erase(str);
                        break;
                    }
                }
                uint256 txid = it->second.txid;
                int taskstate = 0;
                std::string taskstatuscode="";
                // set node usage
                if(it->second.mapDockerTaskLists.size()){
                    string nodeid = it->second.mapDockerTaskLists.begin()->second.nodeID;
                    taskstate = it->second.mapDockerTaskLists.begin()->second.status.state;
                    taskstatuscode=it->second.mapDockerTaskLists.begin()->second.status.err;
                    if(taskstatuscode.size()>50) taskstatuscode=taskstatuscode.substr(0,50);
                    if(!nodeid.empty()){
                        auto it2 = mapDockerNodeLists.find(nodeid);
                        it2->second.isuseable=true;
                    }
                }
                mapDockerServiceLists.erase(it);
                //write deletetime todb
                if (!pwalletMain->mapWallet.count(txid)){
                    LogPrintf("CDockerMan::ProcessMessage Warning txid not found %s\n",txid.ToString());
                    return false;
                }
                CWalletTx& wtx = pwalletMain->mapWallet[txid];  //watch only not check
                wtx.Setdeletetime(std::to_string(GetAdjustedTime()));
                wtx.Settaskstate(std::to_string(taskstate));
                wtx.Settaskstatuscode(taskstatuscode);
                CWalletDB walletdb(pwalletMain->strWalletFile);
                wtx.WriteToDisk(&walletdb);

                //exec tlementset
                timerModule.UpdateSet(wtx);
                //  delete service form map
                LogPrint("docker","CDockerMan::ProcessMessage remove serviceid successful %s\n",id);
            }
            else{
                LogPrint("docker","CDockerMan::ProcessMessage serviceid not found %s\n",id);
                return false;
            }
            break;
        }
        case Method::METHOD_SERVICES_UPDATE:    // not implemented yet
        {
            id=url.substr(url.find_last_of("/")+1);
            bool ret = PushMessage(Method::METHOD_SERVICES_INSPECT,id,"");
            if(!ret){
                return false;
                LogPrint("docker","CDockerMan::ProcessMessage update failed serviceid %s\n",id);
            }
            LogPrint("docker","CDockerMan::ProcessMessage update successful serviceid %s\n",id);
            break;
        }
        case Method::METHOD_SERVICES_LOGS:  // not implemented yet
        {
            break;
        }
        case Method::METHOD_TASKS_LISTS:
        {
            std::set<std::string> serviceidSet;
            Service::UpdateTaskList(responseData,mapDockerServiceLists,serviceidSet); //updatealldate serviceid is null
            
            for(auto iter = serviceidSet.begin();iter != serviceidSet.end();++iter){
                auto it = mapDockerServiceLists.find(*iter);
                bool isMinerService=false;
                auto labels = it->second.spec.labels;
                if(labels.find("com.massgrid.miner")!= labels.end()){ // miner service
                    isMinerService=true;
                }
                std::string tmpnodeid="";
                if(it->second.mapDockerTaskLists.size()){
                    string nodeid = it->second.mapDockerTaskLists.begin()->second.nodeID;

                    if(!nodeid.empty()
                    && it->second.mapDockerTaskLists.begin()->second.status.state > Config::TaskState::TASKSTATE_PENDING
                    && it->second.mapDockerTaskLists.begin()->second.status.state < Config::TaskState::TASKSTATE_SHUTDOWN){
                        //miner
                        if(isMinerService){ // miner service
                            mapDockerNodeLists[nodeid].isuseable = true;
                        }
                        else
                        { // other service
                            mapDockerNodeLists[nodeid].isuseable = false;
                            if (pwalletMain->mapWallet.count(it->second.txid)){
                                CWalletTx& wtx = pwalletMain->mapWallet[it->second.txid];  //watch only not check
                                if(wtx.Getprovideraddress().empty()){
                                    wtx.Setprovideraddress(mapDockerNodeLists[nodeid].MGDAddress);
                                    CWalletDB walletdb(pwalletMain->strWalletFile);
                                    wtx.WriteToDisk(&walletdb);
                                }
                            }
                        }
                    }else
                    {
                        if(it->second.mapDockerTaskLists.begin()->second.status.state == Config::TaskState::TASKSTATE_FAILED){
                            tmpnodeid=nodeid;
                        }else{
                            mapDockerNodeLists[nodeid].isuseable=true;
                            LogPrint("docker","CDockerMan::ProcessMessage task state error reset usable serviceid: %s\n",it->first);
                        }
                    }
                }
                if(it->second.deleteTime <= GetAdjustedTime()){//when the task is always in pedding
                    LogPrintf("CDockerMan::ProcessMessage will be delete usable serviceid: %s\n",it->first);
                    if(isMinerService){// miner service 
                        PushMessage(Method::MINER_SERVICES_DELETE,it->first,"");
                    }else{//other service
                        PushMessage(Method::METHOD_SERVICES_DELETE,it->first,"");
                    }
                    
                    if(tmpnodeid.size()>0){
                        std::string pushData="force=ture";
                        bool ret = PushMessage(Method::METHOD_NODES_DELETE,tmpnodeid,pushData);
                        LogPrint("timer","CDockerMan::ProcessMessage delete nodeid %s %d\n",tmpnodeid,ret);
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
            Swarm::DockerSwarm(responseData,swarm);
            break;
        }
        case Method::METHOD_INFO:   // not implemented yet
        {
            break;
        }
        case Method::METHOD_VERSION:    // not implemented yet
        {
            break;   
        }
        case Method::MINER_SERVICES_CREATE:
        {
            if(jsondata.exists("ID")){
                id=jsondata["ID"].get_str();
                LogPrintf("CDockerMan::ProcessMessage miner service create %s Successful\n",id);
            }else{
                LogPrint("docker","CDockerMan::ProcessMessage miner service create error no serviceid\n");
                return false;
            }
            break;
        }
        case Method::MINER_SERVICES_DELETE:
        {
            id=url.substr(url.find_last_of("/")+1);
            auto it = mapDockerServiceLists.begin();
            if((it = mapDockerServiceLists.find(id)) != mapDockerServiceLists.end()){
                mapDockerServiceLists.erase(it);
                LogPrintf("CDockerMan::ProcessMessage Service delete miner %s\n",id);
            }else{
                LogPrintf("CDockerMan::ProcessMessage Service delete miner not in mapDockerServiceLists %s\n",id);
            }
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
    serviceIpList.Clear();
    for(auto it = map.begin();it != map.end();++it){
        auto env =it->second.spec.taskTemplate.containerSpec.env;
        for(auto itenv = env.begin();itenv != env.end();++itenv){
            if(itenv->find("N2N_SERVERIP=")!=-1){
                string str=itenv->substr(13);
                serviceIpList.Insert(str);
            }
        }
    }
}
bool CDockerMan::Update(bool isClear){
    LOCK(cs);
    dockernodefilter ndfilter;

    if(!PushMessage(Method::METHOD_NODES_LISTS,"",ndfilter.ToJsonString(),isClear)){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_NODES_LISTS failed! \n");
        return false;
    }
    dockerservicefilter serfilter;
    if(!PushMessage(Method::METHOD_SERVICES_LISTS,"",serfilter.ToJsonString(),isClear)){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_SERVICES_LISTS failed! \n");
        return false;
    }
    if(!PushMessage(Method::METHOD_SWARM_INSPECT,"","",isClear)){
        LogPrint("docker","CDockerMan::Update ERROR Get METHOD_SWARM_INSPECT failed! \n");
        return false;
    }
    UpdatePriceListFromNodelist();
    LogPrintf("CDockerMan::Update Succcessful, isClear %d\n",isClear);
    return true;
}
bool CDockerMan::UpdateSwarmAndNodeList(){
    LOCK(cs);
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
    if(!PushMessage(Method::METHOD_SERVICES_INSPECT,serviceid,serfilter.ToJsonString(),false)){
        LogPrint("docker","CDockerMan::UpdateService ERROR Get METHOD_SERVICES_INSPECT failed! \n");
        return false;
    }
    LogPrint("docker","CDockerMan::UpdateService Succcessful\n");
    return true;
}
uint64_t CDockerMan::GetDockerNodeCount(){
    uint64_t actnode = GetDockerNodeActiveCount()-GetDockerServiceCount()-1;
    if(actnode < 0) return 0;
    return actnode;
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
    std::map<std::string, Service> serviceList;
    {
        LOCK(cs);
        serviceList = mapDockerServiceLists;
    }
    int64_t serviceSize=0;
    for(auto &servicelist: serviceList){
        auto labels = servicelist.second.spec.labels;
        if(labels.find("com.massgrid.miner") == labels.end()){
            serviceSize++;
        }
    }
    return serviceSize;
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
        if(it->second.customer == CMassGridAddress(pubkey.GetID()).ToString()){
            mapServerlist.insert(*it);
        }
    }
    return mapServerlist;
}
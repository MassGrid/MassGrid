#include "timermodule.h"
#include "masternode-sync.h"
#include "dockerman.h"
#include "dockerserverman.h"
ServiceTimerModule timerModule;
bool ServiceTimerModule::Flush(){
    LogPrint("timer","ServiceTimerModule::Flush start\n");
    return dockerman.Update();
}
void ServiceTimerModule::UpdateQueAll(std::map<std::string, Service>&map){
    LOCK(cs_serInfoQueue);
    while(!serviceInfoQue.empty()) serviceInfoQue.pop();
    for(auto &service: map){
        ServiceListInfo serverinfo(service.second);//7200
        serviceInfoQue.push(serverinfo);
    }
}
void ServiceTimerModule::UpdateQue(std::map<std::string, Service>&map,std::string id){
    LOCK(cs_serInfoQueue);
    for(auto &service: map){
        if(service.second.ID == id){
            ServiceListInfo serverinfo(service.second);//7200
            serviceInfoQue.push(serverinfo);
            break;
        }
    }
}
void ServiceTimerModule::CheckQue(){
    LOCK(cs_serInfoQueue);
    auto now_time = GetAdjustedTime();
    while(!serviceInfoQue.empty()){
        ServiceListInfo slist=serviceInfoQue.top();
        if(now_time >= slist.deleteTime){
            //settlement 
            SetTlement(slist.serviceid);
            serviceInfoQue.pop();
            LogPrint("timer","ServiceTimerModule::CheckQue settlement serverice id= %s  timpstamp= %lu\n",slist.serviceid,now_time);
        }else{
            LogPrint("timer","ServiceTimerModule::CheckQue serviceInfoQue size= %d, the earliest serverice time= %lu id= %s\n",serviceInfoQue.size(),slist.deleteTime,slist.serviceid);
            break;
        }
    }
}
void ServiceTimerModule::SetTlement(std::string serviceid){
    Service svi;
    if(!dockerman.GetServiceFromList(serviceid,svi)){
        LogPrint("timer","ServiceTimerModule::SetTlement serviceid not found\n");
        return;
    }
    dockerServerman.SetTlementService(svi.txid);
    dockerman.PushMessage(Method::METHOD_SERVICES_DELETE,serviceid,"");
}
void ThreadTimeModule()
{
    RenameThread("massgrid-sctrl");
    LogPrintf("ThreadTimeModule Start\n");

    while(true)
    {
        if (masternodeSync.IsSynced()){
            timerModule.Flush();
            
            timerModule.CheckQue();
        }
        // Check for stop or if block needs to be rebuilt
        for(int i=0;i<100;i++){
            boost::this_thread::interruption_point();
            MilliSleep(100);
        }
    }
}
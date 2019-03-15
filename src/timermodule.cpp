#include "timermodule.h"
#include "init.h"
#include "validation.h"
#include "masternode-sync.h"
#include "dockerman.h"
#include "dockerserverman.h"
ServiceTimerModule timerModule;
bool ServiceTimerModule::Flush(){
    LogPrint("timer","ServiceTimerModule::Flush start\n");
    return dockerman.Update();
}

void ServiceTimerModule::UpdateSetAll(){
    LogPrint("timer","ServiceTimerModule::UpdateSet UpdateSetAll start\n");
    LOCK(cs_serInfoQueue2);
    // setWalletTx.clear();
    pwalletMain->Untlement(setWalletTx);
    CheckTransaction();
}
void ServiceTimerModule::UpdateSet(CWalletTx & wtx){
    LogPrint("timer","ServiceTimerModule::UpdateSet CWalletTx start\n");
    LOCK(cs_serInfoQueue2);
    if(dockerServerman.SetTlementServiceWithoutDelete(wtx.GetHash()) < 0)
        setWalletTx.insert(&wtx);
}
void ServiceTimerModule::UpdateQueAll(std::map<std::string, Service>&map){
    LogPrint("timer","ServiceTimerModule::UpdateQueAll start\n");
    LOCK(cs_serInfoQueue);
    while(!serviceInfoQue.empty()) serviceInfoQue.pop();
    for(auto &service: map){
        ServiceListInfo serverinfo(service.second);//7200
        serviceInfoQue.push(serverinfo);
    }
}
void ServiceTimerModule::UpdateQue(std::map<std::string, Service>&map,std::string id){
    LogPrint("timer","ServiceTimerModule::UpdateQue start\n");
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
    LogPrint("timer","ServiceTimerModule::CheckQue start\n");
    LOCK(cs_serInfoQueue);
    auto now_time = GetAdjustedTime();
    while(!serviceInfoQue.empty()){
        ServiceListInfo slist=serviceInfoQue.top();
        if(now_time >= slist.deleteTime){
            //settlement       
            dockerman.PushMessage(Method::METHOD_SERVICES_DELETE,slist.serviceid,"");
            serviceInfoQue.pop();
            LogPrint("timer","ServiceTimerModule::CheckQue settlement serverice id= %s  timpstamp= %lu\n",slist.serviceid,now_time);
        }else{
            LogPrint("timer","ServiceTimerModule::CheckQue serviceInfoQue size= %d, the earliest serverice time= %lu id= %s\n",serviceInfoQue.size(),slist.deleteTime,slist.serviceid);
            break;
        }
    }
}
void ServiceTimerModule::SetTlement(){
    LogPrint("timer","ServiceTimerModule::SetTlement start\n");
    LOCK(cs_serInfoQueue2);
    for(auto wtx = setWalletTx.begin(); wtx != setWalletTx.end();){
        if(dockerServerman.SetTlementServiceWithoutDelete((*wtx)->GetHash()) >= 0){
            setWalletTx.erase(wtx++);
        }else{
            ++wtx;
        }
        
    }
}
void ServiceTimerModule::CheckTransaction(){
    LogPrint("timer","ServiceTimerModule::CheckTransaction start\n");
    LOCK(cs_serInfoQueue2);
    for(auto wtx = setWalletTx.begin(); wtx != setWalletTx.end();){
        if((*wtx)->Getdeletetime().empty()){
            if(!dockerman.IsExistSerivce((*wtx)->GetHash())){
                LogPrint("timer","ServiceTimerModule::CheckTransaction remove transacation %s\n",(*wtx)->GetHash().ToString());
                (*wtx)->Setdeletetime(std::to_string(GetAdjustedTime()));
                (*wtx)->Settaskstate(std::to_string(Config::TASKSTATE_SHUTDOWN));
                CWalletDB walletdb(pwalletMain->strWalletFile);
                (*wtx)->WriteToDisk(&walletdb);
            }else{
                setWalletTx.erase(wtx++);
                continue;
            }
        }
         ++wtx;
    }
}
void ThreadTimeModule()
{
    RenameThread("massgrid-sctrl");
    LogPrintf("ThreadTimeModule Start\n");
    int height = chainActive.Height();
    while(true)
    {
        if (masternodeSync.IsSynced()){
            timerModule.Flush();
            
            timerModule.CheckQue();

            if(height != chainActive.Height()){
                timerModule.SetTlement();
                height = chainActive.Height();
            }
        }
        // Check for stop or if block needs to be rebuilt
        for(int i=0;i<100;i++){
            boost::this_thread::interruption_point();
            MilliSleep(100);
        }
    }
}
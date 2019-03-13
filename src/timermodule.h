#ifndef TIMEMODULE_H
#define TIMEMODULE_H
#include <queue>
#include <set>
#include "dockernode.h"
#include "dockerswarm.h"
#include "dockerservice.h"
#include "dockertask.h"
#include "wallet/wallet.h"
class ServiceTimerModule;
class ServiceListInfo{
public:
    ServiceListInfo(Service& p){
        deleteTime = p.deleteTime;
        serviceid = p.ID;
    }
    ~ServiceListInfo(){};
public:
    int64_t deleteTime;
    std::string serviceid;
public:
    bool operator < (const ServiceListInfo &a) const { 
        return deleteTime > a.deleteTime;//max first 
    }
};
extern ServiceTimerModule timerModule;
class ServiceTimerModule{
private:
    mutable CCriticalSection cs_serInfoQueue;
    mutable CCriticalSection cs_serInfoQueue2;
    std::priority_queue<ServiceListInfo, std::vector<ServiceListInfo> > serviceInfoQue;
    std::set<CWalletTx*> setWalletTx;
public:
    bool Flush();
    void UpdateSetAll();
    void UpdateSet(CWalletTx & wtx);
    void UpdateQueAll(std::map<std::string, Service>&map);
    void UpdateQue(std::map<std::string, Service>&map,std::string id);
    void CheckQue();
    void CheckTransaction();
    void SetTlement();
    std::set<CWalletTx*>& GetWalletTxSet(){
        LOCK(cs_serInfoQueue2);
        return setWalletTx;
    }
};
void ThreadTimeModule();
#endif //TIMEMODULE_H
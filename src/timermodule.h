#ifndef TIMEMODULE_H
#define TIMEMODULE_H
#include "dockernode.h"
#include "dockerswarm.h"
#include "dockerservice.h"
#include "dockertask.h"
#include <queue>
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
public:
    mutable CCriticalSection cs_serInfoQueue;
    std::priority_queue<ServiceListInfo, std::vector<ServiceListInfo> > serviceInfoQue;
public:
    bool Flush();
    void UpdateQueAll(std::map<std::string, Service>&map);
    void UpdateQue(std::map<std::string, Service>&map,std::string id);
    void CheckQue();
    void SetTlement(std::string serviceid);
};
void ThreadTimeModule();
#endif //TIMEMODULE_H
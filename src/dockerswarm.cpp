#include "dockerswarm.h"
void DockerSwarm(const string& swarmData,std::vector<Swarm> &swarms)
{
    LogPrintf("docker json node\n");
    try{
        UniValue data(UniValue::VARR);
        if(!data.read(swarmData)){
            LogPrintf("docker json error\n");
            return;
        }
        Swarm swarm;
        bool fSuccess = Swarm::DockerSwarmJson(data,swarm);
        if(fSuccess)
            swarms.push_back(swarm);
    }catch(std::exception& e){
        LogPrintf("JSON read error,%s\n",string(e.what()).c_str());
    }catch(...){
        LogPrintf("unkonw exception\n");
    }
}
bool Swarm::DockerSwarmJson(const UniValue& data, Swarm& swarm)
{
    std::string id;
    int idx;
    uint64_t createdTime;
    uint64_t updateTime;
    std::string mjoinWorkerTokens;
    std::string mjoinManagerTokens;
    int version=DEFAULT_CTASK_API_VERSION;
    
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID") id=data[vKeys[i]].get_str();
            else if(vKeys[i]=="CreatedAt") createdTime=getDockerTime(data[vKeys[i]].get_str());
            else if(vKeys[i]=="UpdatedAt") updateTime=getDockerTime(data[vKeys[i]].get_str());
        }
        if(data[vKeys[i]].isObject()){
            UniValue tdata(data[vKeys[i]]);
            if(vKeys[i]=="Version") idx=find_value(tdata,"Index").get_int();
            else if(vKeys[i]=="JoinTokens"){
                mjoinWorkerTokens=find_value(tdata,"Worker").get_str();
                mjoinManagerTokens=find_value(tdata,"Manager").get_str();
            }
        }
    }
    swarm=Swarm(id,idx,createdTime,updateTime,mjoinWorkerTokens,mjoinManagerTokens,version);
    return true;
}
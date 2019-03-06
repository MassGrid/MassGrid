#include "dockerswarm.h"
void Swarm::DockerSwarm(const string& swarmData,Swarm &swarms)
{
    // LogPrint("docker","Swarm::DockerSwarm docker json node\n");
    try{
        UniValue data(UniValue::VARR);
        if(!data.read(swarmData)){
            LogPrint("docker","Swarm::DockerSwarm docker json error\n");
            return;
        }
        Swarm swarm;
        bool fSuccess = DockerSwarmJson(data,swarm);
        if(fSuccess)
            swarms=swarm;
    }catch(std::exception& e){
        LogPrint("docker","Swarm::DockerSwarm JSON read error,%s\n",string(e.what()).c_str());
    }catch(...){
        LogPrint("docker","Swarm::DockerSwarm unkonw exception\n");
    }
}
bool Swarm::DockerSwarmJson(const UniValue& data, Swarm& swarm)
{
    std::string id;
    Config::Version version;
    uint64_t createdTime;
    uint64_t updateTime;
    std::string mjoinWorkerTokens;
    std::string mjoinManagerTokens;
    int protocolVersion=DEFAULT_CTASK_API_VERSION;
    
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID") id=data[vKeys[i]].get_str();
            else if(vKeys[i]=="CreatedAt") createdTime=getDockerTime(data[vKeys[i]].get_str());
            else if(vKeys[i]=="UpdatedAt") updateTime=getDockerTime(data[vKeys[i]].get_str());
        }
        if(data[vKeys[i]].isObject()){
            UniValue tdata(data[vKeys[i]]);
            if(vKeys[i]=="Version"){
                version.index=find_value(tdata,"Index").get_int64();
            }else if(vKeys[i]=="JoinTokens"){
                mjoinWorkerTokens=find_value(tdata,"Worker").get_str();
                mjoinManagerTokens=find_value(tdata,"Manager").get_str();
            }
        }
    }
    swarm=Swarm(id,version,createdTime,updateTime,mjoinWorkerTokens,mjoinManagerTokens,protocolVersion);
    return true;
}
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
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID") swarm.ID=data[vKeys[i]].get_str();
            else if(vKeys[i]=="CreatedAt") swarm.createdAt=getDockerTime(data[vKeys[i]].get_str());
            else if(vKeys[i]=="UpdatedAt") swarm.updatedAt=getDockerTime(data[vKeys[i]].get_str());
        }
        if(data[vKeys[i]].isObject()){
            UniValue tdata(data[vKeys[i]]);
            if(vKeys[i]=="Version"){
                swarm.version.index=find_value(tdata,"Index").get_int64();
            }else if(vKeys[i]=="JoinTokens"){
                swarm.joinWorkerTokens =find_value(tdata,"Worker").get_str();
                swarm.joinManagerTokens=find_value(tdata,"Manager").get_str();
            }
        }
    }
    return true;
}
#include "dockernode.h"

void dockernode(const string& nodeData,std::vector<Node> &nodes)
{
    std::cout<<"docker json node"<<std::endl;
    try{
        UniValue dataArry(UniValue::VARR);
        if(!dataArry.read(nodeData)){
            std::cout<<"json error\n";
            return;
        }

        for(size_t i=0;i<dataArry.size();i++){
            UniValue data(dataArry[i]);
            Node *node=DockerNodeJson(data);
            nodes.push_back(*node);
            // break;
        }
    }catch(std::exception& e){
        std::cout<<string(e.what())<<std::endl;
    }catch(...){
        std::cout<<"unkonw exception"<<std::endl;
    }
}
Node *DockerNodeJson(const UniValue& data)
{
    std::string id;
    int idx;
    uint64_t createdTime;
    uint64_t updateTime;
    Config::NodeSpec spec;
    Config::NodeDescription description;
    Config::NodeStatus status;
    Config::NodeManagerStatus managerStatus;
    int version=DEFAULT_CNODE_API_VERSION;

    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ID") id=tdata.get_str();
            else if(vKeys[i]=="CreatedAt") createdTime=getDockerTime(tdata.get_str());
            else if(vKeys[i]=="UpdatedAt") updateTime=getDockerTime(tdata.get_str());
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Version"){
                idx=find_value(tdata,"Index").get_int();
            }else if(vKeys[i]=="Spec"){
                ParseNodeSpec(tdata,spec);
            }else if(vKeys[i]=="Description"){
                ParseNodeDescription(tdata,description);
            }else if(vKeys[i]=="Status"){
                ParseNodeStatus(tdata,status);
            }else if(vKeys[i]=="ManagerStatus"){
                ParseNodeManageStatus(tdata,managerStatus);
            }
        }
    }
    return new Node(id,idx,createdTime,updateTime,spec,description,status,managerStatus,version);
}
void ParseNodeSpec(const UniValue& data,Config::NodeSpec &spec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Role"){
                spec.role=tdata.get_str();
            }else if(vKeys[i]=="Availability"){
                spec.availability=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Labels"){
                // ParseNodeLabels(tdata);
            }
        }
    }
}
void ParseNodeDescription(const UniValue& data,Config::NodeDescription &decp)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Hostname"){
                decp.hostname=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Platform"){
                ParseNodePlatform(tdata,decp.platform);
            }else if(vKeys[i]=="Resources"){
                ParseNodeResource(tdata,decp.resources);
            }else if(vKeys[i]=="Engine"){
                ParseNodeEngine(tdata,decp.engine);
            }
        }
    }
}
void ParseNodePlatform(const UniValue& data,Config::Platform &platform)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Architecture") platform.architecture=tdata.get_str();
            else if(vKeys[i]=="OS") platform.OS=tdata.get_str();
        }
    }
}
void ParseNodeResource(const UniValue& data, Config::Limits &limits)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="NanoCPUs"){
                limits.nanoCPUs=tdata.get_int64();
            }else if(vKeys[i]=="MemoryBytes"){
                limits.memoryBytes=tdata.get_int64();
            }
        }
    }
}
void ParseNodeEngine(const UniValue& data, Config::Engine &engine)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="EngineVersion"){
                engine.engineVersion=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Plugins"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::SPlugins splugins;
                    ParseNodeSPlugins(tdata[j],splugins);
                    engine.plugin.push_back(splugins);
                }
            }
        }
    }
}
void ParseNodeSPlugins(const UniValue& data, Config::SPlugins &splugin)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Type"){
                splugin.type =tdata.get_str();
            }else if(vKeys[i]=="Name"){
                splugin.name =tdata.get_str();
            }
        }
    }
}
void ParseNodeStatus(const UniValue& data, Config::NodeStatus &status)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="State"){
                status.state =tdata.get_str();
            }else if(vKeys[i]=="Addr"){
                status.Addr =tdata.get_str();
            }
        }
    }
}
void ParseNodeManageStatus(const UniValue& data, Config::NodeManagerStatus &managerStatus)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isBool()){
            if(vKeys[i]=="Leader"){
                managerStatus.Leader =tdata.get_bool();
            }
        }
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Reachability"){
                managerStatus.Reachability =tdata.get_str();
            }else if(vKeys[i]=="Addr"){
                managerStatus.Addr =tdata.get_str();
            }
        }
    }
}
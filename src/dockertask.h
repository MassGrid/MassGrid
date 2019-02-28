#ifndef DOCKERTASK_H
#define DOCKERTASK_H
#include "dockerbase.h"
#include "dockercontainer.h"
namespace Config{
    enum TaskState{
        TASKSTATE_NEW=0,TASKSTATE_ALLOCATED,TASKSTATE_PENDING,
        TASKSTATE_ASSIGNED,TASKSTATE_ACCEPTED,TASKSTATE_PREPAREING,
        TASKSTATE_READY,TASKSTATE_STARTING,TASKSTATE_RUNNING,
        TASKSTATE_COMPLETE,TASKSTATE_SHUTDOWN,TASKSTATE_FAILED,
        TASKSTATE_REJECTED,TASKSTATE_REMOVE,TASKSTATE_ORPHANED
    };
    struct TaskStatus{
        uint64_t timeStamp{};
        int state{};  //new allocated pending assigned 
                            //accepted preparing ready starting 
                            //running complete shutdown failed
                            //rejected remove orphaned
        std::string message{};
        std::string err{};
        ContainerStatus containerStatus{};
        ADD_SERIALIZE_PROPERTIES(timeStamp,state,message,err,containerStatus);
    };
    struct TaskSpec{    
        // pluginSpec
        ContainerSpec containerSpec{};
        Resource resources{};
        RestartPolicy restartPolicy{};
        Placement placement{};
        int64_t forceUpdate{};
        std::string runtime{};
        vector<struct NetWork> netWorks{};
        LogDriver logdriver{};
        ADD_SERIALIZE_PROPERTIES(containerSpec,resources,restartPolicy,placement,forceUpdate,runtime,netWorks,logdriver);
    };
};
class Task :public DockerBase{
    static void ParseTaskLabels(const UniValue& data,Config::Labels &labels);
    static void ParseTaskTemplateSpec(const UniValue& data,Config::TaskSpec &taskTemplate);
    static void ParseResource(const UniValue& data,Config::Resource &resource);
    static void ParseResourceObj(const UniValue& data, Config::ResourceObj &resources);
    static void ParseGenericResources(const UniValue& data, Config::GenericResources &genResources);
    static void ParseResGenNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec);
    static void ParseResGenDiscSpec(const UniValue& data, Config::DiscreteResourceSpec &discResourceSpec);
    static void ParseRestartPolicy(const UniValue& data,Config::RestartPolicy &repoly);
    static void ParsePlacement(const UniValue& data,Config::Placement &placement);
    static void ParsePreferences(const UniValue& data,Config::Preferences &preference);
    static void ParsePreferencesSpread(const UniValue& data,Config::Spread &spread);
    static void ParsePlatforms(const UniValue& data,Config::Platform &platform);
    static void ParseNetwork(const UniValue& data,Config::NetWork &network);
    static void ParseLogDriver(const UniValue& data,Config::LogDriver &logdriver);
    static void ParseLogDriverOpt(const UniValue& data,Config::Labels &labels);
    static void ParseTaskStatus(const UniValue& data,Config::TaskStatus &taskstaus);
    static void ParseTaskContainerStatus(const UniValue& data, Config::ContainerStatus &contstatus);
    static void ParseGenResources(const UniValue& data, Config::GenericResources &genResources);
    static void ParseGenResNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec);
    static void ParseGenResDiscSpec(const UniValue& data, Config::DiscreteResourceSpec &discResourceSpec);
    static int GetTaskStatus(std::string strType);
public:

    static bool DecodeFromJson(const UniValue& data,Task& task);
    static void TaskListUpdateAll(const string& taskData, std::map<std::string,Task> &tasks);

    std::string name{};  
    Config::Labels labels{}; 
    
    Config::TaskSpec spec{};
    std::string serviceID{};
    // CService service;
    int64_t slot{};
    std::string nodeID{};
    // CNode node;
    std::vector<Config::GenericResources> genericResources{};
    Config::TaskStatus status{};

    int desiredState{};
    std::vector<Config::NetworkTemplate> networksAttachments{};

public:
    Task() = default;
    // Task(){};
    Task(std::string id,Config::Version version ,uint64_t createdTime ,uint64_t updateTime,
    Config::Labels lab,
    Config::TaskSpec spec,
    std::string serviceid,
    int64_t slot,
    std::string nodeid,
    Config::TaskStatus taskstatus,
    Config::eStatus desiredstate,
    std::vector<Config::NetworkTemplate> networksattachments,
    std::vector<Config::GenericResources> genericResources,
    int protocolVersion=DEFAULT_CTASK_API_VERSION):labels(lab),spec(spec),serviceID(serviceid),
    slot(slot),nodeID(nodeid),status(taskstatus),desiredState(desiredstate),
    networksAttachments(networksattachments),genericResources(genericResources),
    DockerBase(id,version,createdTime,updateTime,protocolVersion){}

    Task(const Task& from){
        ID=from.ID;
        version=from.version;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        serviceID=from.serviceID;
        slot=from.slot;
        nodeID=from.nodeID;
        status=from.status;
        desiredState=from.desiredState;
        networksAttachments=from.networksAttachments;
        genericResources=from.genericResources;
    }

    Task& operator=(Task const& from){
        ID=from.ID;
        version=from.version;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        serviceID=from.serviceID;
        slot=from.slot;
        nodeID=from.nodeID;
        status=from.status;
        desiredState=from.desiredState;
        networksAttachments=from.networksAttachments;
        genericResources=from.genericResources;
        return *this;
    }
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(version);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(nProtocolVersion);
        READWRITE(name);
        READWRITE(labels);
        READWRITE(spec);
        READWRITE(serviceID);
        READWRITE(slot);
        READWRITE(nodeID);
        READWRITE(genericResources);
        READWRITE(status);
        READWRITE(desiredState);
        READWRITE(networksAttachments);
    }


    void Update();
    std::string ToString();
};
class dockertaskfilter:public filterbase{
public:

    bool DesiredState_running = false;
    bool DesiredState_shutdown = false;
    bool DesiredState_accepted = false;
    vector<std::string> nodeid;
    vector<std::string> serviceid;
    std::string ToJsonString();
};
#endif //__DOCKERTASK__

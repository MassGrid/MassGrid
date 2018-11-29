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
    const char* strTaskState[]={"new", "allocated","pending","assigned", 
                            "accepted","preparing","ready","starting",
                            "running","complete","shutdown","failed",
                            "rejected","remove","orphaned"};
    struct TaskStatus{
        uint64_t timeStamp;
        int state;  //new allocated pending assigned 
                            //accepted preparing ready starting 
                            //running complete shutdown failed
                            //rejected remove orphaned
        std::string message;
        std::string err;
        ContainerStatus containerStatus;
    };
    struct TaskSpec{    
        // pluginSpec
        ContainerSpec containerSpec;
        Resource resources;
        RestartPolicy restartPolicy;
        Placement placement;
        int64_t forceUpdate;
        std::string runtime;
        vector<struct NetWork> netWorks;
        LogDriver logdriver;
    };
        struct TaskStatus: Status {
        ContainerStatus containerStatus;
        // PortStatus
    };
};
class Task :public DockerBase{
    static bool DcokerTaskJson(const UniValue& data,Task& task);
    static void ParseTaskStatus(const UniValue& data,Config::TaskStatus &taskstaus);
    static void ParseTaskContainerStatus(const UniValue& data, Config::ContainerStatus &contstatus);
    static void ParseTaskNetworkTemplate(const UniValue& data,Config::NetworkTemplate &networktemp);
    static void ParseTaskNetwork(const UniValue& data,Config::NetworkTemplate &networktemp);
    static void ParseTaskNetWorkSpec(const UniValue& data, Config::NetWorkSpec &Spec);
    static void ParseTaskLabels(const UniValue& data,Config::Labels &labels);
    static void ParseTaskDriverConf(const UniValue& data,vector<std::string> &array);
    static void ParseTaskIPAMOptions(const UniValue& data,Config::IPAMOption &ipamoption);
    static void ParseTaskIPMOPDriver(const UniValue& data,Config::Driver &driver);
    static void ParseTaskIPMOPConfigs(const UniValue& data,Config::ConfigIP &configip);
    static void ParseTaskDriverState(const UniValue& data,Config::DriverState &drivstat);
    static void ParseTaskGenericResources(const UniValue& data, Config::NamedResourceSpec &namerespec);
    static void ParseTaskGenericResourcesName(const UniValue& data, Config::NamedResourceSpec &namerespec);
    static void ParseTaskSpec(const UniValue& data, Config::TaskSpec &taskspec);
    static void ParseTaskSpecContainer(const UniValue& data,  Config::ContainerSpec &conttemp);
    static void ParseTaseSpecMount(const UniValue& data,Config::Mount &mount);
    static void ParseTaskSpecResources(const UniValue& data,Config::Resource &resources);
    static void ParseTaskSpecLimits(const UniValue& data,Config::Limits &limits);
    static void ParseTaskSpecReserv(const UniValue& data,Config::Reservation &reserv);
    static void ParseTaseSpecGenericRe(const UniValue& data, Config::DiscreteResourceSpec &disreserv);
    static void ParseTaseSpecDisGenericRe(const UniValue& data, Config::DiscreteResourceSpec &disreserv);
    static void ParseTaskSpecRestartPolicy(const UniValue& data,Config::RestartPolicy &repolicy);
    static void ParseTaskSpecPlacement(const UniValue& data,Config::Placement &placement);
    static void ParseTaseSpecPlatforms(const UniValue& data,Config::Platform &platform);
    static void ParseTaskSpecNetWorks(const UniValue& data,Config::NetWork &network);
    static void ParseTaskArray(const UniValue& data,vector<std::string> &array);
    static int getTaskStatus(const std::string& status);
public:

    static void DockerTaskList(const string& taskData, std::map<std::string,Task> &tasks);

    std::string name;
    Config::Labels labels;
    
    Config::TaskSpec spec;
    std::string serviceID;
    // CService service;
    int64_t slot;
    std::string nodeID;
    // CNode node;
    Config::AssignedGenericResources assignedGenericResources;
    Config::TaskStatus status;

    int desiredState;
    std::vector<Config::NetworkTemplate> networksAttachments;

public:
    Task() = default;
    // Task(){};
    Task(std::string id,Config::Version version ,uint64_t createdTime ,uint64_t updateTime,
    std::vector<std::pair<std::string, std::string> > lab,
    Config::TaskSpec spec,
    std::string serviceid,
    uint slot,
    std::string nodeid,
    Config::TaskStatus taskstatus,
    Config::eStatus desiredstate,
    std::vector<Config::NetworkTemplate> networksattachments,
    std::vector<Config::GenericResources> genericresources,
    int protocolVersion=DEFAULT_CTASK_API_VERSION):labels(lab),spec(spec),serviceID(serviceid),
    slot(slot),nodeID(nodeid),status(taskstatus),desiredState(desiredstate),
    networksAttachments(networksattachments),genericResources(genericresources),
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
#endif //DOCKERTASK_H

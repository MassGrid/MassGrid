#ifndef __DOCKERTASK__
#define __DOCKERTASK__
#include "dockerbase.h"
class Task :public DockerBase{
public:
    static bool DcokerTaskJson(const UniValue& data,Task& task);
    static void ParseTaskStatus(const UniValue& data,Config::TaskStatus &taskstaus);
    static void ParseTaskContainerStatus(const UniValue& data, Config::ContainerStatus &contstatus);
    static void ParseTaskNetworkTemplate(const UniValue& data,Config::NetworkTemplate &networktemp);
    static void ParseTaskNetwork(const UniValue& data,Config::NetworkTemplate &networktemp);
    static void ParseTaskNetWorkSpec(const UniValue& data, Config::NetWorkSpec &Spec);
    static void ParseTaskLabels(const UniValue& data,vector<std::string> &array);
    static void ParseTaskIPAMOptions(const UniValue& data,Config::IPAMOption &ipamoption);
    static void ParseTaskIPMOPDriver(const UniValue& data,Config::Driver &driver);
    static void ParseTaskIPMOPConfigs(const UniValue& data,Config::ConfigIP &configip);
    static void ParseTaskDriverState(const UniValue& data,Config::DriverState &drivstat);
    static void ParseTaskGenericResources(const UniValue& data, Config::NamedResourceSpec &namerespec);
    static void ParseTaskGenericResourcesName(const UniValue& data, Config::NamedResourceSpec &namerespec);
    static void ParseTaskSpec(const UniValue& data, Config::TaskSpec &taskspec);
    static void ParseTaskSpecContainer(const UniValue& data,  Config::ContainerTemplate &conttemp);
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
    std::vector<std::pair<std::string, std::string> > labels;
    
    Config::TaskSpec spec;
    std::string serviceID;
    // CService service;
    uint slot;
    std::string nodeID;
    // CNode node;
    Config::TaskStatus status;
    Config::eStatus desiredState;
    std::vector<Config::NetworkTemplate> networksAttachments;
    std::vector<Config::NamedResourceSpec> genericResources;

public:
    Task() = default;
    // Task(){};
    Task(std::string id,int idx ,uint64_t createdTime ,uint64_t updateTime,
    std::vector<std::pair<std::string, std::string> > lab,
    Config::TaskSpec spec,
    std::string serviceid,
    uint slot,
    std::string nodeid,
    Config::TaskStatus taskstatus,
    Config::eStatus desiredstate,
    std::vector<Config::NetworkTemplate> networksattachments,
    std::vector<Config::NamedResourceSpec> genericresources,
    int version=DEFAULT_CTASK_API_VERSION):labels(lab),spec(spec),serviceID(serviceid),
    slot(slot),nodeID(nodeid),status(taskstatus),desiredState(desiredstate),
    networksAttachments(networksattachments),genericResources(genericresources),
    DockerBase(id,idx,createdTime,updateTime,version){}

    Task(const Task& from){
        ID=from.ID;
        index=from.index;
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
        index=from.index;
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
        READWRITE(index);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(nProtocolVersion);
    }


    void Update();
    std::string ToString();
};
class dockertaskfilter:public filterbase{
public:

    bool DesiredState_running=true;
    bool DesiredState_shutdown=true;
    bool DesiredState_accepted=true;
    vector<std::string> nodeid;
    vector<std::string> serviceid;
    std::string ToJsonString();
    std::string ToString();
};
void DockerTask(const string& taskData, std::vector<Task> &tasks);
#endif //__DOCKERSERVICE__

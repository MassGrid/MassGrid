#ifndef __DOCKERTASK__
#define __DOCKERTASK__
#include "dockerbase.h"
class Task :public DockerBase{
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
void dockertask(const string& taskData, std::vector<Task> &tasks);
Task *DcokerTaskJson(const UniValue& data);
void ParseTaskStatus(const UniValue& data,Config::TaskStatus &taskstaus);
void ParseTaskContainerStatus(const UniValue& data, Config::ContainerStatus &contstatus);
void ParseTaskNetworkTemplate(const UniValue& data,Config::NetworkTemplate &networktemp);
void ParseTaskNetwork(const UniValue& data,Config::NetworkTemplate &networktemp);
void ParseTaskNetWorkSpec(const UniValue& data, Config::NetWorkSpec &Spec);
void ParseTaskLabels(const UniValue& data,vector<std::string> &array);
void ParseTaskIPAMOptions(const UniValue& data,Config::IPAMOption &ipamoption);
void ParseTaskIPMOPDriver(const UniValue& data,Config::Driver &driver);
void ParseTaskIPMOPConfigs(const UniValue& data,Config::ConfigIP &configip);
void ParseTaskDriverState(const UniValue& data,Config::DriverState &drivstat);
void ParseTaskGenericResources(const UniValue& data, Config::NamedResourceSpec &namerespec);
void ParseTaskGenericResourcesName(const UniValue& data, Config::NamedResourceSpec &namerespec);
void ParseTaskSpec(const UniValue& data, Config::TaskSpec &taskspec);
void ParseTaskSpecContainer(const UniValue& data,  Config::ContainerTemplate &conttemp);
void ParseTaseSpecMount(const UniValue& data,Config::Mount &mount);
void ParseTaskSpecResources(const UniValue& data,Config::Resource &resources);
void ParseTaskSpecLimits(const UniValue& data,Config::Limits &limits);
void ParseTaskSpecReserv(const UniValue& data,Config::Reservation &reserv);
void ParseTaseSpecGenericRe(const UniValue& data, Config::DiscreteResourceSpec &disreserv);
void ParseTaseSpecDisGenericRe(const UniValue& data, Config::DiscreteResourceSpec &disreserv);
void ParseTaskSpecRestartPolicy(const UniValue& data,Config::RestartPolicy &repolicy);
void ParseTaskSpecPlacement(const UniValue& data,Config::Placement &placement);
void ParseTaseSpecPlatforms(const UniValue& data,Config::Platform &platform);
void ParseTaskSpecNetWorks(const UniValue& data,Config::NetWork &network);
void ParseTaskArray(const UniValue& data,vector<std::string> &array);
int getTaskStatus(const std::string& status);
#endif //__DOCKERSERVICE__

#ifndef DOCKERSERVICE_H
#define DOCKERSERVICE_H
#include "dockerbase.h"

class Service:public DockerBase{
    static bool DcokerServiceJson(const UniValue& data, Service& service);
    static void ParseSpec(const UniValue& data,Config::ServiceSpec &spc);
    static void ParseSpecLabels(const UniValue& data,std::map<std::string,std::string> &labels);
    static void ParseTaskTemplate(const UniValue& data,Config::TaskSpec &taskTemplate);
    static void ParseContainerTemplate(const UniValue& data,Config::ContainerTemplate &contTemp);
    static void ParseSpecContainerLabels(const UniValue& data,std::map<std::string,std::string> &labels);
    static void ParseMount(const UniValue& data,Config::Mount &mount);
    static void ParseArray(const UniValue& data,vector<std::string> &array);
    static void ParseContainerTemplate(const UniValue& data,Config::Resource &resources);
    static void ParseLimits(const UniValue& data,Config::Limits &limits);
    static void ParseResource(const UniValue& data,Config::Resource &resources);
    static void ParseReservation(const UniValue& data,Config::Reservation &reservations);
    static void ParseDirReservation(const UniValue& data,Config::DiscreteResourceSpec &disres);
    static void ParseRestartPolicy(const UniValue& data,Config::RestartPolicy &repoly);
    static void ParsePlacement(const UniValue& data,Config::Placement &placement);
    static void ParsePlatforms(const UniValue& data,Config::Platform &platform);
    static void ParseNetwork(const UniValue& data,Config::NetWork &network);
    static void ParseMode(const UniValue& data,Config::SerivceMode &mode);
    static void ParseReplicated(const UniValue& data,Config::Replicated &rep);
    static void ParseEndpointSpec(const UniValue& data,Config::EndpointSpec &endpointSpec);
    static void ParseEndSpecPort(const UniValue& data,Config::Port &port);
    static void ParseEndpoint(const UniValue& data,Config::Endpoint &endpoint);
    static void ParseVirtualIPs(const UniValue& data,Config::VirtualIP &virtualip);
    static void ParseUpdateStatus(const UniValue& data,Config::SerivceUpdateStatus &updateStatus);
public:
    
    static void DockerServiceList(const string& serviceData,std::map<std::string,Service> &services);
    Config::ServiceSpec spec;
    Config::ServiceSpec previousSpec;

    Config::Endpoint endpoint;
    Config::SerivceUpdateStatus updateStatus;

public:    
    Service() = default;
    
    Service(std::string id,int idx ,uint64_t createdTime ,uint64_t updateTime,
    Config::ServiceSpec spec,
    Config::ServiceSpec previousSpec,
    Config::Endpoint endpoint,
    Config::SerivceUpdateStatus updateStatus,
    int version=DEFAULT_CSERVICE_API_VERSION):spec(spec),
    previousSpec(previousSpec),endpoint(endpoint),
    updateStatus(updateStatus),
    DockerBase(id,idx,createdTime,updateTime,version){}
    Service(const Service& from){
        ID=from.ID;
        index=from.index;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        previousSpec=from.previousSpec;
        endpoint=from.endpoint;
        updateStatus=from.updateStatus;
    }
    Service& operator=(Service const& from){
        ID=from.ID;
        index=from.index;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        previousSpec=from.previousSpec;
        endpoint=from.endpoint;
        updateStatus=from.updateStatus;
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

class dockerservicefilter:public filterbase{
public:
    bool Mode_replicated = false;
    bool Mode_global = false;

    std::string ToJsonString();
};

#endif //__DOCKERSERVICE__

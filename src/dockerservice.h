#ifndef __DOCKERSERVICE__
#define __DOCKERSERVICE__
#include "dockerbase.h"

class Service:public DockerBase{
public:
    
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
    bool Mode_replicated=true;
    bool Mode_global=true;

    std::string ToJsonString();
    std::string ToString();
};
void dockerservice(const string& serviceData,std::vector<Service> &services);
Service *DcokerServiceJson(const UniValue& data);
void ParseSpec(const UniValue& data,Config::ServiceSpec &spc);
void ParseLabels(const UniValue& data,vector<std::string> &labels);
void ParseTaskTemplate(const UniValue& data,Config::TaskSpec &taskTemplate);
void ParseContainerTemplate(const UniValue& data,Config::ContainerTemplate &contTemp);
void ParseMount(const UniValue& data,Config::Mount &mount);
void ParseArray(const UniValue& data,vector<std::string> &array);
void ParseContainerTemplate(const UniValue& data,Config::Resource &resources);
void ParseLimits(const UniValue& data,Config::Limits &limits);
void ParseResource(const UniValue& data,Config::Resource &resources);
void ParseReservation(const UniValue& data,Config::Reservation &reservations);
void ParseDirReservation(const UniValue& data,Config::DiscreteResourceSpec &disres);
void ParseRestartPolicy(const UniValue& data,Config::RestartPolicy &repoly);
void ParsePlacement(const UniValue& data,Config::Placement &placement);
void ParsePlatforms(const UniValue& data,Config::Platform &platform);
void ParseNetwork(const UniValue& data,Config::NetWork &network);
void ParseMode(const UniValue& data,Config::SerivceMode &mode);
void ParseReplicated(const UniValue& data,Config::Replicated &rep);
void ParseEndpointSpec(const UniValue& data,Config::EndpointSpec &endpointSpec);
void ParseEndSpecPort(const UniValue& data,Config::Port &port);
void ParseEndpoint(const UniValue& data,Config::Endpoint &endpoint);
void ParseVirtualIPs(const UniValue& data,Config::VirtualIP &virtualip);
void ParseUpdateStatus(const UniValue& data,Config::SerivceUpdateStatus &updateStatus);
#endif //__DOCKERSERVICE__

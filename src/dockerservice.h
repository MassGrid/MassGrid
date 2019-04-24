// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERSERVICE_H
#define DOCKERSERVICE_H
#include "primitives/transaction.h"
#include "dockertask.h"
#include "dockernode.h"
namespace Config{

    struct Replicated{
        uint64_t replicas{};
        ADD_SERIALIZE_PROPERTIES(replicas);
    };
    struct Global{
    };
    struct Mode{
        Replicated replicated{};
        bool global{};      //pick one  if global is false,use the replicated
        ADD_SERIALIZE_PROPERTIES(replicated,global);
    };
    struct EndpointPortConfig{
        std::string name{};
        std::string protocol{};   //tcp udp sctp
        int64_t targetPort{};
        int64_t publishedPort{};
        std::string publishMode{};    //default ingress ,host
        ADD_SERIALIZE_PROPERTIES(name,protocol,targetPort,publishedPort,publishMode);
    };
    struct VirtualIP{
        std::string networkID{};
        std::string addr{};
        ADD_SERIALIZE_PROPERTIES(networkID,addr);
    };

    struct EndpointSpec{
        std::string mode{}; //default vip ,vip dnsrr
        vector<EndpointPortConfig> ports{};
        ADD_SERIALIZE_PROPERTIES(mode,ports);
    };
    
    struct Endpoint{
        EndpointSpec spec{};
        vector<EndpointPortConfig> ports{};
        vector<VirtualIP> virtualIPs{};
        ADD_SERIALIZE_PROPERTIES(spec,ports,virtualIPs);
    };
    struct UpdateConfig{
        int64_t parallelism{};
        int64_t delay{};
        std::string failureAction{};  //continue pasue rollback
        int64_t monitor{};
        double maxFailureRatio{};   //default 0
        std::string order{};          // stop-first start-first
        ADD_SERIALIZE_PROPERTIES(parallelism,delay,failureAction,monitor,maxFailureRatio,order);
    };

    struct ServiceSpec{     //create template
        std::string name{};
        Labels labels{};
        TaskSpec taskTemplate{};
        Mode mode{};
        UpdateConfig updateConfig{};
        UpdateConfig rollbackConfig{};
        vector<NetWork> networks{};
        EndpointSpec endpointSpec{};
        ADD_SERIALIZE_PROPERTIES(name,labels,taskTemplate,mode,updateConfig,rollbackConfig,networks,endpointSpec);


        uint256 GetHash() const
        {
            CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
            ss << name;
            ss << labels;
            ss << taskTemplate;
            Mode mode;
            ss << updateConfig;
            ss << rollbackConfig;
            ss << networks;
            ss << endpointSpec;
            return ss.GetHash();
        }
        std::string ToJsonString();
    };
};
class Service:public DockerBase{
    static bool DecodeFromJson(const UniValue& data, Service& service);
    static void ParseSpec(const UniValue& data,Config::ServiceSpec &spc);
    static void ParseSpecLabels(const UniValue& data,Config::Labels &labels);
    static void ParseTaskTemplate(const UniValue& data,Config::TaskSpec &taskTemplate);
    static void ParseResource(const UniValue& data,Config::Resource &resource);
    static void ParseResourceObj(const UniValue& data, Config::ResourceObj &resources);
    static void ParseResGenRes(const UniValue& data, Config::GenericResources &genResources);
    static void ParseResGenNameSpec(const UniValue& data, Config::NamedResourceSpec &namedResourceSpec);
    static void ParseResGenDiscSpec(const UniValue& data, Config::DiscreteResourceSpec &discResourceSpec);
    static void ParseRestartPolicy(const UniValue& data,Config::RestartPolicy &repoly);
    static void ParseUpdateConfig(const UniValue& data,Config::UpdateConfig &upconfig);
    static void ParsePlacement(const UniValue& data,Config::Placement &placement);
    static void ParsePreferences(const UniValue& data,Config::Preferences &preference);
    static void ParsePreferencesSpread(const UniValue& data,Config::Spread &spread);
    static void ParsePlatforms(const UniValue& data,Config::Platform &platform);
    static void ParseNetwork(const UniValue& data,Config::NetWork &network);
    static void ParseLogDriver(const UniValue& data,Config::LogDriver &logdriver);
    static void ParseLogDriverOpt(const UniValue& data,Config::Labels &labels);
    static void ParseMode(const UniValue& data,Config::Mode &mode);
    static void ParseModeReplicated(const UniValue& data,Config::Replicated &rep);
    static void ParseEndpointSpec(const UniValue& data,Config::EndpointSpec &endpointSpec);
    static void ParseEndSpecPort(const UniValue& data,Config::EndpointPortConfig &port);
    static void ParseEndpoint(const UniValue& data,Config::Endpoint &endpoint);
    static void ParseVirtualIPs(const UniValue& data,Config::VirtualIP &virtualip);
    static void ParseUpdateStatus(const UniValue& data,Config::UpdateStatus &updateStatus);
        
    static UniValue ServVerToJson(Config::Version &version);
    static UniValue SpecLabelsToJson(Config::Labels &labels);
    static UniValue TaskTemplateToJson(Config::TaskSpec &taskTemplate);
    static UniValue ResourceToJson(Config::Resource &resource);
    static UniValue ResourceObjToJson(Config::ResourceObj &resources);
    static UniValue ResGenResToJson( Config::GenericResources &genResources);
    static UniValue ResGenNameSpecToJson(Config::NamedResourceSpec &namedResourceSpec);
    static UniValue ResGenDiscSpecToJson(Config::DiscreteResourceSpec &discResourceSpec);
    static UniValue RestartPolicyToJson(Config::RestartPolicy &repoly);
    static UniValue PlacementToJson(Config::Placement &placement);
    static UniValue PreferencesToJson(Config::Preferences &preference);
    static UniValue PreferSpreadToJson(Config::Spread &spread);
    static UniValue PlatformsToJson(Config::Platform &platform);
    static UniValue NetworkToJson(Config::NetWork &network);
    static UniValue LogDriverToJson(Config::LogDriver &logdriver);
    static UniValue LogDriverOptToJson(Config::Labels &labels);
    static UniValue ModeToJson(Config::Mode &mode);
    static UniValue ModeReplicatedToJson(Config::Replicated &rep);
    static UniValue UpdateConfigToJson(Config::UpdateConfig &upconfig);
    static UniValue EndpointSpecToJson(Config::EndpointSpec &endpointSpec);
    static UniValue EndSpecPortToJson(Config::EndpointPortConfig &port);
    static UniValue EndpointToJson(Config::Endpoint &endpoint);
    static UniValue VirtualIPsToJson(Config::VirtualIP &virtualip);
    static UniValue UpdateStatusToJson(Config::UpdateStatus &updateStatus);
    static UniValue ArryToJson(std::vector<std::string> &strArry);

public:
    static UniValue DockerServToJson(Service &services);
    static UniValue SerSpecToJson(Config::ServiceSpec &spec);
public:
    
    Config::ServiceSpec spec{};

    Config::Endpoint endpoint{};
    Config::UpdateStatus updateStatus{};

    int64_t deleteTime{};
    int64_t timeUsage{};
    uint256 txid;
    CAmount price{};
    CAmount payment{};
    Item item{};
    std::string customer{};
    double feeRate{};

    
    map<std::string,Task> mapDockerTaskLists{};
    int GetTaskState(){
        if(mapDockerTaskLists.begin()!=mapDockerTaskLists.end()){
            auto it = mapDockerTaskLists.begin();
            return it->second.status.state;
        }
    }
    static void UpdateTaskList(const string& taskData,std::map<std::string,Service> &services,std::set<std::string>& serviceidSet);
    static void ServiceListUpdateAll(const string& serviceData,std::map<std::string,Service> &services);
    static void ServiceListUpdate(const string& serviceData,std::map<std::string,Service> &services);
public:    
    Service() = default;
    
    Service(std::string id,Config::Version version ,uint64_t createdTime ,uint64_t updateTime,uint64_t requestTime,
    Config::ServiceSpec spec,
    Config::Endpoint endpoint,
    Config::UpdateStatus updateStatus,
    int protocolVersion=DEFAULT_CDOCKER_API_VERSION):spec(spec),
    endpoint(endpoint),
    updateStatus(updateStatus),
    DockerBase(id,version,createdTime,updateTime,requestTime,protocolVersion){}
    Service(const Service& from){
        ID=from.ID;
        version=from.version;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        requestTimeStamp=from.requestTimeStamp;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        endpoint=from.endpoint;
        updateStatus=from.updateStatus;
        mapDockerTaskLists=from.mapDockerTaskLists;
        deleteTime=from.deleteTime;
        timeUsage=from.timeUsage;
        txid=from.txid;
        price=from.price;
        payment=from.payment;
        customer=from.customer;
        feeRate=from.feeRate;
        item=from.item;
    }
    Service& operator=(Service const& from){
        ID=from.ID;
        version=from.version;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        requestTimeStamp=from.requestTimeStamp;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        endpoint=from.endpoint;
        updateStatus=from.updateStatus;
        mapDockerTaskLists=from.mapDockerTaskLists;
        deleteTime=from.deleteTime;
        timeUsage=from.timeUsage;
        txid=from.txid;
        price=from.price;
        payment=from.payment;
        customer=from.customer;
        feeRate=from.feeRate;
        item=from.item;
        return *this;
    }
        ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(version);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(requestTimeStamp);
        READWRITE(nProtocolVersion);
        READWRITE(spec);
        READWRITE(endpoint);
        READWRITE(updateStatus);
        READWRITE(mapDockerTaskLists);
        READWRITE(deleteTime);
        READWRITE(timeUsage);
        READWRITE(txid);
        READWRITE(price);
        READWRITE(payment);
        READWRITE(customer);
        READWRITE(feeRate);
        READWRITE(item);
    }
    std::string ToJsonString();
};

class dockerservicefilter:public filterbase{
public:
    bool Mode_replicated = false;
    bool Mode_global = false;

    std::string ToJsonString();
};
#endif //__DOCKERSERVICE__
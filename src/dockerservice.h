#ifndef DOCKERSERVICE_H
#define DOCKERSERVICE_H
#include "dockertask.h"
#include "primitives/transaction.h"
namespace Config{

    struct Replicated{
        uint64_t replicas{};
        ADD_SERIALIZE_PROPERTIES(replicas);
    };
    struct Global{
    };
    struct Mode{
        Replicated replicated;
        bool global{};      //pick one  if global is false,use the replicated
        ADD_SERIALIZE_PROPERTIES(replicated,global);
    };
    struct EndpointPortConfig{
        std::string name;
        std::string protocol;   //tcp udp sctp
        int64_t targetPort;
        int64_t publishedPort;
        std::string publishMode;    //default ingress ,host
        ADD_SERIALIZE_PROPERTIES(name,protocol,targetPort,publishedPort,publishMode);
    };
    struct VirtualIP{
        std::string networkID;
        std::string addr;
        ADD_SERIALIZE_PROPERTIES(networkID,addr);
    };

    struct EndpointSpec{
        std::string mode; //default vip ,vip dnsrr
        vector<EndpointPortConfig> ports;
        ADD_SERIALIZE_PROPERTIES(mode,ports);
    };
    
    struct Endpoint{
        EndpointSpec spec;
        vector<EndpointPortConfig> ports;
        vector<VirtualIP> virtualIPs;
        ADD_SERIALIZE_PROPERTIES(spec,ports,virtualIPs);
    };
    struct UpdateConfig{
        int64_t parallelism{};
        int64_t delay{};
        std::string failureAction;  //continue pasue rollback
        int64_t monitor{};
        double maxFailureRatio{};   //default 0
        std::string order;          // stop-first start-first
        ADD_SERIALIZE_PROPERTIES(parallelism,delay,failureAction,monitor,maxFailureRatio,order);
    };

    struct ServiceSpec{     //create template
        std::string name;
        Labels labels;
        TaskSpec taskTemplate;
        Mode mode;
        UpdateConfig updateConfig;
        UpdateConfig rollbackConfig;
        vector<NetWork> networks;
        EndpointSpec endpointSpec;
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
        std::string ToString(){
            return "{}";
        }
        
    };
};
class Service:public DockerBase{
    static bool DcokerServiceJson(const UniValue& data, Service& service);
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
    static void ParseArray(const UniValue& data,vector<std::string> &array);

public:
    
    Config::ServiceSpec spec;
    Config::ServiceSpec previousSpec;   //?

    Config::Endpoint endpoint;
    Config::UpdateStatus updateStatus;

    map<std::string,Task> mapDockerTasklists;
    static void DockerServiceList(const string& serviceData,std::map<std::string,Service> &services);
public:    
    Service() = default;
    
    Service(std::string id,Config::Version version ,uint64_t createdTime ,uint64_t updateTime,
    Config::ServiceSpec spec,
    Config::ServiceSpec previousSpec,
    Config::Endpoint endpoint,
    Config::UpdateStatus updateStatus,
    int protocolVersion=DEFAULT_CSERVICE_API_VERSION):spec(spec),
    previousSpec(previousSpec),endpoint(endpoint),
    updateStatus(updateStatus),
    DockerBase(id,version,createdTime,updateTime,protocolVersion){}
    Service(const Service& from){
        ID=from.ID;
        version=from.version;
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
        version=from.version;
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
        READWRITE(version);
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

class DockerGetData{
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    CPubKey pubKeyClusterAddress{};
    CTxIn vin{};
    int64_t sigTime{}; //dkct message times
    std::map<std::string,Service> mapDockerServiceLists;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vin);
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
        READWRITE(mapDockerServiceLists);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << version;
        ss << pubKeyClusterAddress;
        return ss.GetHash();
    }
};
class DockerCreateService{
public:

    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    CPubKey pubKeyClusterAddress{};
    CTxIn vin{};
    int64_t sigTime{}; //dkct message times
    Config::ServiceSpec sspec{};


    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vchSig);
        READWRITE(vin);
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
        READWRITE(sspec);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << vin;
        ss << version;
        ss << pubKeyClusterAddress;
        ss << sspec;
        return ss.GetHash();
    }
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);
    std::string ToJsonString(){
            return "{}";
        }
    std::string ToString(){
        return sspec.GetHash().ToString();
    }
};
typedef DockerCreateService DockerUpdateService;
#endif //__DOCKERSERVICE__
#ifndef DOCKERNODE_H
#define DOCKERNODE_H
#include "dockerbase.h"
namespace Config{
    enum Role{
        ROLE_WORKER=0,
        ROLE_MANAGER
    };
    enum NodeStatusState{
        NODESTATUSSTATE_UNKNOWN=0,
        NODESTATUSSTATE_DOWN,
        NODESTATUSSTATE_READY,
        NODESTATUSSTATE_DISCONNECTED
    };
    enum Availability{
        AVAILABILITY_ACTIVE=0,
        AVAILABILITY_PAUSE,
        AVAILABILITY_DRAIN
    };
    enum reachability{
        REACHABILITY_UNKNOWN=0,
        REACHABILITY_UNREACHABLE,
        REACHABILITY_REACHABLE
    };
    const char* strRole[]={"worker","manager"};
    const char* strNodeStatusState[]={"unknown","down","ready","disconnected"};
    const char* strAvailability[]={"active","pause","drain"};
    const char* strRechability[]={"unknown","unreachable","rechable"};
    struct EngineDescription{
        std::string engineVersion;
        Labels labels;
        vector<Plugins> plugin;
    };
    struct NodeSpec{
        std::string name;
        Labels labels;
        int role;   //worker manager
        int availability;   //active pause drain
    };
    struct NodeDescription{
        std::string hostname;
        Platform platform;
        Reservation resources;  //something?
        EngineDescription engine;
        TLSInfo tlsInfo;
    };
    struct NodeStatus{
        int state;  //unknown down ready disconnected
        std::string message;    
        std::string addr;
    };
    struct ManagerStatus{
        bool Leader{};
        int reachability;   //unknown unreachable reachable
        std::string addr;
    }; 
};
class Node:public DockerBase{
    static bool DockerNodeJson(const UniValue& data, Node& node);
    static void ParseNodeSpec(const UniValue& data,Config::NodeSpec &spec);
    static void ParseNodeLabels(const UniValue& data,Config::Labels &labels);
    static void ParseNodeDescription(const UniValue& data,Config::NodeDescription &decp);
    static void ParseNodePlatform(const UniValue& data,Config::Platform &platform);
    static void ParseNodeResource(const UniValue& data, Config::Limits &limits);
    static void ParseNodeEngine(const UniValue& data, Config::Engine &engine);
    static void ParseNodeSPlugins(const UniValue& data, Config::SPlugins &splugin);
    static void ParseNodeStatus(const UniValue& data, Config::NodeStatus &status);
    static void ParseNodeManageStatus(const UniValue& data, Config::NodeManagerStatus &managerStatus);
public:

    static void DockerNodeList(const string& nodeData,std::map<std::string,Node> &nodes);

    Config::NodeSpec spec;
    Config::NodeDescription description;
    Config::NodeStatus status;
    Config::ManagerStatus managerStatus; 

// function]
public:

    Node() =default;
    Node(std::string id,Config::Version version ,uint64_t createdTime ,uint64_t updateTime,
    Config::NodeSpec spec,
    Config::NodeDescription description,
    Config::NodeStatus status,
    Config::NodeManagerStatus managerStatus, 
    int protocolVersion=DEFAULT_CNODE_API_VERSION):spec(spec),description(description),
    status(status),managerStatus(managerStatus),
    DockerBase(id,version,createdTime,updateTime,protocolVersion){}

    Node(const Node& from){
        ID=from.ID;
        version=from.version;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        description=from.description;
        status=from.status;
        managerStatus=from.managerStatus;
    }

    Node& operator=(Node const& from){
        ID=from.ID;
        version=from.version;
        createdAt=from.createdAt;
        updatedAt=from.updatedAt;
        nProtocolVersion=from.nProtocolVersion;
        spec=from.spec;
        description=from.description;
        status=from.status;
        managerStatus=from.managerStatus;
        return *this;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nProtocolVersion);
        READWRITE(ID);
        READWRITE(version);
        READWRITE(createdAt);
        READWRITE(updatedAt);
    }
    NodeState getNodeState(){return status.state;}
    
    void Update();
    std::string ToString();

};

class dockernodefilter:public filterbase{
public:
    bool Membership_accepted = false;
    bool Membership_pending = false;
    bool Role_manager = false;
    bool Role_worker = false;
    std::string ToJsonString();
};
#endif //__DOCKERSERVICE__

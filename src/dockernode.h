#ifndef DOCKERNODE_H
#define DOCKERNODE_H
#include "dockerbase.h"
class Node:public DockerBase{
    static bool DockerNodeJson(const UniValue& data, Node& node);
    static void ParseNodeSpec(const UniValue& data,Config::NodeSpec &spec);
    static void ParseNodeDescription(const UniValue& data,Config::NodeDescription &decp);
    static void ParseNodePlatform(const UniValue& data,Config::Platform &platform);
    static void ParseNodeResource(const UniValue& data, Config::Limits &limits);
    static void ParseNodeEngine(const UniValue& data, Config::Engine &engine);
    static void ParseNodeSPlugins(const UniValue& data, Config::SPlugins &splugin);
    static void ParseNodeStatus(const UniValue& data, Config::NodeStatus &status);
    static void ParseNodeManageStatus(const UniValue& data, Config::NodeManagerStatus &managerStatus);
public:

    static void DockerNodeList(const string& nodeData,std::map<std::string,Node> &nodes);
    typedef std::string NodeState;
    typedef std::string RoleState;
    const RoleState RoleStateWorker = "worker";
    const RoleState RoleStateManager = "Manager";

    const NodeState NodeStateUnknown = "unknown";
	// NodeStateDown DOWN
	const NodeState NodeStateDown= "down";
	// NodeStateReady READY
	const NodeState NodeStateReady = "ready";
	// NodeStateDisconnected DISCONNECTED
	const NodeState NodeStateDisconnected = "disconnected";

    Config::NodeSpec spec;
    Config::NodeDescription description;
    Config::NodeStatus status;
    Config::NodeManagerStatus managerStatus; 

// function]
public:

    Node() =default;
    Node(std::string id,int idx ,uint64_t createdTime ,uint64_t updateTime,
    Config::NodeSpec spec,
    Config::NodeDescription description,
    Config::NodeStatus status,
    Config::NodeManagerStatus managerStatus, 
    int version=DEFAULT_CNODE_API_VERSION):spec(spec),description(description),
    status(status),managerStatus(managerStatus),
    DockerBase(id,idx,createdTime,updateTime,version){}

    Node(const Node& from){
        ID=from.ID;
        index=from.index;
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
        index=from.index;
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
        READWRITE(index);
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

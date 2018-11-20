#ifndef DOCKERBASE_H  
#define DOCKERBASE_H 
#include <stdint.h>
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include "key.h"
#include "sync.h"
#include "util.h"
#include <univalue.h>
using namespace std;

#define DEFAULT_CDOCKER_API_VERSION 10038
#define DEFAULT_CTASK_API_VERSION 34428
#define DEFAULT_CSERVICE_API_VERSION 34264
#define DEFAULT_CSWARM_API_VERSION 34419
#define DEFAULT_CNODE_API_VERSION 15590

union docker_Version
{
    struct {
    uint8_t Build;
    uint8_t Revision;
    uint8_t Minor;
    uint8_t Major;
    }part;
    uint32_t ver{0};
    uint8_t unv[4];
};
namespace Config{
    enum eStatus{
        created = 0,
        restarting,
        running,
        removing,
        paused,
        exited,
        dead
    };
    static const char* strStatus[] = { "created", "restarting", "running", "removing", "paused", "exited", "dead" };

    struct Status{
        enum eStatus state;
        uint64_t timestamp;
        std::string message;
        std::string err;
    };
    struct Replicated{
        uint replicas;
    };
    struct SerivceMode{
        Replicated replicated;
    };

    struct Port{
        std::string protocol;
        uint targetPort;
        uint publishedPort;
        std::string publishMode;
    };

    struct VirtualIP{
        std::string networkID;
        std::string addr;
    };

    struct EndpointSpec{
        std::string mode;
        vector<Port> ports;
    };
    
    struct Endpoint{
        EndpointSpec spec;
        vector<Port> ports;
        vector<VirtualIP> virtualIPs;
    };

    struct SerivceUpdateStatus{
        std::string state;
        uint64_t startedAt;
        uint64_t completedAt;
        std::string message;
    };
    struct ContainerStatus{
            // uint256 containerID;
            std::string containerID;
            int PID;
            int exitCode;
    };
    struct TaskStatus: Status {
        ContainerStatus containerStatus;
        // PortStatus
    };

    struct NodeStatus{
        std::string state;
        std::string Addr;
    };

    struct NodeManagerStatus{
        bool Leader;
        std::string Reachability;
        std::string Addr;
    }; 
    struct Mount{
        std::string type;
        std::string source;
        std::string target;
    };

    struct DiscreteResourceSpec{
        std::string kind;
        uint value;
    };
    struct NamedResourceSpec{
        std::string kind;
        std::string value;
    };
    struct Limits{
        uint64_t nanoCPUs;
        uint64_t memoryBytes;
    };

    struct Reservation{
        uint64_t nanoCPUs;
        uint64_t memoryBytes;
        vector<struct DiscreteResourceSpec> Reservations;
    };

    struct Resource{
        struct Limits limits;
        struct Reservation reservations;
    };

    struct RestartPolicy{
        std::string condition;
        uint MmaxAttempts;
    };

    struct Platform{
        std::string architecture;
        std::string OS;
    };
    struct Placement{
        vector<std::string> constraints;
        vector<Platform> platforms;
    };
    struct NetWork{
        std::string target;
        vector<std::string> aliases;
    };
    struct Driver{
        std::string name;
    };

    struct DriverState{
        std::string name; 
        // Options
    };
    struct ConfigIP{
        std::string subnet;
        std::string gateway;
    };
    struct IPAMOption{
        Driver driver;
        vector<ConfigIP> configip;
        std::string scope;
    };

    struct ContainerTemplate{
        std::string image;
        vector<std::string> labels;
        vector<std::string> env;
        // Privileges
        vector<struct Mount> mounts;
        std::string isolation;
    };
    struct TaskSpec{
        ContainerTemplate containerTemplate;
        Resource resources;
        RestartPolicy restartPolicy;
        Placement placement;
        vector<struct NetWork> netWorks;
        uint forceUpdate;
        std::string runtime;
    };
    struct ServiceSpec{
        std::string name;
        vector<std::string> labels;
        TaskSpec taskTemplate;
        SerivceMode mode;
        EndpointSpec endpointSpec;
    };
    struct NodeSpec{
        vector<pair<std::string, std::string> > labels;
        std::string role;
        std::string availability;
    };
    struct NetWorkSpec{
        std::string name;
        vector<std::string> labels;
        vector<std::string> driverConfiguration;
        bool ingress;
        IPAMOption IPAMOptions;
    };
    struct SPlugins{
        std::string type;
        std::string name;
    };
    struct Engine{
        std::string engineVersion;
        vector<SPlugins> plugin;
    };
    struct NodeDescription{
        std::string hostname;
        Platform platform;
        Limits resources;
        Engine engine;
    };
    struct NetworkTemplate{
        int nProtocolVersion = DEFAULT_CTASK_API_VERSION;
        int index;
        std::string ID;
        uint64_t createdAt;
        uint64_t updatedAt;
        NetWorkSpec Spec;
        DriverState driverState;
        IPAMOption IPAMOptions;
        vector<std::string> addresses;
    };
};
class DockerBase{
protected:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    int nProtocolVersion = DEFAULT_CDOCKER_API_VERSION;
    int index;
    std::string ID;
    uint64_t createdAt;
    uint64_t updatedAt;
    
public:
    DockerBase() =default;

    DockerBase(std::string id,int idx ,uint64_t createdTime ,uint64_t updateTime,int version=DEFAULT_CDOCKER_API_VERSION):
    nProtocolVersion(version),
    ID(id),
    createdAt(createdTime),
    updatedAt(updateTime){}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(index);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(nProtocolVersion);
    }

    std::string ToString();
};

class filterbase{

public:
    vector<std::string> id;
    vector<std::string> label;
    vector<std::string> name;
    virtual std::string ToJsonString()= 0;
};
uint64_t getDockerTime(const std::string& timeStr);
uint64_t TimeestampStr(const char *nTimeStr);
std::string unixTime2Str(uint64_t unixtime);
#endif //dockerbase
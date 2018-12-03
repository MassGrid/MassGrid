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
#include "utilstrencodings.h"
using namespace std;

#define DEFAULT_CDOCKER_API_VERSION 10038
#define DEFAULT_CTASK_API_VERSION 34428
#define DEFAULT_CSERVICE_API_VERSION 34264
#define DEFAULT_CSWARM_API_VERSION 34419
#define DEFAULT_CNODE_API_VERSION 15590
#define DOCKERREQUEST_API_VERSION 10030
#define DOCKERREQUEST_API_MINSUPPORT_VERSION 10030

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
    typedef std::map<std::string,std::string> Labels;
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

    struct Version{
        uint64_t index;
        ADD_SERIALIZE_PROPERTIES(index);
    };
    struct Status{
        enum eStatus state;
        uint64_t timestamp;
        std::string message;
        std::string err;
    };
    struct HealthCheck{
        std::vector<std::string> test;
        int64_t interval;
        int64_t timeout;
        int64_t retries;
        int64_t startPeriod;
    };

    struct BindOptions{
        std::string propagation;
    };

    struct DriverConfig{
        std::string name;
        Labels options;
    };

    struct VolumeOptions{
        bool nocopy{};
        Labels labels;
        DriverConfig driverConfig;
    };

    struct TmpfsOptions{
        int64_t sizeBytes;
        int mode;
    };

    struct Mount{
        std::string type;
        std::string source;
        std::string target;
        bool readOnly{};
        std::string consistency;
        BindOptions bindOptions;
        VolumeOptions volumeOption;;
        TmpfsOptions tmpfsOptions;
    };

    struct DiscreteResourceSpec{
        std::string kind;
        int64_t value;
    };

    struct NamedResourceSpec{
        std::string kind;
        std::string value;
    };

    struct GenericResources{
        NamedResourceSpec namedResourceSpec;
        DiscreteResourceSpec discreateResourceSpec;
    };

    struct ResourceObj{
        uint64_t nanoCPUs;
        uint64_t memoryBytes;
        vector<struct GenericResources> genericResources;
    };

    struct RestartPolicy{
        std::string condition;
        int64_t delay{};
        int64_t maxAttempts{};
        int64_t window{};
    };

    struct Spread{
        std::string spreadDescriptor;
    };

    struct Preferences{
        Spread spread;
    };


    struct Resource{
        struct ResourceObj limits;
        struct ResourceObj reservations;
    };

    struct Platform{
        std::string architecture;
        std::string OS;
    };

    struct Placement{
        vector<std::string> constraints;
        vector<Preferences> preferences;
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

    struct UpdateStatus{
        std::string state;   //updating paused completed
        uint64_t createdAt;
        uint64_t completedAt;
        std::string message;
    };

    struct NetWorkSpec{
        std::string name;
        Labels labels;
        vector<std::string> driverConfiguration;
        bool ingress;
        IPAMOption IPAMOptions;
    };

    struct Plugins{
        std::string type;
        std::string name;
    };

    struct TLSInfo{
        std::string trustRoot;
        std::string certIssuerSubject;
        std::string certIssuerPublicKey;
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
    Config::Version version;
    std::string ID;
    uint64_t createdAt;
    uint64_t updatedAt;
    
public:
    DockerBase() =default;

    DockerBase(std::string id,Config::Version version,uint64_t createdTime ,uint64_t updateTime,int protocolVersion=DEFAULT_CDOCKER_API_VERSION):
    nProtocolVersion(protocolVersion),
    version(version),
    ID(id),
    createdAt(createdTime),
    updatedAt(updateTime){}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(version);
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
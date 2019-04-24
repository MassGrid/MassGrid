// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
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

#define DEFAULT_CDOCKER_API_VERSION 10031

#define DOCKERREQUEST_API_VERSION 10041
#define DOCKERREQUEST_API_MINSUPPORT_VERSION 10041
#define DOCKERREQUEST_API_MAXSUPPORT_VERSION 10050


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
        int64_t index{};
        ADD_SERIALIZE_PROPERTIES(index);
    };
    struct Status{
        enum eStatus state{};
        int64_t timestamp{};
        std::string message{};
        std::string err{};
    };
    struct HealthCheck{
        std::vector<std::string> test{};
        int64_t intervals{};
        int64_t timeout{};
        int64_t retries{};
        int64_t startPeriod{};
        ADD_SERIALIZE_PROPERTIES(test,intervals,timeout,retries,startPeriod);
    };

    struct BindOptions{
        std::string propagation{};
        ADD_SERIALIZE_PROPERTIES(propagation);
    };

    struct DriverConfig{
        std::string name{};
        Labels options{};
        ADD_SERIALIZE_PROPERTIES(name,options);
    };

    struct VolumeOptions{
        bool nocopy{};
        Labels labels{};
        DriverConfig driverConfig{};
        ADD_SERIALIZE_PROPERTIES(nocopy,labels,driverConfig);
    };

    struct TmpfsOptions{
        int64_t sizeBytes{};
        int mode{};
        ADD_SERIALIZE_PROPERTIES(sizeBytes,mode);
    };

    struct Mount{
        std::string type{};
        std::string source{};
        std::string target{};
        bool readOnly{};
        std::string consistency{};
        BindOptions bindOptions{};
        VolumeOptions volumeOptions{};
        TmpfsOptions tmpfsOptions{};
        ADD_SERIALIZE_PROPERTIES(type,source,target,readOnly,consistency,volumeOptions,tmpfsOptions);
    };

    struct DiscreteResourceSpec{
        std::string kind{};
        int64_t value{};
        ADD_SERIALIZE_PROPERTIES(kind,value);
    };

    struct NamedResourceSpec{
        std::string kind{};
        std::string value{};
        ADD_SERIALIZE_PROPERTIES(kind,value);
    };

    struct GenericResources{
        NamedResourceSpec namedResourceSpec{};
        DiscreteResourceSpec discreateResourceSpec{};
        ADD_SERIALIZE_PROPERTIES(namedResourceSpec,discreateResourceSpec);
    };

    struct ResourceObj{
        int64_t nanoCPUs{};
        int64_t memoryBytes{};
        vector<struct GenericResources> genericResources;
        ADD_SERIALIZE_PROPERTIES(nanoCPUs,memoryBytes,genericResources);
    };

    struct RestartPolicy{
        std::string condition{};
        int64_t delay{};
        int64_t maxAttempts{};
        int64_t window{};
        ADD_SERIALIZE_PROPERTIES(condition,delay,maxAttempts,window);
    };

    struct Spread{
        std::string spreadDescriptor{};
        ADD_SERIALIZE_PROPERTIES(spreadDescriptor);
    };

    struct Preferences{
        Spread spread{};
        ADD_SERIALIZE_PROPERTIES(spread);
    };


    struct Resource{
        struct ResourceObj limits{};
        struct ResourceObj reservations{};
        ADD_SERIALIZE_PROPERTIES(limits,reservations);
    };

    struct Platform{
        std::string architecture{};
        std::string OS{};
        ADD_SERIALIZE_PROPERTIES(architecture,OS);
    };

    struct Placement{
        vector<std::string> constraints{};
        vector<Preferences> preferences{};
        vector<Platform> platforms{};
        ADD_SERIALIZE_PROPERTIES(constraints,preferences,platforms);
    };
    struct NetWork{
        std::string target{};
        vector<std::string> aliases{};
        ADD_SERIALIZE_PROPERTIES(target,aliases);
    };

    struct Driver{
        std::string name{};
        ADD_SERIALIZE_PROPERTIES(name);
    };

    struct DriverState{
        std::string name{}; 
        // Options

        ADD_SERIALIZE_PROPERTIES(name);
    };

    struct ConfigIP{
        std::string subnet{};
        std::string gateway{};

        ADD_SERIALIZE_PROPERTIES(subnet,gateway);
    };
    struct IPAMOption{
        Driver driver{};
        vector<ConfigIP> configip{};
        std::string scope{};
        ADD_SERIALIZE_PROPERTIES(driver,configip);
    };

    struct UpdateStatus{
        std::string state{};   //updating paused completed
        uint64_t createdAt{};
        uint64_t completedAt{};
        std::string message{};
        ADD_SERIALIZE_PROPERTIES(state,createdAt,completedAt,message);
    };

    struct NetWorkSpec{
        std::string name{};
        Labels labels{};
        vector<std::string> driverConfiguration{};
        bool ingress{};
        IPAMOption IPAMOptions{};
        ADD_SERIALIZE_PROPERTIES(name,labels,driverConfiguration,ingress,IPAMOptions);
    };

    struct Plugins{
        std::string type{};
        std::string name{};
        ADD_SERIALIZE_PROPERTIES(type,name);
    };

    struct TLSInfo{
        std::string trustRoot{};
        std::string certIssuerSubject{};
        std::string certIssuerPublicKey{};
        ADD_SERIALIZE_PROPERTIES(trustRoot,certIssuerSubject,certIssuerPublicKey);
    };

    struct NetworkTemplate{
        int nProtocolVersion{};
        int index{};
        std::string ID{};
        uint64_t createdAt{};
        uint64_t updatedAt{};
        NetWorkSpec Spec{};
        DriverState driverState{};
        IPAMOption IPAMOptions{};
        vector<std::string> addresses{};
        ADD_SERIALIZE_PROPERTIES(nProtocolVersion,index,ID,createdAt,updatedAt,Spec,driverState,IPAMOptions,addresses);
    };

};


class DockerBase{
protected:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    int nProtocolVersion = DEFAULT_CDOCKER_API_VERSION;
    Config::Version version{};
    std::string ID{};
    uint64_t createdAt{};
    uint64_t updatedAt{};
    uint64_t requestTimeStamp{};
    
public:
    DockerBase() =default;

    DockerBase(std::string id,Config::Version version,uint64_t createdTime ,uint64_t updateTime,uint64_t requestTime,int protocolVersion=DEFAULT_CDOCKER_API_VERSION):
    nProtocolVersion(protocolVersion),
    version(version),
    ID(id),
    createdAt(createdTime),
    updatedAt(updateTime),
    requestTimeStamp(requestTime){}


    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(ID);
        READWRITE(version);
        READWRITE(createdAt);
        READWRITE(updatedAt);
        READWRITE(requestTimeStamp);
        READWRITE(nProtocolVersion);
    }

    std::string ToString();
};

class filterbase{

public:
    vector<std::string> id{};
    vector<std::string> label{};
    vector<std::string> name{};
    virtual std::string ToJsonString()= 0;
};
uint64_t getDockerTime(const std::string& timeStr);
uint64_t TimeestampStr(const char *nTimeStr);
std::string unixTime2Str(uint64_t unixtime);

#endif //dockerbase
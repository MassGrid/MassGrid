#ifndef DOCKERCONTAINER_H
#define DOCKERCONTAINER_H
#include "dockerbase.h"
namespace Config{

    struct DNSConfig{
        vector<std::string> nameservers;
        vector<std::string> search;
        vector<std::string> options;
    };

    struct File{
        std::string name;
        std::string uid;
        std::string gid;
        uint32_t mode;
    };

    struct Secret{
        File file;
        std::string secretID;
        std::string secretName;
    };

    struct Config{
        File file;
        std::string configID;
        std::string configName;
    };
        
    struct LogDriver{
        std::string name;
        map<std::string,std::string> options;
    };

    struct ContainerStatus{
        std::string containerID;
        int PID;
        int exitCode;
    };
    
    struct ContainerSpec{
        std::string image;
        Labels labels;
        vector<std::string> command;
        vector<std::string> args;
        std::string hostname;
         vector<std::string> env;
        std::string dir;
        std::string user;
        vector<std::string> groups;
        // Privileges
        bool tty{};
        bool openstdin{};
        bool readOnly{};
        vector<struct Mount> mounts;
        std::string stopSignal;
        int64_t stopGracePeriod;
        HealthCheck healthCheck;
        vector<std::string> hosts;
        DNSConfig dnsConfig;
        vector<Secret> secrets;
        vector<Config> configs;
        std::string isolation;
    };
};
void ParseContainerSpec(const UniValue& data,Config::ContainerSpec &containerSpec);
void ParseContainerSpecLabels(const UniValue& data,Config::Labels &labels);
void ParseContainerSpecMount(const UniValue& data,Config::Mount &mount);
void ParseContSpecMountBind(const UniValue& data,Config::BindOptions &bindOption);
void ParseContSpecMountVol(const UniValue& data,Config::VolumeOptions &voloption);
void ParseContSpecMountVolLabel(const UniValue& data,Config::Labels &labels);
void ParseContSpecMountVolDriv(const UniValue& data,Config::DriverConfig &drivConfig);
void ParseContSpecMountVolLabelOP(const UniValue& data,Config::Labels &labels);
void ParseContSpecMountTfs(const UniValue& data,Config::TmpfsOptions &tmpfsOption);
void ParseContainerSpecHealt(const UniValue& data,Config::HealthCheck &heltCheck);
void ParseContainerSpecDNS(const UniValue& data,Config::DNSConfig &dnsConfig);
void ParseContainerSpecSec(const UniValue& data,Config::Secret &secret);
void ParseContSpecSecFile(const UniValue& data,Config::File &file);
void ParseContainerSpeConf(const UniValue& data,Config::Config &config);
void ParseArray(const UniValue& data,vector<std::string> &array);
#endif //DOCKERCONTAINER_H

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
        std::string UID;
        std::string GID;
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
        Labels env;
        std::string dir;
        std::string user;
        vector<std::string> groups;
        // Privileges
        bool tty{};
        bool openstdin{};
        bool ReadOnly{};
        vector<struct Mount> mounts;
        std::string stopSignal;
        int64_t stopGracePeriod;
        HealthCheck healthCheck;
        vector<std::string> hosts;
        DNSConfig dnsConfig;
        vector<Secret> secrets;
        vector<Config> configs;
        // std::string isolation;
    };
};
#endif //DOCKERCONTAINER_H

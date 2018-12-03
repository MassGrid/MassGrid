#include "dockercontainer.h"
#include "dockertask.h"
#include "dockerservice.h"
//****
void ParseContainerSpec(const UniValue& data,Config::ContainerSpec &containerSpec)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Isolation"){
                containerSpec.isolation=tdata.get_str();
            }else if(vKeys[i]=="Image"){
                containerSpec.image=tdata.get_str();
            }else if(vKeys[i]=="Hostname"){
                containerSpec.hostname=tdata.get_str();
            }else if(vKeys[i]=="Dir"){
                containerSpec.dir=tdata.get_str();
            }else if(vKeys[i]=="User"){
                containerSpec.user=tdata.get_str();
            }else if(vKeys[i]=="StopSignal"){
                containerSpec.stopSignal=tdata.get_str();
            }
        }
        if(tdata.isObject()){
            if(vKeys[i]=="Labels"){
                ParseContainerSpecLabels(tdata,containerSpec.labels);
            }else if(vKeys[i]=="HealthCheck"){
                ParseContainerSpecHealt(tdata,containerSpec.healthCheck);
            }else if(vKeys[i]=="DNSCOnfig"){
                ParseContainerSpecDNS(tdata,containerSpec.dnsConfig);
            }else if(vKeys[i]=="Secrets"){
                Config::Secret secrets;
                ParseContainerSpecSec(tdata,secrets);
                containerSpec.secrets.push_back(secrets);
            }else if(vKeys[i]=="Configs"){
                Config::Config config;
                ParseContainerSpeConf(tdata,config);
                containerSpec.configs.push_back(config);
            }
        }
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Command"){
                ParseArray(tdata,containerSpec.command);
            }else if(vKeys[i]=="Args"){
                ParseArray(tdata,containerSpec.args);
            }else if(vKeys[i]=="Env"){
                ParseArray(tdata,containerSpec.env);
            }else if(vKeys[i]=="Groups"){
                ParseArray(tdata,containerSpec.groups);
            }else if(vKeys[i]=="Mounts"){
                for(size_t j=0;j<tdata.size();j++){
                    Config::Mount mount;
                    ParseContainerSpecMount(tdata[j],mount);
                    containerSpec.mounts.push_back(mount);
                }
            }else if(vKeys[i]=="Hosts"){
                ParseArray(tdata,containerSpec.hosts);
            }
        }
        if(data[vKeys[i]].isBool()){
             if(vKeys[i]=="TTY"){
                containerSpec.tty=tdata.get_bool();
            }else if(vKeys[i]=="OpenStdin"){
                containerSpec.openstdin=tdata.get_bool();
            }else if(vKeys[i]=="ReadOnly"){
                containerSpec.readOnly=tdata.get_bool();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="StopGracePeriod"){
                containerSpec.stopGracePeriod=tdata.get_int64();
            }
        }
    }
}
void ParseContainerSpecLabels(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="key") labels.insert(std::make_pair("com.massgrid.key",tdata.get_str()));
        }
    }
}
void ParseContainerSpecMount(const UniValue& data,Config::Mount &mount)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Type") mount.type=tdata.get_str();
            else if(vKeys[i]=="Source") mount.source=tdata.get_str();
            else if(vKeys[i]=="Target") mount.target=tdata.get_str();
            else if(vKeys[i]=="Consistency") mount.consistency=tdata.get_str();
        }
        if(data[vKeys[i]].isBool()){
            if(vKeys[i]=="ReadOnly"){
                mount.readOnly=tdata.getBool();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="BindOptions"){
                ParseContSpecMountBind(tdata,mount.bindOptions);
            }else if(vKeys[i]=="VolumeOptions"){
                ParseContSpecMountVol(tdata,mount.volumeOption);
            }else if(vKeys[i]=="TmpfsOptions"){
                ParseContSpecMountTfs(tdata,mount.tmpfsOptions);
            }
        }
    }
}
void ParseContSpecMountBind(const UniValue& data,Config::BindOptions &bindOption)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Propagation"){
                bindOption.propagation=tdata.get_str();
            }
        }
    }
}
void ParseContSpecMountVol(const UniValue& data,Config::VolumeOptions &voloption)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isBool()){
            if(vKeys[i]=="NoCopy"){
                voloption.nocopy=tdata.get_bool();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Labels"){//key?
                ParseContSpecMountVolLabel(tdata,voloption.labels);
            }else if(vKeys[i]=="DriverConfig"){
                ParseContSpecMountVolDriv(tdata,voloption.driverConfig);
            }
        }
    }
}
void ParseContSpecMountVolLabel(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="pubkey") labels.insert(std::make_pair("com.massgrid.pubkey",tdata.get_str()));
        }
    }
}
void ParseContSpecMountVolDriv(const UniValue& data,Config::DriverConfig &drivConfig)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Name"){
                drivConfig.name=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="Options"){//key?
                ParseContSpecMountVolLabelOP(tdata,drivConfig.options);
            }
        }
    }
}
void ParseContSpecMountVolLabelOP(const UniValue& data,Config::Labels &labels)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="pubkey") labels.insert(std::make_pair("com.massgrid.pubkey",tdata.get_str()));
        }
    }
}
void ParseContSpecMountTfs(const UniValue& data,Config::TmpfsOptions &tmpfsOption)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="SizeBytes"){
                tmpfsOption.sizeBytes=tdata.get_int64();
            }else if(vKeys[i]=="Mode"){
                tmpfsOption.mode=tdata.get_int();
            }
        }
    }
}
void ParseContainerSpecHealt(const UniValue& data,Config::HealthCheck &heltCheck)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Test"){
                ParseArray(tdata,heltCheck.test);
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Interval"){
                heltCheck.interval=tdata.get_int64();
            }else if(vKeys[i]=="Timeout"){
                heltCheck.timeout=tdata.get_int64();
            }else if(vKeys[i]=="Retries"){
                heltCheck.retries=tdata.get_int64();
            }else if(vKeys[i]=="StartPeriod"){
                heltCheck.startPeriod=tdata.get_int64();
            }
        }
    }
}
void ParseContainerSpecDNS(const UniValue& data,Config::DNSConfig &dnsConfig)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isArray()){
            if(vKeys[i]=="Nameservers"){
                ParseArray(tdata,dnsConfig.nameservers);
            }else if(vKeys[i]=="Search"){
                ParseArray(tdata,dnsConfig.search);
            }else if(vKeys[i]=="Options"){
                ParseArray(tdata,dnsConfig.options);
            }
        }
    }
}
void ParseContainerSpecSec(const UniValue& data,Config::Secret &secret)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="File"){
               ParseContSpecSecFile(tdata,secret.file);
            }
        }
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="SecretID"){
               secret.secretID=tdata.get_str();
            }else if(vKeys[i]=="SecretName"){
                secret.secretName=tdata.get_str();
            }
        }
    }
}
void ParseContSpecSecFile(const UniValue& data,Config::File &file)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="Name"){
               file.name=tdata.get_str();
            }else if(vKeys[i]=="UID"){
                file.uid=tdata.get_str();
            }else if(vKeys[i]=="GID"){
                file.gid=tdata.get_str();
            }
        }
        if(data[vKeys[i]].isNum()){
            if(vKeys[i]=="Mode"){
               file.mode=tdata.get_int64();
            }
        }
    }
}
void ParseContainerSpeConf(const UniValue& data,Config::Config &config)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        UniValue tdata(data[vKeys[i]]);
        if(data[vKeys[i]].isObject()){
            if(vKeys[i]=="File"){
               ParseContSpecSecFile(tdata,config.file);
            }
        }
        if(data[vKeys[i]].isStr()){
            if(vKeys[i]=="ConfigID"){
               config.configID=tdata.get_str();
            }else if(vKeys[i]=="ConfigName"){
                config.configName=tdata.get_str();
            }
        }
    }
}
void ParseArray(const UniValue& data,vector<std::string> &array)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        array.push_back(data[vKeys[i]].get_str());
    }
}
//****
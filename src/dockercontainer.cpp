#include "dockercontainer.h"
#include "dockertask.h"
#include "dockerservice.h"
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
UniValue ContainerSpecToJson(Config::ContainerSpec &containerSpec)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Isolation",containerSpec.isolation));
        data.push_back(Pair("Image",containerSpec.image)); 
        data.push_back(Pair("Hostname",containerSpec.hostname)); 
        data.push_back(Pair("Dir",containerSpec.dir)); 
        data.push_back(Pair("User",containerSpec.user)); 
        data.push_back(Pair("StopSignal",containerSpec.stopSignal));
        data.push_back(Pair("TTY",containerSpec.tty)); 
        data.push_back(Pair("OpenStdin",containerSpec.openstdin)); 
        data.push_back(Pair("ReadOnly",containerSpec.readOnly)); 
        data.push_back(Pair("StopGracePeriod",containerSpec.stopGracePeriod));
    }
    {
        UniValue arryCmd(UniValue::VARR);
        arryCmd=ArryToJson(containerSpec.command);
        data.push_back(Pair("Command",arryCmd));

        UniValue arryArgs(UniValue::VARR);
        arryArgs=ArryToJson(containerSpec.args);
        data.push_back(Pair("Args",arryArgs));

        UniValue arryEnv(UniValue::VARR);
        arryEnv=ArryToJson(containerSpec.env);
        data.push_back(Pair("Env",arryEnv));

        UniValue arryGroups(UniValue::VARR);
        arryGroups=ArryToJson(containerSpec.groups);
        data.push_back(Pair("Groups",arryGroups));

        std::vector<Config::Mount> mounts=containerSpec.mounts;
        UniValue arry(UniValue::VARR);
        for(size_t j=0;j<mounts.size();j++){
            UniValue objMount(UniValue::VOBJ);
            objMount=MountToJson(mounts[j]);
            arry.push_back(objMount);
        }
        data.push_back(Pair("Mounts", arry));

        UniValue arryHosts(UniValue::VARR);
        arryHosts=ArryToJson(containerSpec.hosts);
        data.push_back(Pair("Hosts",arryHosts));
    }
    {
        UniValue objLabel(UniValue::VOBJ);
        objLabel=SpecLabelsToJson(containerSpec.labels);
        data.push_back(Pair("Labels",objLabel));

        UniValue objHelCheck(UniValue::VOBJ);
        objHelCheck=SpecHealtToJson(containerSpec.healthCheck);
        data.push_back(Pair("HealthCheck",objHelCheck));

        UniValue objDNSC(UniValue::VOBJ);
        objDNSC=SpecDNSToJson(containerSpec.dnsConfig);
        data.push_back(Pair("DNSCOnfig",objDNSC));

        vector<Config::Secret> secrets=containerSpec.secrets;
        UniValue arrySecret(UniValue::VARR);
        for(size_t j=0;j<secrets.size();j++){
            UniValue objSecret(UniValue::VOBJ);
            objSecret=SpecSecToJson(secrets[j]);
            arrySecret.push_back(objSecret);
        }
        data.push_back(Pair("Secrets",arrySecret));

       vector<Config::Config> config=containerSpec.configs;
        UniValue arryConfig(UniValue::VARR);
        for(size_t j=0;j<config.size();j++){
            UniValue objConfig(UniValue::VOBJ);
            objConfig=SpecConfToJson(config[j]);
            arryConfig.push_back(objConfig);
        }
        data.push_back(Pair("Configs",arryConfig));
    }
    return data;
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
UniValue SpecLabelsToJson(Config::Labels &labels)
{
    UniValue data(UniValue::VOBJ);
    {
        for(auto &iter: labels) {
            if(iter.first=="com.massgrid.key")
                data.push_back(Pair("key",iter.second));
        }
    }
    return data;
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
                ParseContSpecMountVol(tdata,mount.volumeOptions);
            }else if(vKeys[i]=="TmpfsOptions"){
                ParseContSpecMountTfs(tdata,mount.tmpfsOptions);
            }
        }
    }
}
UniValue MountToJson(Config::Mount &mount)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Type",mount.type));
        data.push_back(Pair("Source",mount.source)); 
        data.push_back(Pair("Target",mount.target)); 
        data.push_back(Pair("Consistency",mount.consistency)); 
        data.push_back(Pair("ReadOnly",mount.readOnly)); 
    }
    {
        UniValue objBind(UniValue::VOBJ);
        objBind=MountBindToJson(mount.bindOptions);
        data.push_back(Pair("BindOptions",objBind));

        UniValue objVol(UniValue::VOBJ);
        objVol=MountVolToJson(mount.volumeOptions);
        data.push_back(Pair("VolumeOptions",objVol));

        UniValue objTmpfs(UniValue::VOBJ);
        objTmpfs=MountTfsToJson(mount.tmpfsOptions);
        data.push_back(Pair("TmpfsOptions",objTmpfs));
    }
    return data;
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
UniValue MountBindToJson(Config::BindOptions &bindOption)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Propagation",bindOption.propagation));
    }
    return data;
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
UniValue MountVolToJson(Config::VolumeOptions &voloption)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("NoCopy",voloption.nocopy));
    }
    {
        UniValue objLabel(UniValue::VOBJ);
        objLabel=MountVolLabelToJson(voloption.labels);
        data.push_back(Pair("Labels",objLabel));

        UniValue objDriv(UniValue::VOBJ);
        objDriv=MountVolDrivToJson(voloption.driverConfig);
        data.push_back(Pair("DriverConfig",objDriv));
    }
    return data;
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
UniValue MountVolLabelToJson(Config::Labels &labels)
{
    UniValue data(UniValue::VOBJ);
    {
        for(auto &iter: labels) {
            if(iter.first=="com.massgrid.pubkey")
                data.push_back(Pair("pubkey",iter.second));
        }
    }
    return data;
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
UniValue MountVolDrivToJson(Config::DriverConfig &drivConfig)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Name",drivConfig.name));
    }
    {
        UniValue objDrivOP(UniValue::VOBJ);
        objDrivOP=MountVolDrivOPToJson(drivConfig.options);
        data.push_back(Pair("Options",objDrivOP));
    }
    return data;
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
UniValue MountVolDrivOPToJson(Config::Labels &labels)
{
    UniValue data(UniValue::VOBJ);
    {
        for(auto &iter: labels) {
            if(iter.first=="com.massgrid.pubkey")
                data.push_back(Pair("pubkey",iter.second));
        }
    }
    return data;
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
UniValue MountTfsToJson(Config::TmpfsOptions &tmpfsOption)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("SizeBytes",tmpfsOption.sizeBytes));
        data.push_back(Pair("Mode",tmpfsOption.mode));
    }
    return data;
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
                heltCheck.intervals=tdata.get_int64();
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
UniValue SpecHealtToJson(Config::HealthCheck &heltCheck)
{
    UniValue data(UniValue::VOBJ);
    {
        UniValue arryTest(UniValue::VARR);
        arryTest=ArryToJson(heltCheck.test);
        data.push_back(Pair("Test",arryTest));
    }
    {
        data.push_back(Pair("Interval",heltCheck.intervals));
        data.push_back(Pair("Timeout",heltCheck.timeout));
        data.push_back(Pair("Retries",heltCheck.retries));
        data.push_back(Pair("StartPeriod",heltCheck.startPeriod));
    }
    return data;
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
UniValue SpecDNSToJson(Config::DNSConfig &dnsConfig)
{
    UniValue data(UniValue::VOBJ);
    {
        UniValue arryNS(UniValue::VARR);
        arryNS=ArryToJson(dnsConfig.nameservers);
        data.push_back(Pair("Nameservers",arryNS));

        UniValue arrySearch(UniValue::VARR);
        arrySearch=ArryToJson(dnsConfig.search);
        data.push_back(Pair("Search",arrySearch));

        UniValue arryOp(UniValue::VARR);
        arryOp=ArryToJson(dnsConfig.options);
        data.push_back(Pair("Options",arryOp));
    }
    return data;
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
UniValue SpecSecToJson(Config::Secret &secret)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("SecretID",secret.secretID));
        data.push_back(Pair("SecretName",secret.secretName));
    }
    {
        UniValue objFile(UniValue::VOBJ);
        objFile=SpecSecFileToJson(secret.file);
        data.push_back(Pair("File",objFile));
    }
    return data;
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
UniValue SpecSecFileToJson(Config::File &file)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("Name",file.name));
        data.push_back(Pair("UID",file.uid));
        data.push_back(Pair("GID",file.gid));
        data.push_back(Pair("Mode",file.mode));
    }
    return data;
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
UniValue SpecConfToJson(Config::Config &config)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("ConfigID",config.configID));
        data.push_back(Pair("ConfigName",config.configName));
    }
    {
        UniValue objFile(UniValue::VOBJ);
        objFile=SpecSecFileToJson(config.file);
        data.push_back(Pair("File",objFile));
    }
    return data;
}
void ParseArray(const UniValue& data,vector<std::string> &array)
{
    std::vector<std::string> vKeys=data.getKeys();
    for(size_t i=0;i<data.size();i++){
        array.push_back(data[vKeys[i]].get_str());
    }
}
UniValue ArryToJson(std::vector<std::string> &strArry)
{
    UniValue arry(UniValue::VARR);
    for(size_t i=0;i<strArry.size();i++){
        arry.push_back(strArry[i]);
    }
    return arry;
}
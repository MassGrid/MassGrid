#include <algorithm>
#include "dockerserverman.h"
#include "netbase.h"
#include "masternode-sync.h"
#include "dockerman.h"
#include "net.h"
#include "wallet/wallet.h"
#include "dockercluster.h"
#include <boost/lexical_cast.hpp>
CDockerServerman dockerServerman;

void CDockerServerman::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman){
    
    if (!masternodeSync.IsSynced()) return;

    LogPrintf("====>CDockerServerman::ProcessMessage start strcommand：%s\n",strCommand);
    
    if (strCommand == NetMsgType::GETDNDATA) { //server
        
        LogPrint("docker","CDockerServerman::ProcessMessage GETDNDATA Started\n");
        if (!fMasterNode) return;
        DockerGetData mdndata;
        CPubKey pubkey;
        vRecv >> pubkey;
        LogPrint("docker", "CDockerServerman::ProcessMessage GETDNDATA -- pubkey =%s\n", pubkey.ToString().substr(0,65));
        
        mdndata.mapDockerServiceLists.clear();
        for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
            if(it->second.spec.labels.find("com.massgrid.pubkey") != it->second.spec.labels.end() && 
                    it->second.spec.labels["com.massgrid.pubkey"] == pubkey.ToString().substr(0,65)){
                mdndata.mapDockerServiceLists.insert(*it);
            }
        }
        mdndata.version = DOCKERREQUEST_API_VERSION;
        mdndata.sigTime = GetAdjustedTime();
        
        LogPrintf("CDockerServerman::ProcessMessage -- Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);

    }else if(strCommand == NetMsgType::DNDATA){     //cluster

        LogPrintf("====>dockerserverman dndata\n");
        LogPrint("docker","CDockerServerman::ProcessMessage DNDATA Started\n");
        DockerGetData mdndata;
        vRecv >> mdndata;
        if(mdndata.version < DOCKERREQUEST_API_MINSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --mdndata version %d is too old %d\n", mdndata.version,DOCKERREQUEST_API_MINSUPPORT_VERSION);
            return;
        }
         
        dockercluster.mapDockerServiceLists = mdndata.mapDockerServiceLists;
        dockercluster.sigTime = mdndata.sigTime;

        LogPrintf("CDockerServerman::ProcessMessage sigTime:%d \n",mdndata.sigTime);

        // std::map<std::string,Service>::iterator iter = mdndata.mapDockerServiceLists.begin();
        // std::string tmpid = iter->first;
        // std::string servspecjson = 
        ///Service::DockerServSpecToJson(iter->second);
        // LogPrintf("======>CDockerServerman::ProcessMessage tmpid:%s\v",tmpid);
        // LogPrintf("======>CDockerServerman::ProcessMessage service:%s\n",iter->second.spec.ToJsonString());


    }else if(strCommand == NetMsgType::CREATESERVICE){
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        if (!fMasterNode) return;
        DockerCreateService createService;
        vRecv >> createService;
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE createService hash %s\n",createService.ToString());
        if(createService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --createService version %d is too old %d\n", createService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION);
            return;
        }
        if(CheckAndCreateServiveSpec(createService)){
            DockerGetData mdndata;
                mdndata.mapDockerServiceLists.clear();
            for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
                if(it->second.spec.labels.find("com.massgrid.pubkey") != it->second.spec.labels.end() && 
                    it->second.spec.labels["com.massgrid.pubkey"] == createService.pubKeyClusterAddress.ToString().substr(0,65)){
                    mdndata.mapDockerServiceLists.insert(*it);
                }
            }
            mdndata.version = DOCKERREQUEST_API_VERSION;
            mdndata.sigTime = GetAdjustedTime();
            
            LogPrintf("CDockerServerman::ProcessMessage -- CREATESERVICE Sent DNDATA to peer %d\n", pfrom->id);
            connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
        }
    }else if(strCommand == NetMsgType::UPDATESERVICE){
        LogPrint("docker","CDockerServerman::ProcessMessage UPDATESERVICE Started\n");
        if (!fMasterNode) return;
        DockerUpdateService updateService;
        vRecv >> updateService;
        if(updateService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --updateService version %d is too old %d\n", updateService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION);
            return;
        }
        if(CheckAndUpdateServiceSpec(updateService)){
            DockerGetData mdndata;
                mdndata.mapDockerServiceLists.clear();
            for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
                if(it->second.spec.labels.find("com.massgrid.pubkey") != it->second.spec.labels.end() && 
                    it->second.spec.labels["com.massgrid.pubkey"] == updateService.pubKeyClusterAddress.ToString().substr(0,65)){
                    mdndata.mapDockerServiceLists.insert(*it);
                }
            }
            mdndata.version = DOCKERREQUEST_API_VERSION;
            mdndata.sigTime = GetAdjustedTime();
            
            LogPrintf("CDockerServerman::ProcessMessage -- CREATESERVICE Sent DNDATA to peer %d\n", pfrom->id);
            connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
        }
    }
        LogPrintf("====>CDockerServerman::ProcessMessage end\n");
}
bool CDockerServerman::CheckAndCreateServiveSpec(DockerCreateService createService){

    LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec Started\n");
    Config::ServiceSpec spec;
    // 1.first check time
    if(createService.sigTime > GetAdjustedTime() + 60 * 5 && createService.sigTime < GetAdjustedTime() - 60 * 5){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec sigTime is invaild\n");
        return false;
    }
    
    //  2.check transactions
    if(createService.vin != CTxIn()){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec vin is invaild\n");
        return false;
    }

    //  3. checkSignature
    if(!createService.CheckSignature(createService.pubKeyClusterAddress)){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- CheckSignature() failed\n");
        return false;
    }

    //  4. update spec

    if(!createService.serviceName.empty())
        spec.name = createService.serviceName.substr(0,10) + "_" + createService.ToString().substr(0,4);
    else
        spec.name = createService.ToString().substr(0,15);

    if(!(createService.memory_byte > 0 && createService.memory_byte < DOCKER_MAX_MEMORY_BYTE))
        return false;
    if(!(createService.cpu > 0 && createService.cpu < DOCKER_MAX_CPU_NUM))
        return false;
    if(!(createService.gpu > 0 && createService.gpu < DOCKER_MAX_GPU_NUM))
        return false;
    
    spec.labels["com.massgrid.pubkey"] = createService.pubKeyClusterAddress.ToString().substr(0,65);
    spec.mode.replicated.replicas = 1;
    
    if(!createService.image.empty())
        spec.taskTemplate.containerSpec.image = createService.image;
    else
        spec.taskTemplate.containerSpec.image = "wany/cuda9.1-base";
    spec.taskTemplate.containerSpec.user= "root";
    
    spec.taskTemplate.resources.limits.memoryBytes = createService.memory_byte;
    spec.taskTemplate.resources.limits.nanoCPUs = createService.cpu;

    Config::GenericResources otherresources;
    otherresources.discreateResourceSpec.kind = createService.gpuname;
    otherresources.discreateResourceSpec.value = createService.gpu;
    spec.taskTemplate.resources.reservations.genericResources.push_back(otherresources);

    spec.taskTemplate.restartPolicy.condition = "on-failure";
    spec.taskTemplate.containerSpec.command.push_back("start.sh");

    if(createService.n2n_community.empty())
        spec.taskTemplate.containerSpec.env.push_back("N2N_NAME=massgridn2n");
    else
        spec.taskTemplate.containerSpec.env.push_back("N2N_NAME="+createService.n2n_community.substr(0,20));
    
    std::string ipaddr = dockerman.GetFreeIP();
    spec.taskTemplate.containerSpec.env.push_back("N2N_KEY=massgrid1208");
    spec.taskTemplate.containerSpec.env.push_back("N2N_LOCALIP=" + ipaddr);
    spec.taskTemplate.containerSpec.env.push_back("N2N_SERVERIP=" + dockerman.managerAddr + ":" + boost::lexical_cast<std::string>(dockerman.n2nServerPort));
    spec.taskTemplate.containerSpec.env.push_back("SSH_PUBKEY=" + createService.ssh_pubkey);
    
    spec.taskTemplate.placement.constraints.push_back("node.role == worker");
    Config::Mount mount;
    mount.target="/dev/net";
    mount.source="/dev/net";
    spec.taskTemplate.containerSpec.mounts.push_back(mount);

    if(!dockerman.PushMessage(Method::METHOD_SERVICES_CREATE,"",spec.ToJsonString())){
        dockerman.SetIPBook(ipaddr,false);
        return false;
    }
    return true;

}
bool CDockerServerman::CheckAndUpdateServiceSpec(DockerUpdateService updateService){

    LogPrint("docker","CDockerServerman::CheckAndUpdateServiceSpec Started\n");

    Config::ServiceSpec spec;
    auto iter = dockerman.mapDockerServiceLists.begin();
    if((iter = dockerman.mapDockerServiceLists.find(updateService.serviceid)) == dockerman.mapDockerServiceLists.end())
        return false;
    spec = iter->second.spec;
    // 1.first check time
    if(updateService.sigTime > GetAdjustedTime() + 60 * 5 && updateService.sigTime < GetAdjustedTime() - 60 * 5){
        LogPrintf("CDockerServerman::CheckAndUpdateServiceSpec sigTime is invaild\n");
        return false;
    }
    
    //  2.check transactions
    if(updateService.vin != CTxIn()){
        LogPrintf("CDockerServerman::CheckAndUpdateServiceSpec vin is invaild\n");
        return false;
    }

    //  3. checkSignature
    if(!updateService.CheckSignature(updateService.pubKeyClusterAddress)){
        LogPrintf("CDockerServerman::CheckAndUpdateServiceSpec -- CheckSignature() failed\n");
        return false;
    }

    //  4. update spec

    if(!updateService.serviceName.empty())
        spec.name = updateService.serviceName.substr(0,10) + "_" + updateService.ToString().substr(0,4);
    else
        spec.name = updateService.ToString().substr(0,15);

    if(!(updateService.memory_byte < DOCKER_MAX_MEMORY_BYTE))
        return false;
    if(!(updateService.cpu < DOCKER_MAX_CPU_NUM))
        return false;
    if(!(updateService.gpu < DOCKER_MAX_GPU_NUM))
        return false;
    if(spec.labels["com.massgrid.pubkey"] != updateService.pubKeyClusterAddress.ToString().substr(0,65))
        return false;
    
    if(!updateService.image.empty())
        spec.taskTemplate.containerSpec.image= updateService.image;
    
    if(updateService.memory_byte != 0)
        spec.taskTemplate.resources.limits.memoryBytes = updateService.memory_byte;
    if(updateService.cpu != 0)
        spec.taskTemplate.resources.limits.nanoCPUs = updateService.cpu;

    if(updateService.gpu != 0){
        Config::GenericResources otherresources;
        otherresources.discreateResourceSpec.kind = updateService.gpuname;
        otherresources.discreateResourceSpec.value = updateService.gpu;
        spec.taskTemplate.resources.reservations.genericResources.clear();
        spec.taskTemplate.resources.reservations.genericResources.push_back(otherresources);
    }

    if(!updateService.n2n_community.empty())
        spec.taskTemplate.containerSpec.env.push_back("N2N_NAME="+updateService.n2n_community.substr(0,20));
    if(!updateService.ssh_pubkey.empty())
        spec.taskTemplate.containerSpec.env.push_back("SSH_PUBKEY=" + updateService.ssh_pubkey);
    
    return dockerman.PushMessage(Method::METHOD_SERVICES_UPDATE,updateService.serviceid,spec.ToJsonString());

}

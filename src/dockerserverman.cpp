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
    
    if (strCommand == NetMsgType::GETDNDATA) { //server
        
        LogPrint("docker","CDockerServerman::ProcessMessage GETDNDATA Started\n");
        if (!fMasterNode) return;
        DockerGetData mdndata;
        CPubKey pubkey;
        vRecv >> pubkey;
        LogPrint("docker", "CDockerServerman::ProcessMessage GETDNDATA -- pubkey =%s\n", pubkey.ToString().substr(0,65));

        mdndata.mapDockerServiceLists.clear();

        dockerservicefilter serfilter;
        serfilter.label.push_back("com.massgrid.pubkey="+pubkey.ToString().substr(0,65));
        if(!dockerman.PushMessage(Method::METHOD_SERVICES_LISTS,"",serfilter.ToJsonString(),false)){
            LogPrint("docker","CDockerServerman::ProcessMessage GETDNDATA service list failed -- pubkey =%s\n", pubkey.ToString().substr(0,65));
        }
        for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
            if(it->second.spec.labels.find("com.massgrid.pubkey") != it->second.spec.labels.end() && 
                    it->second.spec.labels["com.massgrid.pubkey"] == pubkey.ToString().substr(0,65)){
                mdndata.mapDockerServiceLists.insert(*it);
                LogPrint("docker","CDockerServerman::ProcessMessage GETDNDATA found service id %s\n",it->first);
            }
        }
        mdndata.version = DOCKERREQUEST_API_VERSION;
        mdndata.sigTime = GetAdjustedTime();
        
        LogPrintf("CDockerServerman::ProcessMessage -- Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);

    }else if(strCommand == NetMsgType::DNDATA){     //cluster
        
        LogPrint("docker","CDockerServerman::ProcessMessage DNDATA Started\n");
        DockerGetData mdndata;
        vRecv >> mdndata;
        if(mdndata.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || mdndata.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --mdndata version %d not support need [%d - %d]\n", mdndata.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            setDNDataStatus(DNDATASTATUS::Received);
            return;
        }
         
        dockercluster.mapDockerServiceLists = mdndata.mapDockerServiceLists;
        dockercluster.sigTime = mdndata.sigTime;
        LogPrint("docker","CDockerServerman::ProcessMessage DNDATA mapDockerServiceLists.size() %d sigTime:%d\n",mdndata.mapDockerServiceLists.size(),mdndata.sigTime);
        std::map<std::string,Service>::iterator iter = mdndata.mapDockerServiceLists.begin();
        for(;iter != mdndata.mapDockerServiceLists.end();iter++){
            if(iter->second.mapDockerTasklists.size()){
                LogPrintf("CDockerServerman::ProcessMessage DNDATA mapDockerTasklists.size()%d serviceid %s\n",iter->second.mapDockerTasklists.size(),iter->first);
            }
        }
        
        setDNDataStatus(DNDATASTATUS::Received);

    }else if(strCommand == NetMsgType::CREATESERVICE){
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        if (!fMasterNode) return;
        DockerCreateService createService;
        vRecv >> createService;
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE createService hash %s\n",createService.ToString());
        if(createService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || createService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --createService version %d not support need [%d - %d]\n", createService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
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
        if(updateService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || updateService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --mdndata version %d not support need [%d - %d]\n", updateService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
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
    }else if(strCommand == NetMsgType::DELETESERVICE){
        LogPrint("docker","CDockerServerman::ProcessMessage DELETESERVICE Started\n");
        if (!fMasterNode) return;
        DockerDeleteService delService;
        vRecv >> delService;
        LogPrint("docker","CDockerServerman::ProcessMessage DELETESERVICE delService hash %s\n",delService.ToString());
        if(delService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || delService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage DELETESERVICE version %d not support need [%d - %d]\n", delService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            return;
        }
        if(delService.sigTime > GetAdjustedTime() + 60 * 5 && delService.sigTime < GetAdjustedTime() - 60 * 5){
            LogPrintf("CDockerServerman::ProcessMessage DELETESERVICE sigTime is out of vaild timespan = %d\n",delService.sigTime);
            return;
        }
        if(!delService.CheckSignature(delService.pubKeyClusterAddress)){
            LogPrintf("CDockerServerman::ProcessMessage DELETESERVICE --CheckSignature Failed pubkey= %s\n",delService.pubKeyClusterAddress.ToString().substr(0,65));
            return;
        }
        bool ret = dockerman.PushMessage(Method::METHOD_SERVICES_DELETE,delService.serviceid,"");
        if(!ret){
            LogPrintf("CDockerServerman::ProcessMessage DELETESERVICE --deleteService Failed serviceid= %s\n",delService.serviceid);
        }else
            LogPrintf("CDockerServerman::ProcessMessage DELETESERVICE --deleteService successful serviceid= %s\n",delService.serviceid);

    }
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

    if(!(createService.memory_byte > 0 && createService.memory_byte <= DOCKER_MAX_MEMORY_BYTE)){
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec memory_byte %d,  DOCKER_MAX_MEMORY_BYTE %d\n",createService.memory_byte,DOCKER_MAX_MEMORY_BYTE);
        return false;
    }
    if(!(createService.cpu > 0 && createService.cpu <= DOCKER_MAX_CPU_NUM)){
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec cpu %d,  DOCKER_MAX_CPU_NUM,%d\n",createService.cpu,DOCKER_MAX_CPU_NUM);
        return false;
    }
    if(!(createService.gpu > 0 && createService.gpu <= DOCKER_MAX_GPU_NUM)){
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec gpu %d,  DOCKER_MAX_GPU_NUM %d\n",createService.gpu,DOCKER_MAX_GPU_NUM);
        return false;
    }
    
    spec.labels["com.massgrid.pubkey"] = createService.pubKeyClusterAddress.ToString().substr(0,65);
    spec.mode.replicated.replicas = 1;
    
    if(!createService.image.empty() && createService.image.substr(0,9) == "massgrid/")
        spec.taskTemplate.containerSpec.image = createService.image;
    else
        spec.taskTemplate.containerSpec.image = "massgrid/10.0-base-ubuntu16.04";
    spec.taskTemplate.containerSpec.user= "root";
    
    spec.taskTemplate.resources.limits.memoryBytes = createService.memory_byte;
    spec.taskTemplate.resources.limits.nanoCPUs = createService.cpu;

    Config::GenericResources otherresources;
    otherresources.discreateResourceSpec.kind = createService.gpuname;
    otherresources.discreateResourceSpec.value = createService.gpu;
    spec.taskTemplate.resources.reservations.genericResources.push_back(otherresources);

    spec.taskTemplate.restartPolicy.condition = "none";
    spec.taskTemplate.containerSpec.command.push_back("start.sh");

    if(createService.n2n_community.empty())
        spec.taskTemplate.containerSpec.env.push_back("N2N_NAME=massgridn2n");
    else
        spec.taskTemplate.containerSpec.env.push_back("N2N_NAME="+createService.n2n_community.substr(0,20));
    
    std::string ipaddr = dockerman.serviceIp.GetFreeIP();
    spec.taskTemplate.containerSpec.env.push_back("N2N_SERVERIP=" + ipaddr);
    spec.taskTemplate.containerSpec.env.push_back("N2N_SNIP=" + dockerman.managerAddr + ":" + boost::lexical_cast<std::string>(dockerman.n2nServerPort));
    spec.taskTemplate.containerSpec.env.push_back("SSH_PUBKEY=" + createService.ssh_pubkey);
    
    spec.taskTemplate.placement.constraints.push_back("node.role == worker");
    Config::Mount mount;
    mount.target="/dev/net";
    mount.source="/dev/net";
    spec.taskTemplate.containerSpec.mounts.push_back(mount);

    if(!dockerman.PushMessage(Method::METHOD_SERVICES_CREATE,"",spec.ToJsonString())){
        dockerman.serviceIp.Erase(ipaddr);
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

#include <algorithm>
#include "dockerserverman.h"
#include "netbase.h"
#include "masternode-sync.h"
#include "dockerman.h"
#include "net.h"
#include "wallet/wallet.h"
#include "dockercluster.h"
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

        
        LogPrint("docker","CDockerServerman::ProcessMessage DNDATA Started\n");
        DockerGetData mdndata;
        vRecv >> mdndata;
        if(mdndata.version < DOCKERREQUEST_API_MINSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --mdndata version %d is too old %d\n", mdndata.version,DOCKERREQUEST_API_MINSUPPORT_VERSION);
            return;
        }
        
        dockercluster.mapDockerServiceLists = mdndata.mapDockerServiceLists;
        dockercluster.sigTime = mdndata.sigTime;

    }else if(strCommand == NetMsgType::CREATESERVICE){
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        if (!fMasterNode) return;
        DockerCreateService sspec;
        vRecv >> sspec;
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE sspec hash %s\n",sspec.ToString());
        if(sspec.version < DOCKERREQUEST_API_MINSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --sspec version %d is too old %d\n", sspec.version,DOCKERREQUEST_API_MINSUPPORT_VERSION);
            return;
        }
        if(CheckAndCreateServerSpec(sspec)){
            DockerGetData mdndata;
                mdndata.mapDockerServiceLists.clear();
            for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
                if(it->second.spec.labels.find("com.massgrid.pubkey") != it->second.spec.labels.end() && 
                    it->second.spec.labels["com.massgrid.pubkey"] == sspec.pubKeyClusterAddress.ToString().substr(0,65)){
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
        DockerCreateService sspec;
        vRecv >> sspec;
        if(sspec.version < DOCKERREQUEST_API_MINSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --sspec version %d is too old %d\n", sspec.version,DOCKERREQUEST_API_MINSUPPORT_VERSION);
            return;
        }
        if(CheckAndCreateServerSpec(sspec)){
            DockerGetData mdndata;
                mdndata.mapDockerServiceLists.clear();
            for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
                if(it->second.spec.labels.find("com.massgrid.pubkey") != it->second.spec.labels.end() && 
                    it->second.spec.labels["com.massgrid.pubkey"] == sspec.pubKeyClusterAddress.ToString().substr(0,65)){
                    mdndata.mapDockerServiceLists.insert(*it);
                }
            }
            mdndata.version = DOCKERREQUEST_API_VERSION;
            mdndata.sigTime = GetAdjustedTime();
            
            LogPrintf("CDockerServerman::ProcessMessage -- CREATESERVICE Sent DNDATA to peer %d\n", pfrom->id);
            connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
        }
    }

}
bool CDockerServerman::CheckAndCreateServerSpec(DockerCreateService Spec){

    LogPrint("docker","CDockerServerman::CheckAndCreateServerSpec Started\n");
    // 1.first check time
    if(Spec.sigTime > GetAdjustedTime() + 60 * 5 && Spec.sigTime < GetAdjustedTime() - 60 * 5){
        LogPrintf("CDockerServerman::CheckAndCreateServerSpec sigTime is invaild\n");
        return false;
    }
    
    //  2.check transactions
    if(Spec.vin != CTxIn()){
        LogPrintf("CDockerServerman::CheckAndCreateServerSpec vin is invaild\n");
        return false;
    }

    //  3. checkSignature
    if(!Spec.CheckSignature(Spec.pubKeyClusterAddress)){
        LogPrintf("CDockerServerman::CheckAndCreateServerSpec -- CheckSignature() failed\n");
        return false;
    }

    //  4. update spec

    return dockerman.PushMessage(Method::METHOD_SERVICES_CREATE,"",Spec.sspec.ToJsonString());

}
bool CDockerServerman::CheckAndUpdateServerSpec(DockerUpdateService Spec){

    LogPrint("docker","CDockerServerman::CheckAndUpdateServerSpec Started\n");
    // 1.first check time
    if(Spec.sigTime > GetAdjustedTime() + 60 * 5 && Spec.sigTime < GetAdjustedTime() - 60 * 5){
        LogPrintf("CDockerServerman::CheckAndUpdateServerSpec sigTime is invaild\n");
        return false;
    }
    
    //  2.check transactions
    if(Spec.vin != CTxIn()){
        LogPrintf("CDockerServerman::CheckAndUpdateServerSpec vin is invaild\n");
        return false;
    }

    //  3. checkSignature
    if(!Spec.CheckSignature(Spec.pubKeyClusterAddress)){
        LogPrintf("CDockerServerman::CheckAndUpdateServerSpec -- CheckSignature() failed\n");
        return false;
    }

    //  4. update spec

    return dockerman.PushMessage(Method::METHOD_SERVICES_CREATE,"",Spec.ToJsonString());

}

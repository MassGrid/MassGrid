#include <algorithm>
#include "dockercluster.h"
#include "netbase.h"
#include "masternode-sync.h"
#include "dockerman.h"
#include "net.h"
#include <boost/lexical_cast.hpp>
#include "messagesigner.h"
CDockerClusterman dockerClusterman;
CNode* CDockerClusterman::ProcessDockernodeConnections(CConnman& connman){

    LogPrint("docker","CDockerClusterman::ProcessDockernodeConnections Started\n");
    CService addr;
    if (!Lookup(selectDockernode.c_str(), addr, 0, false)){
        LogPrintf("CDockerClusterman::ProcessDockernodeConnections Incorrect DockerNode address %s", selectDockernode);
        return nullptr;
    }
    CNode *pnode = connman.ConnectNode(CAddress(addr, NODE_NETWORK), NULL);
    if(!pnode){
        LogPrintf("CDockerClusterman::ProcessDockernodeConnections Couldn't connect to masternode %s", selectDockernode);
        return nullptr;
    }
    return pnode;
}
void CDockerClusterman::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman){
    
    if (!masternodeSync.IsSynced()) return;
    if (strCommand == NetMsgType::GETDNDATA) { //Get Masternode list or specific entry
        // Ignore such requests until we are fully synced.
        // We could start processing this after masternode list is synced
        // but this is a heavy one so it's better to finish sync first.
        
        LogPrint("docker","CDockerClusterman::ProcessMessage GETDNDATA Started\n");
        if (!fMasterNode) return;
        DockerGetData mdndata;
        vRecv >> mdndata;
        if(mdndata.version < this->dndata.version){
            LogPrintf("CDockerClusterman::ProcessMessage --mdndata version is too old %d\n", mdndata.version);
            return;
        }
        LogPrint("docker", "CDockerClusterman::ProcessMessage GETDNDATA -- pubkey =%s\n", mdndata.pubKeyClusterAddress.ToString());
        
        LOCK(cs);
        dockerman.Update();
        mdndata.mapDockerServiceLists.clear();
        for(auto it = dockerman.mapDockerServiceLists.begin();it != dockerman.mapDockerServiceLists.end();++it){
            if(it->second.spec.labels.count("com.massgrid.pubkey")!=0 && 
                it->second.spec.labels["com.massgrid.pubkey"] == this->dndata.pubKeyClusterAddress.ToString()){//search service
                mdndata.mapDockerServiceLists.insert(*it);
            }
        }
        mdndata.version = DNDATAVERSION;
        mdndata.sigTime = GetAdjustedTime();
        
        LogPrintf("CDockerClusterman::ProcessMessage -- Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);

    }else if(strCommand == NetMsgType::DNDATA){

        
        LogPrint("docker","CDockerClusterman::ProcessMessage DNDATA Started\n");
        DockerGetData mdndata;
        vRecv >> mdndata;
        if(mdndata.version < this->dndata.version){
            LogPrintf("CDockerClusterman::ProcessMessage --mdndata version is too old %d\n", mdndata.version);
            return;
        }
        this->dndata.mapDockerServiceLists=mdndata.mapDockerServiceLists;
        this->dndata.sigTime=mdndata.sigTime;

    }else if(strCommand == NetMsgType::CREATESERVICE){
        if (!fMasterNode) return;
    }
        


}
void CDockerClusterman::AskForDNData(CNode* pnode, CConnman& connman)
{
    LogPrint("docker","CDockerClusterman::AskForDNData Started\n");
    if(!pnode) return;

    LOCK(cs);
    LogPrintf("AskForDNData pukey %s\n",dndata.pubKeyClusterAddress.ToString().substr(0,65));
    connman.PushMessage(pnode, NetMsgType::GETDNDATA, dndata);

    LogPrint("docker","CDockerClusterman::AskForDNData Pushed\n");
}

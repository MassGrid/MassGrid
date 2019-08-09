#include <algorithm>
#include "dockercluster.h"
#include "netbase.h"
#include "masternode-sync.h"
#include "net.h"
#include <boost/lexical_cast.hpp>
#include "messagesigner.h"
#include "validation.h"
#include "init.h"
#include "wallet/wallet.h"
#include "util.h"
#include "pubkey.h"
#include "dockerserverman.h"

Cluster dockercluster;

extern CDockerServerman dockerServerman;

bool Cluster::SetConnectDockerAddress(std::string address_port){
    setDefaultPubkey(GetDefaultPubkey()); 
    dockerServerman.setDNDataStatus(CDockerServerman::Free);  

    LogPrint("dockernode","Cluster::SetConnectDockerAddress Started\n");
    if (!Lookup(address_port.c_str(), connectDockerAddr, 0, false)){
        LogPrintf("Cluster::SetConnectDockerAddress Incorrect DockerNode address %s", address_port);
        return false;
    }
    return true;
}

bool Cluster::ProcessDockernodeConnections(){

    LogPrint("dockernode","Cluster::ProcessDockernodeConnections Started\n");
    
    connectNode = g_connman->ConnectNode(CAddress(connectDockerAddr, NODE_NETWORK), NULL);
    
    if(!connectNode){
        LogPrintf("Cluster::ProcessDockernodeConnections Couldn't connect to dockernode");
        return false;
    }
    return true;
}

bool Cluster::ProcessDockernodeDisconnections(const std::string& strNode)
{
    LogPrint("dockernode","Cluster::ProcessDockernodeDisconnections Started\n");

    if(!g_connman->DisconnectNode(strNode)){
        LogPrintf("Cluster::ProcessDockernodeDisconnections Couldn't disconnect dockernode");
        return false;
    }
    return true;
}
bool Cluster::SetConnectDockerAddr(std::string address_port)
{

    LogPrint("dockernode","Cluster::SetConnectDockerAddress Started\n");
    if (!Lookup(address_port.c_str(), connectDockerAddr, 0, false)){
        LogPrintf("Cluster::SetConnectDockerAddress Incorrect DockerNode address %s", address_port);
        return false;
    }
    return true;
}
void Cluster::AskForDNData()
{
    LogPrint("dockernode","Cluster::AskForDNData Started\n");
    if(!connectNode) return;

    LOCK(cs);
    LogPrintf("AskForDNData pukey %s\n",DefaultPubkey.ToString().substr(0,66));

    dockerServerman.setDNDataStatus(CDockerServerman::Ask);
    g_connman->PushMessage(connectNode, NetMsgType::GETDNDATA, DefaultPubkey);
}

void Cluster::AskForService(COutPoint outpoint)
{
    LogPrint("dockernode","Cluster::AskForService Started\n");
    if(!connectNode) return;

    LOCK(cs);
    LogPrintf("AskForService pukey %s\n",DefaultPubkey.ToString().substr(0,66));

    DockerGetService getService{};
    getService.OutPoint = outpoint;
    getService.pubKeyClusterAddress = DefaultPubkey;
    getService.sigTime = GetAdjustedTime();
    dockerServerman.setDNDataStatus(CDockerServerman::Ask);
    g_connman->PushMessage(connectNode, NetMsgType::GETSERVICE, getService);
}
void Cluster::AskForServices()
{
    LogPrint("dockernode","Cluster::AskForServices Started\n");
    if(!connectNode) return;

    LOCK(cs);
    LogPrintf("AskForServices pukey %s\n",DefaultPubkey.ToString().substr(0,66));

    DockerGetService getService{};
    getService.pubKeyClusterAddress = DefaultPubkey;
    getService.sigTime = GetAdjustedTime();
    dockerServerman.setDNDataStatus(CDockerServerman::Ask);
    g_connman->PushMessage(connectNode, NetMsgType::GETSERVICES, getService);
}
bool Cluster::CreateAndSendSeriveSpec(DockerCreateService sspec){

    LogPrint("dockernode","Cluster::CreateAndSendSeriveSpec Started\n");

    LOCK(cs);
    if(!connectNode){
        LogPrintf("Cluster::CreateAndSendSeriveSpec cann't connect dockernode \n");
        return false;
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CKey vchSecret;
    if(pwalletMain->IsLocked()){
        LogPrintf("Cluster::CreateAndSendSeriveSpec Sign need to unlock wallet first !\n");
        return false;
    }
    if (!pwalletMain->GetKey(sspec.clusterServiceCreate.pubKeyClusterAddress.GetID(), vchSecret)){
        LogPrintf("Cluster::CreateAndSendSeriveSpec GetPrivkey Error\n");
        return false;
    }
    if(!sspec.Sign(vchSecret,sspec.clusterServiceCreate.pubKeyClusterAddress)){
        LogPrintf("Cluster::CreateAndSendSeriveSpec Sign Error\n");
        return false;
    }
    
    dockerServerman.setDNDataStatus(CDockerServerman::Creating);

    g_connman->PushMessage(connectNode, NetMsgType::CREATESERVICE, sspec);
    return true;
}
bool Cluster::UpdateAndSendSeriveSpec(DockerUpdateService sspec){

    LogPrint("dockernode","Cluster::UpdateAndSendSeriveSpec Started\n");

    LOCK(cs);
    if(!connectNode){
        LogPrintf("Cluster::UpdateAndSendSeriveSpec cann't connect dockernode \n");
        return false;
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CKey vchSecret;
    if(pwalletMain->IsLocked()){
        LogPrintf("Cluster::UpdateAndSendSeriveSpec Sign need to unlock wallet first !\n");
        return false;
    }
    if (!pwalletMain->GetKey(sspec.clusterServiceUpdate.pubKeyClusterAddress.GetID(), vchSecret)){
        LogPrintf("Cluster::UpdateAndSendSeriveSpec GetPrivkey Error\n");
        return false;
    }
    if(!sspec.Sign(vchSecret,sspec.clusterServiceUpdate.pubKeyClusterAddress)){
        LogPrintf("Cluster::UpdateAndSendSeriveSpec Sign Error\n");
        return false;
    }
    
    dockerServerman.setDNDataStatus(CDockerServerman::Updating);

    g_connman->PushMessage(connectNode, NetMsgType::UPDATESERVICE, sspec);
    return true;
}
bool Cluster::DeleteAndSendServiceSpec(DockerDeleteService delService)
{
    if (!masternodeSync.IsSynced()){
        LogPrintf("Need to Synced First\n");
        return false;
    }
    CKey vchSecret;
    if (!pwalletMain->GetKey(delService.pubKeyClusterAddress.GetID(), vchSecret)){
        LogPrintf("delService Sign Error1\n");
        return false;
    }
    if(!delService.Sign(vchSecret,delService.pubKeyClusterAddress)){
        LogPrintf("delService Sign Error2\n");
        return false;
    }
    g_connman->PushMessage(dockercluster.connectNode, NetMsgType::DELETESERVICE, delService);
    return true;
}

void Cluster::setDefaultPubkey(CPubKey pubkey)
{
    DefaultPubkey = pubkey;
}

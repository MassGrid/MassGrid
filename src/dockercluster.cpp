#include <algorithm>
#include "dockercluster.h"
#include "netbase.h"
#include "masternode-sync.h"
#include "dockerman.h"
#include "net.h"
#include <boost/lexical_cast.hpp>
#include "messagesigner.h"
#include "validation.h"
#include "init.h"
#include "wallet/wallet.h"
#include "util.h"
#include "pubkey.h"
Cluster dockercluster;

bool Cluster::SetConnectDockerAddress(std::string address_port){
    setDefaultPubkey(GetDefaultPubkey());   

    LogPrint("docker","Cluster::SetConnectDockerAddress Started\n");
    if (!Lookup(address_port.c_str(), connectDockerAddr, 0, false)){
        LogPrintf("Cluster::SetConnectDockerAddress Incorrect DockerNode address %s", address_port);
        return false;
    }
    return true;
}

bool Cluster::ProcessDockernodeConnections(){

    LogPrint("docker","Cluster::ProcessDockernodeConnections Started\n");
    
    connectNode = g_connman->ConnectNode(CAddress(connectDockerAddr, NODE_NETWORK), NULL);
    
    if(!connectNode){
        LogPrintf("Cluster::ProcessDockernodeConnections Couldn't connect to dockernode");
        return false;
    }
    return true;
}

void Cluster::AskForDNData()
{
    LogPrint("docker","Cluster::AskForDNData Started\n");
    if(!connectNode) return;

    LOCK(cs);
    LogPrintf("AskForDNData pukey %s\n",DefaultPubkey.ToString().substr(0,65));
    g_connman->PushMessage(connectNode, NetMsgType::GETDNDATA, DefaultPubkey);
}

bool Cluster::CreateAndSendSeriveSpec(DockerCreateService sspec){

    LogPrint("docker","Cluster::CreateAndSendSeriveSpec Started\n");

    LOCK(cs);
    if(!connectNode){
        LogPrintf("Cluster::CreateDoCreateAndSendSeriveSpecckerService cann't connect dockernode \n");
        return false;
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CKey vchSecret;
    if(pwalletMain->IsLocked()){
        LogPrintf("Cluster::CreateAndSendSeriveSpec Sign need to unlock wallet first !\n");
        return false;
    }
    if (!pwalletMain->GetKey(DefaultPubkey.GetID(), vchSecret)){
        LogPrintf("Cluster::CreateAndSendSeriveSpec GetPrivkey Error\n");
        return false;
    }
    if(!sspec.Sign(vchSecret,DefaultPubkey)){
        LogPrintf("Cluster::CreateAndSendSeriveSpec Sign Error\n");
        return false;
    }
    
    g_connman->PushMessage(connectNode, NetMsgType::CREATESERVICE, sspec);
    return true;
}

bool Cluster::UpdateAndSendSeriveSpec(DockerUpdateService sspec){
    
    LogPrint("docker","Cluster::UpdateAndSendSeriveSpec Started\n");

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
    if (!pwalletMain->GetKey(DefaultPubkey.GetID(), vchSecret)){
        LogPrintf("Cluster::UpdateAndSendSeriveSpec GetPrivkey Error\n");
        return false;
    }
    if(!sspec.Sign(vchSecret,DefaultPubkey)){
        LogPrintf("Cluster::UpdateAndSendSeriveSpec Sign Error\n");
        return false;
    }
    
    g_connman->PushMessage(connectNode, NetMsgType::UPDATESERVICE, sspec);
    return true;
}

void Cluster::setDefaultPubkey(CPubKey pubkey)
{
    DefaultPubkey = pubkey;
}
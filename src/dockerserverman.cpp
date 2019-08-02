#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "init.h"
#include "net.h"
#include "wallet/wallet.h"
#include "dockercluster.h"
#include "dockerserverman.h"
#include "netbase.h"
#include "masternode-sync.h"
#include "coincontrol.h"
#include "validation.h"
#include "messagesigner.h"
#include "instantx.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "dockersupernode.h"
#include "prizes_client.h"
#include "prizes_service.h"
CDockerServerman dockerServerman;
const char* strServiceCode[] = {
    "Successful",
    "SigTime Error",
    "Version not support",
    "CheckSignature Failed",
    "Invalid or non-wallet transaction",
    "The transaction not confirms",
    "Transaction has been create",
    "Serviceitem not found",
    "Serviceitem no resource",
    "Payment not enough",
    "GPU amount error",
    "CPU thread amount error",
    "Memory Size error",
    "Transaction has been tlemented",
    "PubKey check failed",
    "OutPoint not found",
    "request server not dockernode",
    "request error"
};

void CDockerServerman::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman){
    
    if (!masternodeSync.IsSynced()) return;
    
    if (strCommand == NetMsgType::GETDNDATA) { //server

        LogPrint("dockernode", "CDockerServerman::ProcessMessage GETDNDATA Started\n");
        ResponseMachines mdndata;
        if (!fDockerNode) {
            mdndata.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, mdndata);
            return;
        }
        CPubKey pubkey;
        vRecv >> pubkey;
        prizesClient.GetMachines(mdndata);
        mdndata.masternodeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString();
        LogPrint("dockernode", "CDockerServerman::ProcessMessage GETDNDATA -- pubkey %s\n", pubkey.ToString().substr(0,66));
        LogPrintf("CDockerServerman::ProcessMessage -- Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
        
    }else if(strCommand == NetMsgType::DNDATA){     //cluster

        LogPrint("dockernode","CDockerServerman::ProcessMessage DNDATA Started\n");
        ResponseMachines mdndata;
        vRecv >> mdndata;
        if(mdndata.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || mdndata.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", mdndata.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            setDNDataStatus(DNDATASTATUS::Received);
            return;
        }
        dockercluster.machines = mdndata;
        setDNDataStatus(DNDATASTATUS::Received);

    }else if (strCommand == NetMsgType::GETSERVICE) { //docker gettransaction

        LogPrint("dockernode", "CDockerServerman::ProcessMessage GETSERVICE Started\n");
        DockerServiceInfo dockerServiceInfo{};
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        DockerGetService getService{};
        vRecv >> getService;
        if (getService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || getService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", getService.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            dockerServiceInfo.err = "version nout support current version: " + std::to_string(getService.version) + "require: [" + std::to_string(DOCKERREQUEST_API_MINSUPPORT_VERSION) + "," + std::to_string(DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        if(getService.OutPoint.IsNull()){
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::OUTPOINT_NOT_FOUND];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        CWalletTx& wtx = pwalletMain->mapWallet[getService.OutPoint.hash];
        std::string ServiceID = wtx.Getserviceid();
        if(ServiceID.empty()){
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NOT_FOUND];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        ServiceInfo serviceInfo{};
        if(!prizesClient.GetService(ServiceID,serviceInfo)){
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::REQUEST_ERROR] + ServiceID;
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        dockerServiceInfo.servicesInfo[serviceInfo.ServiceID] = serviceInfo;
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);

    }else if (strCommand == NetMsgType::GETSERVICES) { //docker gettransaction

        LogPrint("dockernode", "CDockerServerman::ProcessMessage GETSERVICES Started\n");
        DockerServiceInfo dockerServiceInfo{};
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        DockerGetService getService{};
        vRecv >> getService;
        if (getService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || getService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", getService.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            dockerServiceInfo.err = "version nout support current version: " + std::to_string(getService.version) + "require: [" + std::to_string(DOCKERREQUEST_API_MINSUPPORT_VERSION) + "," + std::to_string(DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        std::vector<ServiceInfo> vecServiceInfo{};
        if (!prizesClient.GetServiceFromPubkey(getService.pubKeyClusterAddress.ToString().substr(0, 66), vecServiceInfo)) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::REQUEST_ERROR];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        for (auto& service : vecServiceInfo) {
            dockerServiceInfo.servicesInfo[service.ServiceID] = service;
        }
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);

    } else if (strCommand == NetMsgType::SERVICEDATA) { //client

        LogPrint("dockernode","CDockerServerman::ProcessMessage SERVICEDATA Started\n");
        DockerServiceInfo dockerServiceInfo;
        
        vRecv >> dockerServiceInfo;
        if (dockerServiceInfo.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || dockerServiceInfo.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", dockerServiceInfo.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            return;
        }
        for (auto& item : dockerServiceInfo.servicesInfo) {
            dockercluster.vecServiceInfo.servicesInfo[item.first] = item.second;
        }
        setTRANSDataStatus(TRANSDATASTATUS::ReceivedTD);
    }else if(strCommand == NetMsgType::CREATESERVICE){
        LogPrint("dockernode", "CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        DockerServiceInfo dockerServiceInfo;
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        DockerCreateService dockerCreateService;
        vRecv >> dockerCreateService;
        if (dockerCreateService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || dockerCreateService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", dockerCreateService.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            dockerServiceInfo.err = "version nout support current version: " + std::to_string(dockerCreateService.version) + "require: [" + std::to_string(DOCKERREQUEST_API_MINSUPPORT_VERSION) + "," + std::to_string(DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }

        if (!dockerCreateService.CheckSignature(dockerCreateService.clusterServiceCreate.pubKeyClusterAddress)) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::PUBKEY_ERROR];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }

        if (!CheckTransaction(dockerCreateService.clusterServiceCreate.OutPoint, dockerCreateService.clusterServiceCreate.pubKeyClusterAddress, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        ServiceCreate serviceCreate{};
        if (!CheckCreateService(dockerCreateService.clusterServiceCreate, serviceCreate, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        std::string ServiceID{};
        if (!prizesClient.PostServiceCreate(serviceCreate, ServiceID, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        ServiceInfo serviceInfo{};
        if (!prizesClient.GetService(ServiceID, serviceInfo)) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::REQUEST_ERROR];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        dockerServiceInfo.servicesInfo[serviceInfo.ServiceID] = serviceInfo;
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
    } else if (strCommand == NetMsgType::UPDATESERVICE) {
        LogPrint("dockernode", "CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        DockerServiceInfo dockerServiceInfo;
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        DockerUpdateService dockerUpdateService;
        vRecv >> dockerUpdateService;
        if (dockerUpdateService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || dockerUpdateService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", dockerUpdateService.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            dockerServiceInfo.err = "version nout support current version: " + std::to_string(dockerUpdateService.version) + "require: [" + std::to_string(DOCKERREQUEST_API_MINSUPPORT_VERSION) + "," + std::to_string(DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }

        if (!dockerUpdateService.CheckSignature(dockerUpdateService.clusterServiceUpdate.pubKeyClusterAddress)) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::PUBKEY_ERROR];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }

        if (!CheckTransaction(dockerUpdateService.clusterServiceUpdate.OutPoint, dockerUpdateService.clusterServiceUpdate.pubKeyClusterAddress, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        ServiceUpdate serviceUpdate{};
        if (!CheckUpdateService(dockerUpdateService.clusterServiceUpdate, serviceUpdate, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        if (!prizesClient.PostServiceUpdate(serviceUpdate, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        ServiceInfo serviceInfo{};
        if (!prizesClient.GetService(serviceUpdate.ServiceID, serviceInfo)) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::REQUEST_ERROR];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        dockerServiceInfo.servicesInfo[serviceInfo.ServiceID] = serviceInfo;
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
    }else if(strCommand == NetMsgType::DELETESERVICE){
        LogPrint("dockernode", "CDockerServerman::ProcessMessage DELETESERVICE Started\n");
        DockerServiceInfo dockerServiceInfo;
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        
    }
}
bool CDockerServerman::CheckTransaction(COutPoint outpoint, CPubKey pubKeyClusterAddress, std::string& err)
{
    //check transactions
    if (!pwalletMain->mapWallet.count(outpoint.hash)) {
        err = strServiceCode[SERVICEMANCODE::NO_THRANSACTION];
        LogPrintf("CDockerServerman::CheckTransaction Invalid or non-wallet transaction: %s\n", outpoint.hash.ToString());
        return false;
    }
    CWalletTx& wtx = pwalletMain->mapWallet[outpoint.hash]; //watch only not check

    //check transaction vin pubkey
    if (!CheckScriptPubkeyInTxVin(GetScriptForDestination(pubKeyClusterAddress.GetID()), wtx)) {
        err = strServiceCode[SERVICEMANCODE::NO_THRANSACTION];
        LogPrintf("CDockerServerman::CheckTransaction %s\n", err);
        return false;
    }

    if (wtx.vout[outpoint.n].scriptPubKey != GetScriptForDestination(pwalletMain->vchDefaultKey.GetID())) {
        err = strServiceCode[SERVICEMANCODE::OUTPOINT_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec outpoint not found\n");
        return false;
    }

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx.GetHash());
    int confirms = wtx.GetDepthInMainChain(false);
    LogPrint("dockernode", "current transaction fLocked %d confirms %d\n", fLocked, confirms);
    if (!fLocked && confirms < 1) {
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_NOT_CONFIRMS];
        LogPrintf("CDockerServerman::CheckTransaction The transaction not confirms: %d\n", confirms);
        return false;
    }

    if (wtx.HasCreatedService()) {
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_DOUBLE_CREATE];
        LogPrintf("CDockerServerman::CheckTransaction current transaction has been used\n");
        return false;
    }
    return true;
}
bool CDockerServerman::CheckCreateService(ClusterServiceCreate_t& clusterServiceCreate, ServiceCreate& serviceCreate, std::string& err)
{
    if (clusterServiceCreate.ServiceName.size() <= 20) {
        serviceCreate.ServiceName = clusterServiceCreate.ServiceName;
    }
    if (!serviceCreate.Image.empty() && serviceCreate.Image.substr(0, 9) == "massgrid/")
        serviceCreate.Image = clusterServiceCreate.Image;
    serviceCreate.SSHPubkey = clusterServiceCreate.SSHPubkey;
    serviceCreate.pubKeyClusterAddress = clusterServiceCreate.pubKeyClusterAddress;
    serviceCreate.OutPoint = clusterServiceCreate.OutPoint;
    serviceCreate.ENV = clusterServiceCreate.ENV;
    ResponseMachines machines{};
    if (!prizesClient.GetMachines(machines)) {
        err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckTransaction no found\n");
        return false;
    }
    Item item(clusterServiceCreate.hardware.CPUType, clusterServiceCreate.hardware.CPUThread, clusterServiceCreate.hardware.MemoryType, clusterServiceCreate.hardware.MemoryCount, clusterServiceCreate.hardware.GPUType, clusterServiceCreate.hardware.GPUCount);
    if (!machines.items.count(item)) {
        err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NO_RESOURCE];
        LogPrintf("CDockerServerman::CheckTransaction no resource\n");
        return false;
    }
    if (!machines.items[item].count) {
        err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NO_RESOURCE];
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- serviceitem no resource\n");
        return false;
    }

    //  5. check item
    serviceCreate.hardware = clusterServiceCreate.hardware;
    CWalletTx& wtx = pwalletMain->mapWallet[serviceCreate.OutPoint.hash]; //watch only not check
    serviceCreate.Amount = wtx.vout[serviceCreate.OutPoint.n].nValue;
    serviceCreate.ServicePrice = machines.items[item].price;
    serviceCreate.MasterNodeN2NAddr = dockerServerman.LocalIP + ":" + boost::lexical_cast<std::string>(GetSNPort());
    serviceCreate.Drawee = CMassGridAddress(serviceCreate.pubKeyClusterAddress.GetID()).ToString();
    serviceCreate.MasterNodeFeeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString();

    CPubKey devpubkey(ParseHex(Params().SporkPubKey()));
    serviceCreate.DevFeeAddress = CMassGridAddress(devpubkey.GetID()).ToString();
    serviceCreate.MasterNodeFeeRate = dockerServerman.feeRate;
    serviceCreate.DevFeeRate = sporkManager.GetDeveloperPayment();
    return true;
}
bool CDockerServerman::CheckUpdateService(ClusterServiceUpdate_t& clusterServiceUpdate, ServiceUpdate& serviceUpdate,std::string& err)
{
    CWalletTx& wtx = pwalletMain->mapWallet[clusterServiceUpdate.CrerateOutPoint.hash];
    if (wtx.Getserviceid().empty()) {
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_DOUBLE_CREATE];
        LogPrintf("CDockerServerman::CheckUpdateService not found serviceID\n");
        return false;
    }
    serviceUpdate.ServiceID = wtx.Getserviceid();
    serviceUpdate.CrerateOutPoint = clusterServiceUpdate.CrerateOutPoint;
    serviceUpdate.pubKeyClusterAddress = clusterServiceUpdate.pubKeyClusterAddress;
    serviceUpdate.OutPoint = clusterServiceUpdate.OutPoint;
    serviceUpdate.Amount = wtx.vout[serviceUpdate.OutPoint.n].nValue;
    ResponseMachines machines{};
    if (!prizesClient.GetMachines(machines)) {
        err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckTransaction no found\n");
        return false;
    }
    Item item(wtx.GetCPUType(), boost::lexical_cast<int64_t>(wtx.GetCPUThread()), wtx.GetMemoryType(), boost::lexical_cast<int64_t>(wtx.GetMemoryCount()), wtx.GetGPUType(), boost::lexical_cast<int64_t>(wtx.GetGPUCount()));

    serviceUpdate.ServicePrice = machines.items[item].price;
    serviceUpdate.Drawee = CMassGridAddress(serviceUpdate.pubKeyClusterAddress.GetID()).ToString();
    serviceUpdate.MasterNodeFeeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString();
    CPubKey devpubkey(ParseHex(Params().SporkPubKey()));
    serviceUpdate.DevFeeAddress = CMassGridAddress(devpubkey.GetID()).ToString();
    serviceUpdate.MasterNodeFeeRate = dockerServerman.feeRate;
    serviceUpdate.DevFeeRate = sporkManager.GetDeveloperPayment();
    return true;
}
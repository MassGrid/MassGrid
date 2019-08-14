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
    "request error",
    "pubkey not found",
    "service not found"
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
        if(!prizesClient.GetMachines(mdndata,mdndata.err)){
            connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
            LogPrintf("CDockerServerman::ProcessMessage GETDNDATA error %s\n",mdndata.err);
            return;
        }
        mdndata.masternodeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString();
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
        
    }else if(strCommand == NetMsgType::DNDATA){     //cluster

        LogPrint("dockernode","CDockerServerman::ProcessMessage DNDATA Started\n");
        ResponseMachines mdndata;
        vRecv >> mdndata;
        if(mdndata.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || mdndata.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", mdndata.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            dockercluster.machines.err = mdndata.err;
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

        if(getService.sigTime > GetAdjustedTime() + TIMEOUT && getService.sigTime < GetAdjustedTime() - TIMEOUT){
             dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::SIGTIME_ERROR];
            LogPrintf("CDockerServerman::CheckAndCreateServiveSpec sigTime Error%s\n");
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
        if(!prizesClient.GetService(ServiceID,serviceInfo,dockerServiceInfo.err)){
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        dockerServiceInfo.servicesInfo[serviceInfo.ServiceID] = serviceInfo;
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);

    }else if (strCommand == NetMsgType::GETSERVICES) { //docker gettransaction

        LogPrint("dockernode", "CDockerServerman::ProcessMessage GETSERVICES Started\n");
        DockerServiceInfo dockerServiceInfo{};
        DockerGetService getService{};
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        vRecv >> getService;
        if (getService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || getService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", getService.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            dockerServiceInfo.err = "version nout support current version: " + std::to_string(getService.version) + "require: [" + std::to_string(DOCKERREQUEST_API_MINSUPPORT_VERSION) + "," + std::to_string(DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        if(getService.sigTime > GetAdjustedTime() + TIMEOUT && getService.sigTime < GetAdjustedTime() - TIMEOUT){
             dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::SIGTIME_ERROR];
            LogPrintf("CDockerServerman::CheckAndCreateServiveSpec sigTime Error%s\n");
            return;
        }
        
        std::vector<ServiceInfo> vecServiceInfo{};
        if (!prizesClient.GetServiceFromPubkey(getService.pubKeyClusterAddress.ToString().substr(0, 66), vecServiceInfo,dockerServiceInfo.err)) {
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
            setDNDataStatus(DNDATASTATUS::Received);
            return;
        }
        for(int i=0;i<dockerServiceInfo.servicesInfo.size();++i){
            LogPrintf("CDockerServerman::ProcessMessage %d\n",i);
        }
        if (dockerServiceInfo.servicesInfo.size() > 1){
            dockercluster.vecServiceInfo.servicesInfo.clear();
        }
        for (auto& item : dockerServiceInfo.servicesInfo) {
            dockercluster.vecServiceInfo.servicesInfo[item.first] = item.second;
        }
        dockercluster.vecServiceInfo.err = dockerServiceInfo.err;
        setDNDataStatus(DNDATASTATUS::Received);
    }else if(strCommand == NetMsgType::CREATESERVICE){
        LogPrint("dockernode", "CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        DockerServiceInfo dockerServiceInfo{};
        DockerCreateService dockerCreateService{};
        ServiceCreate serviceCreate{};
        std::string ServiceID{};
        ServiceInfo serviceInfo{};
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        vRecv >> dockerCreateService;
        if (!CheckCreateService(dockerCreateService, serviceCreate, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        if (!prizesClient.PostServiceCreate(serviceCreate, ServiceID, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }

        CWalletTx& wtx = pwalletMain->mapWallet[dockerCreateService.clusterServiceCreate.OutPoint.hash];
        wtx.Setserviceid(ServiceID);
        wtx.Setpubkey(dockerCreateService.clusterServiceCreate.pubKeyClusterAddress.ToString().substr(0, 66));
        wtx.SetCPUType(dockerCreateService.clusterServiceCreate.hardware.CPUType);
        wtx.SetCPUThread(std::to_string(dockerCreateService.clusterServiceCreate.hardware.CPUThread));
        wtx.SetMemoryType(dockerCreateService.clusterServiceCreate.hardware.MemoryType);
        wtx.SetMemoryCount(std::to_string(dockerCreateService.clusterServiceCreate.hardware.MemoryCount));
        wtx.SetGPUType(dockerCreateService.clusterServiceCreate.hardware.GPUType);
        wtx.SetGPUCount(std::to_string(dockerCreateService.clusterServiceCreate.hardware.GPUCount));
        CWalletDB walletdb(pwalletMain->strWalletFile);
        wtx.WriteToDisk(&walletdb);
        
        if (!prizesClient.GetService(ServiceID, serviceInfo,dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        dockerServiceInfo.servicesInfo[serviceInfo.ServiceID] = serviceInfo;
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);

    } else if (strCommand == NetMsgType::UPDATESERVICE) {
        LogPrint("dockernode", "CDockerServerman::ProcessMessage UPDATESERVICE Started\n");
        DockerServiceInfo dockerServiceInfo{};
        DockerUpdateService dockerUpdateService{};
        ServiceInfo serviceInfo{};
        ServiceUpdate serviceUpdate{};
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        vRecv >> dockerUpdateService;
        if (!CheckUpdateService(dockerUpdateService, serviceUpdate, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        if (!prizesClient.PostServiceUpdate(serviceUpdate, dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }

        CWalletTx& wtx = pwalletMain->mapWallet[dockerUpdateService.clusterServiceUpdate.OutPoint.hash];
        wtx.Setserviceid(serviceUpdate.ServiceID);
        wtx.SetCreateOutPoint(serviceUpdate.CrerateOutPoint.ToStringShort());
        wtx.Setpubkey(dockerUpdateService.clusterServiceUpdate.pubKeyClusterAddress.ToString().substr(0, 66));
        CWalletDB walletdb(pwalletMain->strWalletFile);
        wtx.WriteToDisk(&walletdb);

        if (!prizesClient.GetService(serviceUpdate.ServiceID, serviceInfo,dockerServiceInfo.err)) {
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        dockerServiceInfo.servicesInfo[serviceInfo.ServiceID] = serviceInfo;
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);

    }else if(strCommand == NetMsgType::DELETESERVICE){
        LogPrint("dockernode", "CDockerServerman::ProcessMessage DELETESERVICE Started\n");
        DockerServiceInfo dockerServiceInfo;
        DockerDeleteService dockerDeleteService;
        if (!fDockerNode) {
            dockerServiceInfo.err = strServiceCode[SERVICEMANCODE::NOT_DOCKERNODE];
            connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
            return;
        }
        vRecv >> dockerDeleteService;
        CheckDeleteService(dockerDeleteService,dockerServiceInfo.msg,dockerServiceInfo.err);
        connman.PushMessage(pfrom, NetMsgType::SERVICEDATA, dockerServiceInfo);
        return;
    }
}
bool CDockerServerman::CheckCreateService(DockerCreateService& dockerCreateService, ServiceCreate& serviceCreate, std::string& err)
{
    //1. CheckSignature
    if (dockerCreateService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || dockerCreateService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
        LogPrintf("CDockerServerman::CheckCreateService --current version %d not support [%d - %d]\n", dockerCreateService.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
        err = "version nout support current version: " + std::to_string(dockerCreateService.version) + "require: [" + std::to_string(DOCKERREQUEST_API_MINSUPPORT_VERSION) + "," + std::to_string(DOCKERREQUEST_API_MAXSUPPORT_VERSION);
        return false;
    }
    if(dockerCreateService.sigTime > GetAdjustedTime() + TIMEOUT && dockerCreateService.sigTime < GetAdjustedTime() - TIMEOUT){
        err = strServiceCode[SERVICEMANCODE::SIGTIME_ERROR];
        LogPrintf("CDockerServerman::CheckCreateService sigTime Error%s\n");
        return false;
    }
    if (!dockerCreateService.CheckSignature(dockerCreateService.clusterServiceCreate.pubKeyClusterAddress)) {
        err = strServiceCode[SERVICEMANCODE::PUBKEY_ERROR];
        return false;
    }


    //2.check transactions
    const COutPoint& outpoint = dockerCreateService.clusterServiceCreate.OutPoint;
    if (!pwalletMain->mapWallet.count(outpoint.hash)) {
        err = strServiceCode[SERVICEMANCODE::NO_THRANSACTION];
        LogPrintf("CDockerServerman::CheckCreateService %s %s\n",err, outpoint.hash.ToString());
        return false;
    }
    CWalletTx& wtx = pwalletMain->mapWallet[outpoint.hash]; //watch only not check

    if (wtx.HasCreatedService()) {
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_DOUBLE_CREATE];
        LogPrintf("CDockerServerman::CheckCreateService current transaction has been used\n");
        return false;
    }

    if (!CheckScriptPubkeyInTxVin(GetScriptForDestination(dockerCreateService.clusterServiceCreate.pubKeyClusterAddress.GetID()), wtx)) {
        err = strServiceCode[SERVICEMANCODE::NO_THRANSACTION];
        LogPrintf("CDockerServerman::CheckCreateService %s\n", err);
        return false;
    }

    if (wtx.vout[outpoint.n].scriptPubKey != GetScriptForDestination(pwalletMain->vchDefaultKey.GetID())) {
        err = strServiceCode[SERVICEMANCODE::OUTPOINT_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckCreateService outpoint not found\n");
        return false;
    }

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx.GetHash());
    int confirms = wtx.GetDepthInMainChain(false);
    LogPrint("docker", "current transaction fLocked %d confirms %d\n", fLocked, confirms);
    if (!fLocked && confirms < 1) {
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_NOT_CONFIRMS];
        LogPrintf("CDockerServerman::CheckCreateService The transaction not confirms: %d\n", confirms);
        return false;
    }

    if (dockerCreateService.clusterServiceCreate.ServiceName.size() <= 20) {
        serviceCreate.ServiceName = dockerCreateService.clusterServiceCreate.ServiceName;
    }
    if (!dockerCreateService.clusterServiceCreate.Image.empty() && dockerCreateService.clusterServiceCreate.Image.substr(0, 9) == "massgrid/")
        serviceCreate.Image = dockerCreateService.clusterServiceCreate.Image;
    serviceCreate.SSHPubkey = dockerCreateService.clusterServiceCreate.SSHPubkey;
    serviceCreate.pubKeyClusterAddress = dockerCreateService.clusterServiceCreate.pubKeyClusterAddress;
    serviceCreate.OutPoint = dockerCreateService.clusterServiceCreate.OutPoint;
    serviceCreate.ENV = dockerCreateService.clusterServiceCreate.ENV;
    ResponseMachines machines{};
    std::string tmp;
    if (!prizesClient.GetMachines(machines,tmp)) {
        err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckCreateService no found\n");
        return false;
    }
    Item item(dockerCreateService.clusterServiceCreate.hardware.CPUType,
                dockerCreateService.clusterServiceCreate.hardware.CPUThread,
                dockerCreateService.clusterServiceCreate.hardware.MemoryType,
                dockerCreateService.clusterServiceCreate.hardware.MemoryCount,
                dockerCreateService.clusterServiceCreate.hardware.GPUType,
                dockerCreateService.clusterServiceCreate.hardware.GPUCount
            );
    if (!machines.items.count(item)) {
        err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckCreateService no resource\n");
        return false;
    }
    if (!machines.items[item].count) {
        err = strServiceCode[SERVICEMANCODE::SERVICEITEM_NO_RESOURCE];
        LogPrintf("CDockerServerman::CheckCreateService -- serviceitem no resource\n");
        return false;
    }

    //  5. check item
    serviceCreate.hardware = dockerCreateService.clusterServiceCreate.hardware;
    serviceCreate.Amount = wtx.vout[outpoint.n].nValue;
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
bool CDockerServerman::CheckUpdateService(DockerUpdateService& dockerUpdateService, ServiceUpdate& serviceUpdate,std::string& err)
{
    CWalletTx& wtx = pwalletMain->mapWallet[dockerUpdateService.clusterServiceUpdate.CrerateOutPoint.hash];
    CWalletTx& wtx2 = pwalletMain->mapWallet[dockerUpdateService.clusterServiceUpdate.OutPoint.hash];
    if (wtx.Getserviceid().empty()) {
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_DOUBLE_CREATE];
        LogPrintf("CDockerServerman::CheckUpdateService not found\n");
        return false;
    }
    if (!wtx.GetCreateOutPoint().empty()) {
        err = strServiceCode[SERVICEMANCODE::SERVICE_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckUpdateService not found\n");
        return false;
    }
    if (wtx.Getpubkey().empty()) {
        err = strServiceCode[SERVICEMANCODE::PUBKEY_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckUpdateService not found\n");
        return false;
    }
    if (!CheckScriptPubkeyInTxVin(GetScriptForDestination(dockerUpdateService.clusterServiceUpdate.pubKeyClusterAddress.GetID()), wtx2)) {
        err = strServiceCode[SERVICEMANCODE::NO_THRANSACTION];
        LogPrintf("CDockerServerman::CheckUpdateService %s\n", err);
        return false;
    }

    if (wtx2.vout[dockerUpdateService.clusterServiceUpdate.OutPoint.n].scriptPubKey != GetScriptForDestination(pwalletMain->vchDefaultKey.GetID())) {
        err = strServiceCode[SERVICEMANCODE::OUTPOINT_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckUpdateService outpoint not found\n");
        return false;
    }

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx2.GetHash());
    int confirms = wtx2.GetDepthInMainChain(false);
    LogPrint("docker", "current transaction fLocked %d confirms %d\n", fLocked, confirms);
    if (!fLocked && confirms < 1) {
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_NOT_CONFIRMS];
        LogPrintf("CDockerServerman::CheckUpdateService The transaction not confirms: %d\n", confirms);
        return false;
    }
    serviceUpdate.ServiceID = wtx.Getserviceid();
    serviceUpdate.CrerateOutPoint = dockerUpdateService.clusterServiceUpdate.CrerateOutPoint;
    serviceUpdate.pubKeyClusterAddress = dockerUpdateService.clusterServiceUpdate.pubKeyClusterAddress;
    serviceUpdate.OutPoint = dockerUpdateService.clusterServiceUpdate.OutPoint;
    serviceUpdate.Amount = wtx2.vout[serviceUpdate.OutPoint.n].nValue;
    CAmount price = dockerPriceConfig.getPrice("cpu",wtx.GetCPUType()) * boost::lexical_cast<int64_t>(wtx.GetCPUThread()) +
                            dockerPriceConfig.getPrice("mem",wtx.GetMemoryType()) * boost::lexical_cast<int64_t>(wtx.GetMemoryCount()) +
                            dockerPriceConfig.getPrice("gpu", wtx.GetGPUType()) * boost::lexical_cast<int64_t>(wtx.GetGPUCount());
    Item item(wtx.GetCPUType(), boost::lexical_cast<int64_t>(wtx.GetCPUThread()), wtx.GetMemoryType(), boost::lexical_cast<int64_t>(wtx.GetMemoryCount()), wtx.GetGPUType(), boost::lexical_cast<int64_t>(wtx.GetGPUCount()));

    serviceUpdate.ServicePrice = price;
    serviceUpdate.Drawee = CMassGridAddress(serviceUpdate.pubKeyClusterAddress.GetID()).ToString();
    serviceUpdate.MasterNodeFeeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString();
    CPubKey devpubkey(ParseHex(Params().SporkPubKey()));
    serviceUpdate.DevFeeAddress = CMassGridAddress(devpubkey.GetID()).ToString();
    serviceUpdate.MasterNodeFeeRate = dockerServerman.feeRate;
    serviceUpdate.DevFeeRate = sporkManager.GetDeveloperPayment();
    return true;
}

bool CDockerServerman::CheckDeleteService(DockerDeleteService& dockerDeleteService,std::string& msg, std::string& err){
    if (dockerDeleteService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || dockerDeleteService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION) {
        LogPrintf("CDockerServerman::CheckDeleteService --current version %d not support [%d - %d]\n", dockerDeleteService.version, DOCKERREQUEST_API_MINSUPPORT_VERSION, DOCKERREQUEST_API_MAXSUPPORT_VERSION);
        err = "version nout support current version: " + std::to_string(dockerDeleteService.version) + "require: [" + std::to_string(DOCKERREQUEST_API_MINSUPPORT_VERSION) + "," + std::to_string(DOCKERREQUEST_API_MAXSUPPORT_VERSION);
        return false;
    }
    if(dockerDeleteService.sigTime > GetAdjustedTime() + TIMEOUT && dockerDeleteService.sigTime < GetAdjustedTime() - TIMEOUT){
        err = strServiceCode[SERVICEMANCODE::SIGTIME_ERROR];
        LogPrintf("CDockerServerman::CheckDeleteService sigTime Error%s\n");
        return false;
    }
    if (!dockerDeleteService.CheckSignature(dockerDeleteService.pubKeyClusterAddress)) {
        err = strServiceCode[SERVICEMANCODE::PUBKEY_ERROR];
        return false;
    }
        //check transactions
    if (!pwalletMain->mapWallet.count(dockerDeleteService.CrerateOutPoint.hash)) {
        err = strServiceCode[SERVICEMANCODE::NO_THRANSACTION];
        LogPrintf("CDockerServerman::CheckDeleteService Invalid or non-wallet transaction: %s\n", dockerDeleteService.CrerateOutPoint.hash.ToString());
        return false;
    }
    CWalletTx& wtx = pwalletMain->mapWallet[dockerDeleteService.CrerateOutPoint.hash]; //watch only not check
    if (!wtx.Gettlementtxid().empty()){
        err = strServiceCode[SERVICEMANCODE::TRANSACTION_DOUBLE_TLEMENT];
        LogPrintf("CDockerServerman::CheckDeleteService double tlement\n");
        return false;
    }
    if (wtx.Getserviceid().empty()){
        err = strServiceCode[SERVICEMANCODE::SERVICE_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckDeleteService serviceID not found\n");
        return false;
    }
    if (wtx.Getpubkey().empty()){
        err = strServiceCode[SERVICEMANCODE::PUBKEY_NOT_FOUND];
        LogPrintf("CDockerServerman::CheckDeleteService pubkey not found\n");
        return false;
    }
    if(dockerDeleteService.pubKeyClusterAddress.ToString().substr(0, 66) != wtx.Getpubkey()){
        err = strServiceCode[SERVICEMANCODE::PUBKEY_ERROR];
        LogPrintf("CDockerServerman::CheckDeleteService pubkey not equal\n");
        return false;
    }
    uint256 statementTXID;
    if(!prizesClient.GetServiceDelete(wtx.Getserviceid(),statementTXID,err)){  
        LogPrintf("CDockerServerman::CheckDeleteService GetServiceDelete error %s\n",err);
        return false;
    }
    wtx.Settlementtxid(statementTXID.ToString());
    CWalletDB walletdb(pwalletMain->strWalletFile);
    wtx.WriteToDisk(&walletdb);
    msg = "Satement Successful txid: %s" + wtx.Gettlementtxid();
    return true;
}
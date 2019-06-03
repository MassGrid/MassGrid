#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "init.h"
#include "net.h"
#include "wallet/wallet.h"
#include "dockernode.h"
#include "dockercluster.h"
#include "dockerserverman.h"
#include "netbase.h"
#include "masternode-sync.h"
#include "dockerman.h"
#include "coincontrol.h"
#include "validation.h"
#include "messagesigner.h"
#include "instantx.h"
#include "timermodule.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "dockersupernode.h"
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
    "OutPoint not found"
};
bool DockerCreateService::Sign(const CKey& keyClusterAddress, const CPubKey& pubKeyClusterAddress)
{
    std::string strError;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = txid.ToString() + boost::lexical_cast<std::string>(version) + n2n_community +
        serviceName + image + ssh_pubkey + item.ToString() + boost::lexical_cast<std::string>(sigTime) + boost::lexical_cast<std::string>(fPersistentStore);
    for(auto &pair: env){
            strMessage += (pair.first + pair.second);
        }

    if(!CMessageSigner::SignMessage(strMessage, vchSig, keyClusterAddress)) {
        LogPrintf("DockerCreateService::Sign -- SignMessage() failed\n");
        return false;
    }

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerCreateService::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool DockerCreateService::CheckSignature(CPubKey& pubKeyClusterAddress)
{
    // TODO: add sentinel data
    std::string strMessage = txid.ToString() + boost::lexical_cast<std::string>(version) + n2n_community +
        serviceName + image + ssh_pubkey + item.ToString() + boost::lexical_cast<std::string>(sigTime) +  boost::lexical_cast<std::string>(fPersistentStore);
    for(auto &pair: env){
        strMessage += (pair.first + pair.second);
    }
    std::string strError = "";

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerCreateService::CheckSignature -- Got bad signature, error: %s\n", strError);
        return false;
    }
    return true;
}

bool DockerDeleteService::Sign(const CKey& keyClusterAddress, const CPubKey& pubKeyClusterAddress)
{
    std::string strError;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = txid.ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);

    if(!CMessageSigner::SignMessage(strMessage, vchSig, keyClusterAddress)) {
        LogPrintf("DockerDeleteService::Sign -- SignMessage() failed\n");
        return false;
    }
    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerDeleteService::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }
    return true;
}

bool DockerDeleteService::CheckSignature(CPubKey& pubKeyClusterAddress)
{
    // TODO: add sentinel data
    std::string strMessage = txid.ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);

    std::string strError = "";

    if(!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerDeleteService::CheckSignature -- Got bad signature, error: %s\n", strError);
        return false;
    }
    return true;
}
void CDockerServerman::ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman){
    
    if (!masternodeSync.IsSynced()) return;
    
    if (strCommand == NetMsgType::GETDNDATA) { //server
        
        LogPrint("dockernode","CDockerServerman::ProcessMessage GETDNDATA Started\n");
        if (!fDockerNode) return;
        DockerGetData mdndata;
        CPubKey pubkey;
        vRecv >> pubkey;

        mdndata.mapDockerServiceLists.clear();
        mdndata.mapDockerServiceLists = dockerman.GetServiceFromPubkey(pubkey);
        mdndata.sigTime = GetAdjustedTime();
        mdndata.fPersistentStore = dockerman.fPersistentStore;
        mdndata.items = dockerman.GetPriceListFromNodelist();
        mdndata.masternodeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString();
        LogPrint("dockernode", "CDockerServerman::ProcessMessage GETDNDATA -- pubkey %s servicelistSize() %d \n", pubkey.ToString().substr(0,66),mdndata.mapDockerServiceLists.size());
        LogPrintf("CDockerServerman::ProcessMessage -- Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
        
    }else if(strCommand == NetMsgType::DNDATA){     //cluster

        LogPrint("dockernode","CDockerServerman::ProcessMessage DNDATA Started\n");
        DockerGetData mdndata;
        vRecv >> mdndata;
        if(mdndata.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || mdndata.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", mdndata.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            setDNDataStatus(DNDATASTATUS::Received);
            return;
        }
         
        dockercluster.dndata = mdndata;
        LogPrint("dockernode","CDockerServerman::ProcessMessage DNDATA servicelistSize() %d\n",mdndata.mapDockerServiceLists.size());
        std::map<std::string,Service>::iterator iter = mdndata.mapDockerServiceLists.begin();
        for(;iter != mdndata.mapDockerServiceLists.end();iter++){
            if (pwalletMain->mapWallet.count(iter->second.txid)){
                CWalletTx& wtx = pwalletMain->mapWallet[iter->second.txid];
                if(wtx.HasCreatedService())
                    continue;
                wtx.Setserviceid(iter->first);
                wtx.Setcreatetime(std::to_string(iter->second.createdAt));
                wtx.Setverison(std::to_string(WALLET_DATABASE_VERSION));
                wtx.Setprice(std::to_string(iter->second.price));
                wtx.Setcpuname(iter->second.item.cpu.Name);
                wtx.Setcpucount(std::to_string(iter->second.item.cpu.Count));
                wtx.Setmemname(iter->second.item.mem.Name);
                wtx.Setmemcount(std::to_string(iter->second.item.mem.Count));
                wtx.Setgpuname(iter->second.item.gpu.Name);
                wtx.Setgpucount(std::to_string(iter->second.item.gpu.Count));
                wtx.Setcusteraddress(iter->second.customer);
                CWalletDB walletdb(pwalletMain->strWalletFile);
                wtx.WriteToDisk(&walletdb);
            }
        }
        setDNDataStatus(DNDATASTATUS::Received);

    }else if (strCommand == NetMsgType::GETTRANS) { //docker gettransaction
        
        LogPrint("dockernode","CDockerServerman::ProcessMessage GETTRANS Started\n");
        if (!fDockerNode) return;
        DockerGetTranData dtdata;
        vRecv >> dtdata;

        DockerTransData dockerTransData;
        if(CheckAndGetTransaction(dtdata,dockerTransData.errCode)){
            CWalletTx wtx = pwalletMain->mapWallet[dtdata.txid];  //watch only not check
            if(wtx.HasTlemented()){
                dockerTransData.sigTime = GetAdjustedTime();
                std::string feerate= wtx.Getfeerate();            
                dockerTransData.feeRate = boost::lexical_cast<double>(feerate.empty()?"0":feerate);
                std::string deltime= wtx.Getdeletetime();
                dockerTransData.deleteTime = boost::lexical_cast<int64_t>(deltime.empty()?"0":deltime);
                std::string taskstate=wtx.Gettaskstate();
                dockerTransData.errCode = boost::lexical_cast<int>(taskstate.empty()?"0":taskstate);
                dockerTransData.taskStatus = wtx.Gettaskstatuscode();
                std::string tlementtxid = wtx.Gettlementtxid();
                if(tlementtxid.empty())
                    dockerTransData.tlementtxid = uint256();
                else
                    dockerTransData.tlementtxid = uint256S(tlementtxid);
                if(dtdata.askCode == DockerGetTranData::ASKALL){
                    std::string createtime=wtx.Getcreatetime();
                    if(!createtime.empty()) 
                        dockerTransData.extData["createtime"] = createtime;

                    std::string custeraddress=wtx.Getcusteraddress();
                    if(!custeraddress.empty()) 
                        dockerTransData.extData["custeraddress"] = custeraddress;

                    std::string provideraddress=wtx.Getprovideraddress();
                    if(!provideraddress.empty()) 
                        dockerTransData.extData["provideraddress"] = provideraddress;
                    
                    std::string masternodeaddress=wtx.Getmasternodeaddress();
                    if(!masternodeaddress.empty()) 
                        dockerTransData.extData["masternodeaddress"] = masternodeaddress;
                    
                    std::string serviceid = wtx.Getserviceid();
                    if(!serviceid.empty()) 
                        dockerTransData.extData["serviceid"] = serviceid;
                    
                    std::string version = wtx.Getverison();
                    if(!version.empty()) 
                        dockerTransData.extData["verison"] = version;

                    std::string price = wtx.Getprice();
                    if(!price.empty()) dockerTransData.extData["price"] = price;
                    
                    std::string gpucount = wtx.Getgpucount();
                    if(!gpucount.empty()) 
                        dockerTransData.extData["gpucount"] = gpucount;

                    std::string gpuname = wtx.Getgpuname();
                    if(!gpuname.empty()) 
                        dockerTransData.extData["gpuname"] = gpuname;

                    std::string cpucount=wtx.Getcpucount();
                    if(!cpucount.empty()) 
                        dockerTransData.extData["cpucount"]=cpucount;

                    std::string cpuname = wtx.Getcpuname();
                    if(!cpuname.empty()) 
                        dockerTransData.extData["cpuname"] = cpuname;

                    std::string memcount = wtx.Getmemcount();
                    if(!memcount.empty()) 
                        dockerTransData.extData["memcount"] = memcount;

                    std::string memname = wtx.Getmemname();
                    if(!memname.empty()) 
                        dockerTransData.extData["memname"] = memname;
                }
                dockerTransData.msgStatus = TASKDTDATA::SUCCESS;
            }else{
                dockerTransData.errCode =SERVICEMANCODE::NO_THRANSACTION;
                dockerTransData.msgStatus = TASKDTDATA::ERRORCODE;
            }
        }else{
            dockerTransData.msgStatus=TASKDTDATA::ERRORCODE;
        }
        dockerTransData.txid = dtdata.txid;
        LogPrint("timer","CDockerServerman::GETTRANS status %d msgstatus %d\n", dockerTransData.errCode,dockerTransData.msgStatus);
        LogPrintf("CDockerServerman::ProcessMessage -- Sent TRANSDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::TRANSDATA, dockerTransData);
        
    }else if(strCommand == NetMsgType::TRANSDATA){     //client

        LogPrint("dockernode","CDockerServerman::ProcessMessage TRANSDATA Started\n");
        DockerTransData dockerTransData;
        vRecv >> dockerTransData;
        if(dockerTransData.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || dockerTransData.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", dockerTransData.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            return;
        }
        dockercluster.dtdata = dockerTransData;
        if (pwalletMain->mapWallet.count(dockerTransData.txid)){
            CWalletTx& wtx = pwalletMain->mapWallet[dockerTransData.txid];
            if(wtx.HasTlemented()) return;
            if(dockerTransData.msgStatus != TASKDTDATA::SUCCESS){
                dockerTransData.tlementtxid.SetNull();
                wtx.Settlementtxid(dockerTransData.tlementtxid.ToString());
                LogPrint("timer","CDockerServerman::TRANSDATA msgStatus %d %s\n",dockerTransData.msgStatus,dockerTransData.tlementtxid.ToString());
            }else{
                wtx.Setdeletetime(std::to_string(dockerTransData.deleteTime));
                wtx.Setfeerate(std::to_string(dockerTransData.feeRate));
                wtx.Settaskstate(std::to_string(dockerTransData.errCode));
                wtx.Settaskstatuscode(dockerTransData.taskStatus);
                wtx.Settlementtxid(dockerTransData.tlementtxid.ToString());
                for(auto &it: dockerTransData.extData){
                    if(it.first == "createtime") 
                        wtx.Setcreatetime(it.second);
                    else if(it.first == "custeraddress") 
                        wtx.Setcusteraddress(it.second);
                    else if(it.first == "provideraddress") 
                        wtx.Setprovideraddress(it.second);
                    else if(it.first == "masternodeaddress") 
                        wtx.Setmasternodeaddress(it.second);
                    else if(it.first == "serviceid") 
                        wtx.Setserviceid(it.second);
                    else if(it.first == "verison") 
                        wtx.Setverison(it.second);
                    else if(it.first == "price") 
                        wtx.Setprice(it.second);
                    else if(it.first == "gpucount") 
                        wtx.Setgpucount(it.second);
                    else if(it.first == "gpuname") 
                        wtx.Setgpuname(it.second);
                    else if(it.first == "memcount") 
                        wtx.Setmemcount(it.second);
                    else if(it.first == "memname") 
                        wtx.Setmemname(it.second);
                    else if(it.first == "cpuname") 
                        wtx.Setcpuname(it.second);
                    else if(it.first == "cpucount") 
                        wtx.Setcpucount(it.second);
                }
            }
            CWalletDB walletdb(pwalletMain->strWalletFile);
            wtx.WriteToDisk(&walletdb);
        }
        setTRANSDataStatus(TRANSDATASTATUS::ReceivedTD);
    }else if(strCommand == NetMsgType::CREATESERVICE){
        LogPrint("dockernode","CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        if (!fDockerNode) return;
        DockerCreateService createService;
        vRecv >> createService;
        DockerGetData mdndata;
        mdndata.sigTime = GetAdjustedTime();
        LogPrint("dockernode","CDockerServerman::ProcessMessage CREATESERVICE txid %s\n",createService.txid.ToString());
        if(CheckAndCreateServiveSpec(createService,mdndata.errCode)){
            mdndata.mapDockerServiceLists.clear();
            mdndata.mapDockerServiceLists = dockerman.GetServiceFromPubkey(createService.pubKeyClusterAddress);
        }
        LogPrintf("CDockerServerman::ProcessMessage -- CREATESERVICE Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
    }else if(strCommand == NetMsgType::DELETESERVICE){
        LogPrint("dockernode","CDockerServerman::ProcessMessage DELETESERVICE Started\n");
        if (!fDockerNode) return;
        DockerDeleteService delService;
        vRecv >> delService;
        DockerGetData mdndata;
        mdndata.sigTime = GetAdjustedTime();
        LogPrint("dockernode","CDockerServerman::ProcessMessage DELETESERVICE txid %s\n",delService.txid.ToString());
        if(CheckAndRemoveServiveSpec(delService,mdndata.errCode)){
            mdndata.mapDockerServiceLists.clear();
            mdndata.mapDockerServiceLists = dockerman.GetServiceFromPubkey(delService.pubKeyClusterAddress);
        }
            
        LogPrintf("CDockerServerman::ProcessMessage -- DELETESERVICE Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
    }
}
bool CDockerServerman::CheckAndRemoveServiveSpec(DockerDeleteService delService, int& errCode){

    LogPrint("dockernode","CDockerServerman::CheckAndRemoveServiveSpec Started\n");
    
    if(delService.sigTime > GetAdjustedTime() + TIMEOUT && delService.sigTime < GetAdjustedTime() - TIMEOUT){
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec DELETESERVICE sigTime is out of vaild timespan = %d\n",delService.sigTime);
        errCode = SERVICEMANCODE::SIGTIME_ERROR;
        return false;
    }
    if(delService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || delService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec --current version %d not support [%d - %d]\n", delService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
        errCode = SERVICEMANCODE::VERSION_ERROR;
        return false;
    }
    if(!delService.CheckSignature(delService.pubKeyClusterAddress)){
        errCode = SERVICEMANCODE::CHECKSIGNATURE_ERROR;
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec DELETESERVICE --CheckSignature Failed pubkey= %s\n",delService.pubKeyClusterAddress.ToString().substr(0,66));
        return false;
    }
    if(!pwalletMain->mapWallet.count(delService.txid)){
        errCode = SERVICEMANCODE::NO_THRANSACTION;
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec Invalid or non-wallet transaction: %s\n",delService.txid.ToString());
        return false;
    }
    CWalletTx& wtx = pwalletMain->mapWallet[delService.txid];  //watch only not check
    if(wtx.HasTlemented()){
        errCode = SERVICEMANCODE::TRANSACTION_DOUBLE_TLEMENT;
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec current transaction has been used\n");
        return false;
    }
    //check transaction prescript pubkey
    CTransaction tx;
    uint256 hash;
    std::string strErr;
    if(!CheckTransactionInputScriptPubkey(delService.txid, tx,delService.pubKeyClusterAddress, Params().GetConsensus(), hash,strErr, true)){
        errCode = SERVICEMANCODE::PUBKEY_ERROR;
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec %s\n",strErr);
        return false;
    }
    // check outpoint 
    COutPoint outpoint;
    CMassGridAddress masternodeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID());
    CScript masternodescriptPubKey = GetScriptForDestination(masternodeAddress.Get());

    if(!wtx.GetOutPoint(masternodescriptPubKey,outpoint)){
        errCode = SERVICEMANCODE::OUTPOINT_NOT_FOUND;
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec outpoint not found\n");
        return false;
    }
    Coin coin;
    if(!GetUTXOCoin(outpoint,coin)){
        errCode = SERVICEMANCODE::TRANSACTION_NOT_CONFIRMS;
        LogPrintf("CDockerServerman::CheckAndRemoveServiveSpec outpoint not UTXO\n");
        return false;
    }
    Service svi;
    if(dockerman.GetServiceFromTxId(delService.txid,svi))
        dockerman.PushMessage(Method::METHOD_SERVICES_DELETE,svi.ID,"");
    else 
        timerModule.UpdateSet(wtx);
    return true;
}
bool CDockerServerman::CheckAndCreateServiveSpec(DockerCreateService createService, int& errCode){

    LogPrint("dockernode","CDockerServerman::CheckAndCreateServiveSpec Started\n");
    // 1.first check time
    if(createService.sigTime > GetAdjustedTime() + TIMEOUT && createService.sigTime < GetAdjustedTime() - TIMEOUT){
        errCode = SERVICEMANCODE::SIGTIME_ERROR;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec sigTime Error%s\n");
        return false;
    }
    if(createService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || createService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
        errCode = SERVICEMANCODE::VERSION_ERROR;
        LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", createService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
        return false;
    }     
    //  2. checkSignature
    if(!createService.CheckSignature(createService.pubKeyClusterAddress)){
        errCode = SERVICEMANCODE::CHECKSIGNATURE_ERROR;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- CheckSignature() failed\n");
        return false;
    }
    //  3.check transactions
    if (!pwalletMain->mapWallet.count(createService.txid)){
        errCode = SERVICEMANCODE::NO_THRANSACTION;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec Invalid or non-wallet transaction: %s\n",createService.txid.ToString());
        return false;
    }
    
    //check transaction prescript pubkey
    CTransaction tx;
    uint256 hash;
    std::string strErr;
    if(!CheckTransactionInputScriptPubkey(createService.txid, tx,createService.pubKeyClusterAddress, Params().GetConsensus(), hash,strErr, true)){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec %s\n",strErr);
        errCode = SERVICEMANCODE::NO_THRANSACTION;
        return false;
    }

    CWalletTx& wtx = pwalletMain->mapWallet[createService.txid];  //watch only not check

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx.GetHash());
    int confirms = wtx.GetDepthInMainChain(false);
    LogPrint("dockernode","current transaction fLocked %d confirms %d\n",fLocked,confirms);
    if(!fLocked && confirms < 1){
        errCode = SERVICEMANCODE::TRANSACTION_NOT_CONFIRMS;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec The transaction not confirms: %d\n",confirms);
        return false;
    }
    if(wtx.HasCreatedService()){
        errCode = SERVICEMANCODE::TRANSACTION_DOUBLE_CREATE;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec current transaction has been used\n");
        return false;
    }
    
    isminefilter filter = ISMINE_SPENDABLE;
    CAmount nCredit = wtx.GetCredit(filter);
    CAmount nDebit = wtx.GetDebit(filter);
    CAmount nNet = nCredit - nDebit;
    CAmount nFee = (wtx.IsFromMe(filter) ? wtx.GetValueOut() - nDebit : 0);
    CAmount payment = nNet - nFee;

    //  4. find item

    const Item serviceItem = createService.item;
    auto usablemapnode = dockerman.GetPriceListFromNodelist();
    if(!usablemapnode.count(serviceItem)){
        errCode = SERVICEMANCODE::SERVICEITEM_NOT_FOUND;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- serviceitem not found\n");
        return false;
    }
    if(!usablemapnode[serviceItem].count){
        errCode = SERVICEMANCODE::SERVICEITEM_NO_RESOURCE;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- serviceitem no resource\n");
        return false;
    }

    //  5. check item
    CAmount price = usablemapnode[serviceItem].price;
    if(payment < price){
        errCode = SERVICEMANCODE::PAYMENT_NOT_ENOUGH;
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- serviceitem payment not enough\n");
        return false;
    }


    if(!(serviceItem.gpu.Count > 0 && serviceItem.gpu.Count <= DOCKER_MAX_GPU_NUM)){
        errCode = SERVICEMANCODE::GPU_AMOUNT_ERROR;
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec gpu %d,  DOCKER_MAX_GPU_NUM,%d\n",serviceItem.gpu.Count,DOCKER_MAX_GPU_NUM);
        return false;
    }
    if(!(serviceItem.cpu.Count > 0 && serviceItem.cpu.Count <= DOCKER_MAX_CPU_NUM)){
        errCode = SERVICEMANCODE::CPU_AMOUNT_ERROR;
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec cpu %d,  DOCKER_MAX_CPU_NUM,%d\n",serviceItem.cpu.Count,DOCKER_MAX_CPU_NUM);
        return false;
    }
    if(!(serviceItem.mem.Count > 0 && serviceItem.mem.Count <= DOCKER_MAX_MEMORY_BYTE)){
        errCode = SERVICEMANCODE::MEM_AMOUNT_ERROR;
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec memory_byte %d,  DOCKER_MAX_MEMORY_BYTE %d\n",serviceItem.mem.Count,DOCKER_MAX_MEMORY_BYTE);
        return false;
    }

    //  6. update spec

    Config::ServiceSpec spec;
    if(!createService.serviceName.empty())
        spec.name = createService.serviceName.substr(0,10) + "_" + createService.ToString().substr(0,4);
    else
        spec.name = createService.ToString().substr(0,15);
    spec.labels["com.massgrid.deletetime"] = std::to_string(payment/price * 3600 + GetAdjustedTime());
    spec.labels["com.massgrid.feerate"] = std::to_string(dockerServerman.feeRate);
    spec.labels["com.massgrid.pubkey"] = createService.pubKeyClusterAddress.ToString().substr(0,66);
    spec.labels["com.massgrid.txid"] = createService.txid.ToString();
    spec.labels["com.massgrid.price"] =std::to_string(price);
    spec.labels["com.massgrid.payment"] = std::to_string(payment);
    spec.labels["com.massgrid.cpuname"] = serviceItem.cpu.Name;
    spec.labels["com.massgrid.cpucount"] = std::to_string(serviceItem.cpu.Count);
    spec.labels["com.massgrid.memname"] = serviceItem.mem.Name;
    spec.labels["com.massgrid.memcount"] = std::to_string(serviceItem.mem.Count);
    spec.labels["com.massgrid.gpuname"] = serviceItem.gpu.Name;
    spec.labels["com.massgrid.gpucount"] = std::to_string(serviceItem.gpu.Count);
    spec.mode.replicated.replicas = 1;
    
    if(!createService.image.empty() && createService.image.substr(0,9) == "massgrid/")
        spec.taskTemplate.containerSpec.image = createService.image;
    else
        spec.taskTemplate.containerSpec.image = "massgrid/10.0-base-ubuntu16.04";
    spec.taskTemplate.containerSpec.user= "root";
    
    spec.taskTemplate.resources.limits.memoryBytes = serviceItem.mem.Count *G;
    spec.taskTemplate.resources.limits.nanoCPUs = serviceItem.cpu.Count * G;

    Config::GenericResources otherresources;
    otherresources.discreateResourceSpec.kind = serviceItem.gpu.Name;
    otherresources.discreateResourceSpec.value = serviceItem.gpu.Count;
    spec.taskTemplate.resources.reservations.genericResources.push_back(otherresources);

    spec.taskTemplate.restartPolicy.condition = "none";
    spec.taskTemplate.containerSpec.command.push_back("start.sh");

    if(createService.n2n_community.empty())
        spec.taskTemplate.containerSpec.env.push_back("N2N_NAME=massgridn2n");
    else
        spec.taskTemplate.containerSpec.env.push_back("N2N_NAME="+createService.n2n_community.substr(0,20));
    
    std::string ipaddr = dockerman.serviceIpList.GetFreeIP();
    spec.taskTemplate.containerSpec.env.push_back("N2N_SERVERIP=" + ipaddr);
    spec.taskTemplate.containerSpec.env.push_back("N2N_NETMASK=" + dockerman.serviceIpList.GetNetMask());
    spec.taskTemplate.containerSpec.env.push_back("N2N_SNIP=" + dockerman.GetMasterIp() + ":" + boost::lexical_cast<std::string>(GetSNPort()));
    spec.taskTemplate.containerSpec.env.push_back("SSH_PUBKEY=" + createService.ssh_pubkey);
    spec.taskTemplate.containerSpec.env.push_back("CPUNAME=" + serviceItem.cpu.Name);
    spec.taskTemplate.containerSpec.env.push_back("CPUCOUNT=" + std::to_string(serviceItem.cpu.Count));
    spec.taskTemplate.containerSpec.env.push_back("MEMNAME=" + serviceItem.mem.Name);
    spec.taskTemplate.containerSpec.env.push_back("MEMCOUNT=" + std::to_string(serviceItem.mem.Count));
    spec.taskTemplate.containerSpec.env.push_back("GPUNAME=" + serviceItem.gpu.Name);
    spec.taskTemplate.containerSpec.env.push_back("GPUCOUNT=" + std::to_string(serviceItem.gpu.Count));
    for(auto &pair: createService.env){
            std::string envs=pair.first+"="+pair.second;
            strToUpper(envs,1);
            spec.taskTemplate.containerSpec.env.push_back(envs);
        }

    spec.taskTemplate.placement.constraints.push_back("node.role == worker");
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.cpuname == "+serviceItem.cpu.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.cpucount == "+std::to_string(serviceItem.cpu.Count)));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.memname == "+serviceItem.mem.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.memcount == "+std::to_string(serviceItem.mem.Count)));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.gpuname == "+serviceItem.gpu.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.gpucount == "+std::to_string(serviceItem.gpu.Count)));
    std::string nodeid;
    if(GetNodeIdAndstopMiner(serviceItem,nodeid)){
        spec.taskTemplate.placement.constraints.push_back(std::string("node.id == "+nodeid));
        LogPrintf("CDockerServerman::GetNodeIdAndstopMiner nodeid %s\n",nodeid);
    }
    Config::Mount mount;
    mount.target="/dev/net";
    mount.source="/dev/net";
    spec.taskTemplate.containerSpec.mounts.push_back(mount);

    if(createService.fPersistentStore && dockerman.fPersistentStore){
        Node node;
        std::string nfsip;
        if(dockerman.GetNodeFromList(nodeid,node) && node.description.engine.labels.count("nfsip")){
            Config::Mount persistentMount;
            persistentMount.type = "volume";
            persistentMount.target = "/mnt";
            persistentMount.volumeOptions.driverConfig.name = "massgrid/nfs-volume-plugin";
            persistentMount.volumeOptions.driverConfig.options["device"] = node.description.engine.labels["nfsip"] + ":/" + createService.pubKeyClusterAddress.ToString().substr(0,66);
            persistentMount.volumeOptions.driverConfig.options["nfsopts"] = "hard,proto=tcp,nfsvers=4,intr,nolock";
            spec.taskTemplate.containerSpec.mounts.push_back(persistentMount);
        } 
    }

    //  7.request docker
    if(!dockerman.PushMessage(Method::METHOD_SERVICES_CREATE,"",spec.ToJsonString())){
        dockerman.serviceIpList.Erase(ipaddr);
        return false;
    }
    return true;

}
bool CDockerServerman::GetNodeIdAndstopMiner(Item item,std::string &nodeid){
    std::string tmpnodeid = dockerman.GetNodeFromItem(item);
    std::string serviceid;
    if(!dockerman.GetServiceidFromNode(tmpnodeid,serviceid)){
        LogPrintf("CDockerServerman::GetNodeIdAndstopMiner not found serviceid from nodeid,maybe the miner not start\n");
        nodeid=tmpnodeid;
        return true;
    }
    Service service;
    if(!dockerman.GetSerivceFromServiceID(serviceid,service)){
        LogPrintf("CDockerServerman::GetNodeIdAndstopMiner get service from services id faild %s\n",serviceid);
        nodeid=tmpnodeid;
        return true;
    }
    if( "massgridminer_" == service.spec.name.substr(0,14)){
        for(auto it = service.spec.labels.begin();it!=service.spec.labels.end();++it){
            if(it->first == "com.massgrid.miner"){
                dockerman.PushMessage(Method::MINER_SERVICES_DELETE,serviceid,"");
                LogPrintf("CDockerServerman::GetNodeIdAndstopMiner delete miner service %s\n",serviceid);
                nodeid=tmpnodeid;
                return true;
            }
        }
        return false;
    }
    return true;
}
bool CDockerServerman::CreateMinerServiceSpec(std::string nodeid){
    LogPrint("dockernode","CDockerServerman::CreateMinerServiceSpec Started\n");
    Item serviceItem;
    if(!dockerman.GetItemFromNodeID(nodeid,serviceItem)){
        LogPrintf("CDockerServerman::CreateMinerServiceSpec node %s is not found\n",nodeid);
        return false;
    }
    Config::ServiceSpec spec;
    spec.mode.replicated.replicas = 1;

    spec.name = "massgridminer_"+ std::to_string(insecure_rand());
    spec.labels["com.massgrid.deletetime"] = std::to_string(MINER_MAX_TIME + GetAdjustedTime());
    spec.labels["com.massgrid.miner"] = "1";
    spec.taskTemplate.containerSpec.image = "massgrid/10.0-autominer-ubuntu16.04";
    spec.taskTemplate.containerSpec.user= "root";
    
    spec.taskTemplate.resources.limits.memoryBytes = serviceItem.mem.Count *G;
    spec.taskTemplate.resources.limits.nanoCPUs = serviceItem.cpu.Count * G;

    Config::GenericResources otherresources;
    otherresources.discreateResourceSpec.kind = serviceItem.gpu.Name;
    otherresources.discreateResourceSpec.value = serviceItem.gpu.Count;
    spec.taskTemplate.resources.reservations.genericResources.push_back(otherresources);

    spec.taskTemplate.restartPolicy.condition = "none";
    spec.taskTemplate.containerSpec.command.push_back("minerstart.sh");
    Node node;
    if(dockerman.GetNodeFromList(nodeid,node)){
        for(auto &labels: node.description.engine.labels){
            std::string envs=labels.first+"="+labels.second;
            strToUpper(envs,1);
            spec.taskTemplate.containerSpec.env.push_back(envs);
        }
    }else{
        LogPrintf("Warring CDockerServerman::CreateMinerServiceSpec node %s is not exits\n",nodeid);
    }
    spec.taskTemplate.placement.constraints.push_back("node.role == worker");
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.cpuname == "+serviceItem.cpu.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.cpucount == "+std::to_string(serviceItem.cpu.Count)));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.memname == "+serviceItem.mem.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.memcount == "+std::to_string(serviceItem.mem.Count)));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.gpuname == "+serviceItem.gpu.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.gpucount == "+std::to_string(serviceItem.gpu.Count)));
    spec.taskTemplate.placement.constraints.push_back(std::string("node.id == "+nodeid));
    
    Config::Mount mount;
    mount.target="/dev/net";
    mount.source="/dev/net";
    spec.taskTemplate.containerSpec.mounts.push_back(mount);

    if(!dockerman.PushMessage(Method::MINER_SERVICES_CREATE,"",spec.ToJsonString())){
        LogPrintf("CDockerServerman::CreateMinerServiceSpec MINER_SERVICES_CREATE failed\n");
        return false;
    }
    return true;
}
bool CheckAndGetTransaction(DockerGetTranData dtdata,int& errCode){
    // 1.first check time
    if(dtdata.sigTime > GetAdjustedTime() + TIMEOUT && dtdata.sigTime < GetAdjustedTime() - TIMEOUT){
        errCode = SERVICEMANCODE::SIGTIME_ERROR;
        LogPrintf("CDockerServerman::CheckAndGetTransaction sigTime Error %lld\n",dtdata.sigTime);
        return false;
    }
    // 2 .check docker version
    if(dtdata.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || dtdata.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
        errCode = SERVICEMANCODE::VERSION_ERROR;
        LogPrintf("CDockerServerman::ProcessMessage --current version %d not support [%d - %d]\n", dtdata.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
        return false;
    }
    //  3.check transactions
    if (!pwalletMain->mapWallet.count(dtdata.txid)){
        errCode = SERVICEMANCODE::NO_THRANSACTION;
        LogPrintf("CDockerServerman::CheckAndGetTransaction Invalid or non-wallet transaction: %s\n",dtdata.txid.ToString());
        return false;
    }
    return true;
}
int CDockerServerman::SetTlementServiceWithoutDelete(uint256 serviceTxid){

    bool fnoCreated;
    vector<CRecipient> vecSend;
    COutPoint outpoint;
    CMassGridAddress masternodeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID());
    CScript masternodescriptPubKey = GetScriptForDestination(masternodeAddress.Get());
    CPubKey devpubkey(ParseHex(Params().SporkPubKey()));
    CMassGridAddress feeAddress;
    if(!mnodeman.GetAddress(activeMasternode.outpoint,feeAddress))
        feeAddress = masternodeAddress;
    CScript feescriptPubKey = GetScriptForDestination(feeAddress.Get());
    CMassGridAddress customerAddress = feeAddress;
    CMassGridAddress providerAddress = feeAddress;
    CMassGridAddress devAddress = CMassGridAddress(devpubkey.GetID());
    CScript customerscriptPubKey = feescriptPubKey;
    CScript providerscriptPubKey = feescriptPubKey;;
    CScript devScriptPubKey = GetScriptForDestination(devAddress.Get());

    if (!pwalletMain->mapWallet.count(serviceTxid)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete Invalid or non-wallet transaction id\n");
        return TLEMENTSTATE::FAILEDCONTINUE;
    }
    CWalletTx& wtx = pwalletMain->mapWallet[serviceTxid];  //watch only not check

    if(wtx.HasTlemented()){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete has been tlementtxid\n");
        return TLEMENTSTATE::FAILEDREMOVE;
    }
    
    if(!wtx.GetOutPoint(masternodescriptPubKey,outpoint)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete outpoint not found\n");
        return TLEMENTSTATE::FAILEDREMOVE;
    }
    Coin coin;
    if(!GetUTXOCoin(outpoint,coin)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete outpoint not UTXO\n");
        return TLEMENTSTATE::FAILEDREMOVE;
    }

    isminefilter filter = ISMINE_SPENDABLE;
    CAmount nCredit = wtx.GetCredit(filter);
    CAmount nDebit = wtx.GetDebit(filter);
    CAmount nNet = nCredit - nDebit;
    CAmount nFee = (wtx.IsFromMe(filter) ? wtx.GetValueOut() - nDebit : 0);
    CAmount payment = nNet - nFee;
    const CAmount pay = payment;

    if(!wtx.HasCreatedService()){
        LogPrint("dockernode","CDockerServerman::SetTlementServiceWithoutDelete current transaction not been used\n");

        CTransaction txOut;
        uint256 hash;
        if(GetTransaction(serviceTxid, txOut, Params().GetConsensus(), hash, true)){
            for(int i = 0; i< txOut.vin.size(); ++i){
                CTransaction tx;
                if(GetTransaction(txOut.vin[i].prevout.hash, tx, Params().GetConsensus(), hash, true)){
                    customerscriptPubKey = tx.vout[txOut.vin[i].prevout.n].scriptPubKey;
                    break;
                }
            }
         }
        vecSend.clear();
        CAmount customerSend = payment;
        if(customerSend > CAmount(0)){ 
            CRecipient customerrecipient = {customerscriptPubKey, customerSend, true};
            vecSend.push_back(customerrecipient);
        }
    }
    else{
        LogPrint("dockernode","CDockerServerman::SetTlementServiceWithoutDelete current transaction has been used\n");
        if(wtx.Getdeletetime().empty()){
            LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete current transaction deletetime invaild\n");
            return TLEMENTSTATE::FAILEDCONTINUE;
        }

        customerAddress = CMassGridAddress(wtx.Getcusteraddress());
        providerAddress = CMassGridAddress(wtx.Getprovideraddress());
        double feeRate = boost::lexical_cast<double>(wtx.Getfeerate());
        int64_t createTime = boost::lexical_cast<int64_t>(wtx.Getcreatetime());
        int64_t deleteTime = boost::lexical_cast<int64_t>(wtx.Getdeletetime());
        CAmount price = boost::lexical_cast<CAmount>(wtx.Getprice());
        int taskStatus = boost::lexical_cast<int>(wtx.Gettaskstate());
        fnoCreated = taskStatus < Config::TASKSTATE_RUNNING;
        

        if(customerAddress.IsValid())
            customerscriptPubKey = GetScriptForDestination(customerAddress.Get());
        if(providerAddress.IsValid())
            providerscriptPubKey = GetScriptForDestination(providerAddress.Get());
        if (feeRate >= 1 ||feeRate < 0){
            LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete svi.feeRate reset %lf\n",feeRate);
            feeRate = 0;
        }
        int64_t trustTime = deleteTime - createTime;
        int64_t prepareTime = (double)payment / price * 3600;
        if(trustTime > prepareTime)
            trustTime = prepareTime;
        double payrate = (double)trustTime / prepareTime;
        if(payrate < 0|| payrate >= 1.0){
            LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete payrate %lf error\n",payrate);
            return TLEMENTSTATE::FAILEDREMOVE;
        }
        fnoCreated  |= (taskStatus >= Config::TASKSTATE_SHUTDOWN && trustTime<=180);
        if(fnoCreated){  //not create task
            // customer
            vecSend.clear();
            CAmount customerSend = payment;
            if(customerSend > CAmount(1000)){
                CRecipient customerrecipient = {customerscriptPubKey, customerSend, true};
                vecSend.push_back(customerrecipient);
            }

        }else
        {
            vecSend.clear();
            CAmount masternodeSend = payment * feeRate;
            double devRate = sporkManager.GetDeveloperPayment()/10000.0;
            if( devRate < 0 || devRate >= 1) 
                devRate = 0;
            CAmount devSend = payment * devRate;
            while(devSend/(double)payment < devRate){
                devSend += payment * 0.001;
            }
            payment = payment - masternodeSend;  //compute fee

            payment = payment - devSend;  //compute fee

            // provider
            CAmount providerSend = payment * payrate;
            CAmount customerSend = payment * (1 - payrate);
            if(providerSend > CAmount(1000)){
                payment -= providerSend;
                CRecipient providerrecipient = {providerscriptPubKey, providerSend, false};
                vecSend.push_back(providerrecipient);
            }

            // customer
            if(customerSend > CAmount(1000)){
                payment -= customerSend;
                CRecipient customerrecipient = {customerscriptPubKey, customerSend, false};
                vecSend.push_back(customerrecipient);
            }
            //masternode fee
            masternodeSend += payment;
            if(masternodeSend + devSend + providerSend + customerSend != pay){
                LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete payment error masternodeSend %lld providerSend %lld customerSend %lld devSend %lld sumpayment %lld\n",masternodeSend, providerSend , customerSend ,devSend,pay);
                return TLEMENTSTATE::FAILEDREMOVE;
            }
            LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete masternodeSend %lld providerSend %lld customerSend %lld devSend %lld pay %lld feerate %lf payrate %lf trustTime %lld prepareTime %lld\n",masternodeSend, providerSend , customerSend ,devSend,pay,feeRate,payrate,trustTime,prepareTime);
            if(masternodeSend > CAmount(1000)){
                CRecipient masternoderecipient = {feescriptPubKey, masternodeSend, false};
                vecSend.push_back(masternoderecipient);
            }
            
            for(auto it = vecSend.rbegin();it!= vecSend.rend();++it){
                if(it->nAmount >= CENT){
                    it->fSubtractFeeFromAmount =true;
                    break;
                }
            }
            // Miners'fees are deducted from other transactions.
            if(devSend > CAmount(1000)){
                CRecipient devrecipient = {devScriptPubKey, devSend, false};
                vecSend.push_back(devrecipient);
            }       
        }
    }
    if (!vecSend.size()){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete vecSend.size() == 0 \n");
        return TLEMENTSTATE::FAILEDREMOVE;
    }

    CWalletTx wtxNew;
    CCoinControl coinControl;
    coinControl.fUseInstantSend = true;
    coinControl.Select(outpoint);
    coinControl.destChange = masternodeAddress.Get();
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::string strError;
    CAmount nFeeRet = 0;
    int nChangePosRet = -1;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet, nChangePosRet, strError, &coinControl, true, ALL_COINS, true);
    if (!fCreated){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete CreateTransaction error %s\n",strError);
        return TLEMENTSTATE::FAILEDCONTINUE;
    }

    //commit to wtx;
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey, g_connman.get(),NetMsgType::TXLOCKREQUEST)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete CommitTransaction error \n");
        return TLEMENTSTATE::FAILEDCONTINUE;
    }
    wtx.Settlementtxid(wtxNew.GetHash().ToString());
    CWalletDB walletdb(pwalletMain->strWalletFile);
    wtx.WriteToDisk(&walletdb);
    return TLEMENTSTATE::SUCCESS;
}

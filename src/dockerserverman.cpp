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
CDockerServerman dockerServerman;
bool DockerCreateService::Sign(const CKey& keyClusterAddress, const CPubKey& pubKeyClusterAddress)
{
    std::string strError;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = txid.ToString() + boost::lexical_cast<std::string>(version) + n2n_community +
        serviceName + image + ssh_pubkey + item.ToString() + boost::lexical_cast<std::string>(sigTime);

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
        serviceName + image + ssh_pubkey + item.ToString() + boost::lexical_cast<std::string>(sigTime);

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
        
        LogPrint("docker","CDockerServerman::ProcessMessage GETDNDATA Started\n");
        if (!fMasterNode) return;
        DockerGetData mdndata;
        CPubKey pubkey;
        vRecv >> pubkey;
        LogPrint("docker", "CDockerServerman::ProcessMessage GETDNDATA -- pubkey =%s\n", pubkey.ToString().substr(0,66));

        mdndata.mapDockerServiceLists.clear();

        // dockerservicefilter serfilter;
        // serfilter.label.push_back("com.massgrid.pubkey="+pubkey.ToString().substr(0,66));
        // if(!dockerman.PushMessage(Method::METHOD_SERVICES_LISTS,"",serfilter.ToJsonString(),false)){
        //     LogPrint("docker","CDockerServerman::ProcessMessage GETDNDATA service list failed -- pubkey =%s\n", pubkey.ToString().substr(0,66));
        // }
        mdndata.mapDockerServiceLists = dockerman.GetServiceFromPubkey(pubkey);
        mdndata.version = DOCKERREQUEST_API_VERSION;
        mdndata.sigTime = GetAdjustedTime();
        mdndata.items = dockerman.GetPriceListFromNodelist();
        mdndata.masternodeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString();
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
         
        dockercluster.dndata = mdndata;
        LogPrint("docker","CDockerServerman::ProcessMessage DNDATA mapDockerServiceLists.size() %d sigTime:%d\n",mdndata.mapDockerServiceLists.size(),mdndata.sigTime);
        std::map<std::string,Service>::iterator iter = mdndata.mapDockerServiceLists.begin();
        for(;iter != mdndata.mapDockerServiceLists.end();iter++){
            if(iter->second.mapDockerTaskLists.size()){
                LogPrintf("CDockerServerman::ProcessMessage DNDATA mapDockerTaskLists.size()%d serviceid %s\n",iter->second.mapDockerTaskLists.size(),iter->first);
            }
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
                wtx.Setmasternodeaddress(CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).ToString());

                CWalletDB walletdb(pwalletMain->strWalletFile);
                wtx.WriteToDisk(&walletdb);
            }

        }
        
        setDNDataStatus(DNDATASTATUS::Received);

    }else if(strCommand == NetMsgType::CREATESERVICE){
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE Started\n");
        if (!fMasterNode) return;
        DockerCreateService createService;
        DockerGetData mdndata;
        vRecv >> createService;
        LogPrint("docker","CDockerServerman::ProcessMessage CREATESERVICE createService hash %s\n",createService.ToString());
        if(createService.version < DOCKERREQUEST_API_MINSUPPORT_VERSION || createService.version > DOCKERREQUEST_API_MAXSUPPORT_VERSION){
            LogPrintf("CDockerServerman::ProcessMessage --createService version %d not support need [%d - %d]\n", createService.version,DOCKERREQUEST_API_MINSUPPORT_VERSION,DOCKERREQUEST_API_MAXSUPPORT_VERSION);
            mdndata.strErr = "Error service version is: "+ std::to_string(DOCKERREQUEST_API_VERSION);
        }else{
            mdndata.version = DOCKERREQUEST_API_VERSION;
            mdndata.sigTime = GetAdjustedTime();
            if(CheckAndCreateServiveSpec(createService,mdndata.strErr)){
                mdndata.mapDockerServiceLists.clear();
                mdndata.mapDockerServiceLists = dockerman.GetServiceFromPubkey(createService.pubKeyClusterAddress);
            }
        }
            
        LogPrintf("CDockerServerman::ProcessMessage -- CREATESERVICE Sent DNDATA to peer %d\n", pfrom->id);
        connman.PushMessage(pfrom, NetMsgType::DNDATA, mdndata);
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
        if(delService.sigTime > GetAdjustedTime() + TIMEOUT && delService.sigTime < GetAdjustedTime() - TIMEOUT){
            LogPrintf("CDockerServerman::ProcessMessage DELETESERVICE sigTime is out of vaild timespan = %d\n",delService.sigTime);
            return;
        }
        if(!delService.CheckSignature(delService.pubKeyClusterAddress)){
            LogPrintf("CDockerServerman::ProcessMessage DELETESERVICE --CheckSignature Failed pubkey= %s\n",delService.pubKeyClusterAddress.ToString().substr(0,66));
            return;
        }
        Service svi;
        if(dockerman.GetServiceFromTxId(delService.txid,svi))
            dockerman.PushMessage(Method::METHOD_SERVICES_DELETE,svi.ID,"");
        timerModule.UpdateSet(delService.txid);
    }
}
bool CDockerServerman::CheckAndCreateServiveSpec(DockerCreateService createService, string& strErr){

    LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec Started\n");
    // 1.first check time
    if(createService.sigTime > GetAdjustedTime() + TIMEOUT && createService.sigTime < GetAdjustedTime() - TIMEOUT){
        strErr = "sigTime Error";
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec %s\n",strErr);
        return false;
    }

    //  2. checkSignature
    if(!createService.CheckSignature(createService.pubKeyClusterAddress)){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- CheckSignature() failed\n");
        return false;
    }
    //  3.check transactions
    if (!pwalletMain->mapWallet.count(createService.txid)){
        strErr = "Invalid or non-wallet transaction: "+createService.txid.ToString();
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec %s\n",strErr);
        return false;
    }
    
    //check transaction prescript pubkey
    CTransaction tx;
    uint256 hash;
    if(!CheckTransactionInputScriptPubkey(createService.txid, tx,createService.pubKeyClusterAddress, Params().GetConsensus(), hash,strErr, true)){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec %s\n",strErr);
        return false;
    }

    CWalletTx& wtx = pwalletMain->mapWallet[createService.txid];  //watch only not check

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx.GetHash());
    int confirms = wtx.GetDepthInMainChain(false);
    LogPrint("docker","current transaction fLocked %d confirms %d\n",fLocked,confirms);
    if(!fLocked && confirms < 1){
        strErr = "The transaction not confirms: "+std::to_string(confirms);
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec %s\n",strErr);
        return false;
    }
    if(wtx.HasCreatedService()){
        strErr = "The transaction has been used";
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec current %s\n",strErr);
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
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- serviceItem not found\n");
        strErr = "spec not found";
        return false;
    }
    if(!usablemapnode[serviceItem].count){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- serviceItem no resource\n");
        strErr = "no available resources";
        return false;
    }

    //  5. check item
    CAmount price = usablemapnode[serviceItem].price;
    if(payment < price){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec -- serviceItem payment not enough\n");
        strErr = "payment not enough";
        return false;
    }


    if(!(serviceItem.gpu.Count > 0 && serviceItem.gpu.Count <= DOCKER_MAX_GPU_NUM)){
        strErr = "GPU count error MAX_COUNT: "+std::to_string(DOCKER_MAX_GPU_NUM);
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec gpu %d,  DOCKER_MAX_GPU_NUM,%d\n",serviceItem.gpu.Count,DOCKER_MAX_GPU_NUM);
        return false;
    }
    if(!(serviceItem.cpu.Count > 0 && serviceItem.cpu.Count <= DOCKER_MAX_CPU_NUM)){
        strErr = "CPU count error MAX_COUNT: "+std::to_string(DOCKER_MAX_CPU_NUM);
        LogPrint("docker","CDockerServerman::CheckAndCreateServiveSpec cpu %d,  DOCKER_MAX_CPU_NUM,%d\n",serviceItem.cpu.Count,DOCKER_MAX_CPU_NUM);
        return false;
    }
    if(!(serviceItem.mem.Count > 0 && serviceItem.mem.Count <= DOCKER_MAX_MEMORY_BYTE)){
        strErr = "memory size error MAX_COUNT: "+std::to_string(DOCKER_MAX_MEMORY_BYTE);
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
    
    std::string ipaddr = dockerman.serviceIp.GetFreeIP();
    spec.taskTemplate.containerSpec.env.push_back("N2N_SERVERIP=" + ipaddr);
    spec.taskTemplate.containerSpec.env.push_back("N2N_SNIP=" + dockerman.GetMasterIp() + ":" + boost::lexical_cast<std::string>(dockerman.n2nServerPort));
    spec.taskTemplate.containerSpec.env.push_back("SSH_PUBKEY=" + createService.ssh_pubkey);
    spec.taskTemplate.containerSpec.env.push_back("CPUNAME=" + serviceItem.cpu.Name);
    spec.taskTemplate.containerSpec.env.push_back("CPUCOUNT=" + std::to_string(serviceItem.cpu.Count));
    spec.taskTemplate.containerSpec.env.push_back("MEMNAME=" + serviceItem.mem.Name);
    spec.taskTemplate.containerSpec.env.push_back("MEMCOUNT=" + std::to_string(serviceItem.mem.Count));
    spec.taskTemplate.containerSpec.env.push_back("GPUNAME=" + serviceItem.gpu.Name);
    spec.taskTemplate.containerSpec.env.push_back("GPUCOUNT=" + std::to_string(serviceItem.gpu.Count));

    
    spec.taskTemplate.placement.constraints.push_back("node.role == worker");
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.cpuname == "+serviceItem.cpu.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.cpucount == "+std::to_string(serviceItem.cpu.Count)));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.memname == "+serviceItem.mem.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.memcount == "+std::to_string(serviceItem.mem.Count)));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.gpuname == "+serviceItem.gpu.Name));
    spec.taskTemplate.placement.constraints.push_back(std::string("engine.labels.gpucount == "+std::to_string(serviceItem.gpu.Count)));
    Config::Mount mount;
    mount.target="/dev/net";
    mount.source="/dev/net";
    spec.taskTemplate.containerSpec.mounts.push_back(mount);

    //  7.request docker
    if(!dockerman.PushMessage(Method::METHOD_SERVICES_CREATE,"",spec.ToJsonString())){
        dockerman.serviceIp.Erase(ipaddr);
        return false;
    }
    return true;

}
bool CDockerServerman::SetTlementServiceWithoutDelete(uint256 serviceTxid){

    CWalletTx wtxNew;
    bool fnoCreated;
    vector<CRecipient> vecSend;
    COutPoint outpoint;
    CMassGridAddress masternodeAddress = CMassGridAddress(pwalletMain->vchDefaultKey.GetID());
    CScript masternodescriptPubKey = GetScriptForDestination(masternodeAddress.Get());
    CMassGridAddress customerAddress = masternodeAddress;
    CMassGridAddress providerAddress = masternodeAddress;
    CScript customerscriptPubKey = masternodescriptPubKey;
    CScript providerscriptPubKey = masternodescriptPubKey;

    if (!pwalletMain->mapWallet.count(serviceTxid)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete Invalid or non-wallet transaction id\n");
        return false;
    }
    CWalletTx& wtx = pwalletMain->mapWallet[serviceTxid];  //watch only not check

    if(wtx.HasTlemented()){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete has been tlementtxid\n");
        return false;
    }

    if(!wtx.GetOutPoint(masternodescriptPubKey,outpoint)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete outpoint not found\n");
        return false;
    }
    Coin coin;
    if(!GetUTXOCoin(outpoint,coin)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete outpoint not UTXO\n");
        return false;
    }
    
    if(pwalletMain->IsSpent(outpoint.hash,outpoint.n)){
        LogPrintf("CDockerServerman::CheckAndCreateServiveSpec IsSpent\n");
        return false;
    }

    isminefilter filter = ISMINE_SPENDABLE;
    CAmount nCredit = wtx.GetCredit(filter);
    CAmount nDebit = wtx.GetDebit(filter);
    CAmount nNet = nCredit - nDebit;
    CAmount nFee = (wtx.IsFromMe(filter) ? wtx.GetValueOut() - nDebit : 0);
    CAmount payment = nNet - nFee;
    const CAmount pay = payment;

    if(!wtx.HasCreatedService()){
        LogPrint("docker","CDockerServerman::SetTlementServiceWithoutDelete current transaction not been used\n");

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
        LogPrint("docker","CDockerServerman::SetTlementServiceWithoutDelete current transaction has been used\n");
        if(wtx.Getdeletetime().empty()){
            LogPrint("docker","CDockerServerman::SetTlementServiceWithoutDelete current transaction deletetime invaild\n");
            return false;
        }

        customerAddress = CMassGridAddress(wtx.Getcusteraddress());
        providerAddress = CMassGridAddress(wtx.Getprovideraddress());
        double feeRate = boost::lexical_cast<double>(wtx.Getfeerate());
        int64_t createTime = boost::lexical_cast<int64_t>(wtx.Getcreatetime());
        int64_t deleteTime = boost::lexical_cast<int64_t>(wtx.Getdeletetime());
        CAmount price = boost::lexical_cast<CAmount>(wtx.Getprice());
        fnoCreated = boost::lexical_cast<int>(wtx.Gettaskstate()) < Config::TASKSTATE_RUNNING;
        

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
        if(payrate < 0){
            LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete payrate %lf < 0\n",payrate);
            return false;
        }

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
            payment -= masternodeSend;  //compute fee

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
            if(masternodeSend + providerSend + customerSend != pay){
                LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete payment error masternodeSend %lld providerSend %lld customerSend %lld sumpayment %lld\n",masternodeSend, providerSend , customerSend ,pay);
                return false;
            }
            LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete masternodeSend %lld providerSend %lld customerSend %lld sumpayment %lld feerate %lf\n",masternodeSend, providerSend , customerSend ,payment,feeRate);
            if(masternodeSend > CAmount(1000)){
                CRecipient masternoderecipient = {masternodescriptPubKey, masternodeSend, false};
                vecSend.push_back(masternoderecipient);
            }
            for(auto it = vecSend.rbegin();it!= vecSend.rend();++it){
                if(it->nAmount >= CENT){
                    it->fSubtractFeeFromAmount =true;
                    break;
                }
            }       
        }
    }
    if (!vecSend.size()){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete vecSend.size() == 0 \n");
        return false;
    }
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
        return false;
    }

    //commit to wtx;
    wtxNew.Settlementtxid(wtxNew.GetHash().ToString());
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey, g_connman.get(),NetMsgType::TXLOCKREQUEST)){
        LogPrintf("CDockerServerman::SetTlementServiceWithoutDelete CommitTransaction error \n");
        return false;
    }
    return true;
}

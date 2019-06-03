// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "base58.h"
#include "init.h"
#include "netbase.h"
#include "validation.h"
#include "coincontrol.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "dockerpriceconfig.h"
#include "masternodeman.h"
#include "dockerman.h"
#include "dockerserverman.h"
#include "dockercluster.h"
#include "messagesigner.h"
#include "rpc/server.h"
#include "util.h"
#include "utilmoneystr.h"
#include "init.h"
#include "../wallet/wallet.h"
#include "dockercluster.h"
#include "dockeredge.h"
#include "timermodule.h"

#include <fstream>
#include <iomanip>
#include <univalue.h>
#define ENABLE_WALLET

UniValue docker(const UniValue& params, bool fHelp)
{
    std::string strCommand;
    if (params.size() >= 1) {
        strCommand = params[0].get_str();
    }

#ifdef ENABLE_WALLET
    if (strCommand == "start-many")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "DEPRECATED, please use start-all instead");
#endif // ENABLE_WALLET

    if (fHelp  ||
        (
#ifdef ENABLE_WALLET
            strCommand != "create" && strCommand != "delete"  && strCommand != "sendtomasternode" &&
#endif // ENABLE_WALLET
            strCommand != "connect" && strCommand != "disconnect" && strCommand != "getdndata" && strCommand != "gettransaction" && strCommand != "listprice" && strCommand != "listuntlementtx" && strCommand != "settlement" && strCommand != "setprice"&& strCommand != "setdockerfee"))
            throw std::runtime_error(
                "docker \"command\"...\n"
                "Set of commands to execute docker related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
                "   getdndata      - - get infomation from dockernode Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)  Print number of all of yours docker services\n"
                
                "   connect        - Connect to docker network\n"
                "       1. \"localAddress (IP)\" (string, required)\n"
                "       2. \"netmask (netmask)\" (string, required)\n"
                "       3. \"snAddress (IP:Port)\" (string, required)\n"

                "   disconnect     - Disconnect to docker network\n"
#ifdef ENABLE_WALLET
                "   create         - create a docker service Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)\n"
                "       2. \"service name\" (string, required)\n"
                "       3. \"Image\"(string, required)\n"
                "       4. \"CPU name\"(string, required)\n"
                "       5. \"CPU thread count\"(int, required)\n"
                "       6. \"Memory name\"(string, required)\n"
                "       7. \"Memory GByte\"(int, required)\n"
                "       8. \"GPU Kind\"(string, required)\n"
                "       9. \"GPU count\"(int, required)\n"
                "       10. \"NetWork Community\"(string, required)\n"
                "       11. \"SSH_PUBKEY\"(string, required)\n"
                "       12. \"environment\"(string,string , optional)\n"
                "       13. {\n"
                "               \"ENV1\": \"value1\"   (string,string, optional)"
                "               \"ENV2\": \"value2\"   (string,string, optional)"
                "               ...\n"
                "           }\n"
                "       14. \"persistentStore\"(bool, optional)\n"
                "   delete         - delete a docker service Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)\n"
                "       2. \"txid\" (string, required)\n"
                "sendtomasternode  - send to masternode address: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)\n"
                "       2. \"MassGrid address\"(string, required)\n"
                "       3. \"amount\"(int, required)\n"
                "dockernode command:  "
                "settlement        - set tlement a txid\n"
                "       1. \"txid\" (string, required)\n"
                "setpersistentstore        - set tlement a txid\n"
                "       1. \"status\" (bool, required)\n"
                "listuntlementtx   - list all untlement transactions\n"
                "listprice         - list all service items price\n"
                "setprice          - set item price\n"
                "       1. \"type\" (string, required)\n"
                "       2. \"name\"(string, required)\n"
                "       3. \"price\"(double, required)\n"
                "setdockerfee      - set docker masternode fee (0.01):\n"
                "       1. \"price\" (percent(double), required)\n"
                + HelpExampleCli("docker", "create \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\" \"MassGrid\" \"massgrid/10.0-base-ubuntu16.04\" intel_i3 1 ddr 1 \"nvidia_p104_100_4g\" 1 \"massgridn2n\" \"ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDPEBGcs6VnDI89aVZHBCoDVq57qh7WamwXW4IbaIMWPeYIXQGAaYt83tCmJAcVggM176KELueh7+d1VraYDAJff9V5CxVoMhdJf1AmcIHGCyEjHRf12+Lme6zNVa95fI0h2tsryoYt1GAwshM6K1jUyBBWeVUdITAXGmtwco4k12QcDhqkfMlYD1afKjcivwaXVawaopdNqUVY7+0Do5ct4S4DDbx6Ka3ow71KyZMh2HpahdI9XgtzE3kTvIcena9GwtzjN+bf0+a8+88H6mtSyvKVDXghbGjunj55SaHZEwj+Cyv6Q/3EcZvW8q0jVuJu2AAQDm7zjgUfPF1Fwdv/ MassGrid\" \"{\\\"ENV1\\\":\\\"value1\\\"}\" false")
                + HelpExampleCli("docker", "delete \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\"")
                + HelpExampleCli("docker", "sendtomasternode \"119.3.66.159:19443\" \"mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T\" 6.4")
                + HelpExampleCli("docker", "settlement \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\"")
                + HelpExampleCli("docker", "setpersistentstore \"true\"")
                + HelpExampleCli("docker", "listuntlementtx")
                + HelpExampleCli("docker", "listprice")
                + HelpExampleCli("docker", "setprice \"cpu intel_i3 0.8\"")
                + HelpExampleCli("docker", "setdockerfee 0.01")
#endif // ENABLE_WALLET
                + HelpExampleCli("docker", "getdndata \"119.3.66.159:19443\"")
                + HelpExampleCli("docker", "gettransaction \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\"")
                + HelpExampleCli("docker", "connect \"10.1.1.4\" \"240.0.0.0\" \"119.3.66.159\"")
                + HelpExampleCli("docker", "disconnect")
                );


    
    if (strCommand == "connect")
    {
        if (params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");

        std::string strLocalAddr = params[1].get_str();

        std::string netmask = params[2].get_str();

        std::string strSnAddr = params[3].get_str();

        if(ThreadEdgeStart("massgridn2n",strLocalAddr,netmask,strSnAddr)){
            return "edge Start Successfully";
        }
        else
            return "edge Start Failed";
    }

    if (strCommand == "disconnect"){
        if (params.size() != 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");
        ThreadEdgeStop();
        return "edge Stop Successfully";
    }

#ifdef ENABLE_WALLET
    if(strCommand == "create")
    {
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        if (params.size() < 13)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        std::string strAddr = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strAddr))
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Invalid IP");
        if(!dockercluster.ProcessDockernodeConnections())
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Connect to Masternode failed");

        DockerCreateService createService{};

        createService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
        createService.txid = uint256S(params[2].get_str());
        
        std::string strServiceName = params[3].get_str();
        createService.serviceName = strServiceName;

        std::string strServiceImage = params[4].get_str();
        createService.image = strServiceImage;

        createService.item.cpu.Name = params[5].get_str();
        int64_t serviceCpu = params[6].get_int64();
        createService.item.cpu.Count = serviceCpu;

        createService.item.mem.Name = params[7].get_str();
        int64_t serviceMemoey_byte = params[8].get_int64();
        createService.item.mem.Count = serviceMemoey_byte;
        
        std::string strServiceGpuName = params[9].get_str();
        createService.item.gpu.Name = strServiceGpuName;

        int64_t serviceGpu = params[10].get_int64();
        createService.item.gpu.Count = serviceGpu;

        std::string strn2n_Community = params[11].get_str();
        createService.n2n_community = strn2n_Community;

        std::string strssh_pubkey = params[12].get_str();
        createService.ssh_pubkey = strssh_pubkey;

        UniValue envs = params[13].get_obj();
        std::vector<std::string> vKeys = envs.getKeys();
        for (unsigned int idx = 0; idx < vKeys.size(); idx++) {
            if(!envs[vKeys[idx]].isStr())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected object");
            createService.env[vKeys[idx]] = envs[vKeys[idx]].get_str();
        }
        if(params.size() >= 15){
            createService.fPersistentStore = params[14].get_bool();
        }
        EnsureWalletIsUnlocked();
        if(!dockercluster.CreateAndSendSeriveSpec(createService))
            return "CreateSpec Error Failed";

        for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                return strServiceCode[dockercluster.dndata.errCode];
            }
        }
        return NullUniValue;
    }
    if(strCommand == "delete"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        if (params.size() != 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        std::string strAddr = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strAddr))
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Invalid IP");
        if(!dockercluster.ProcessDockernodeConnections())
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Connect to Masternode failed");
        
        DockerDeleteService delService{};

        delService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
        
        std::string strtxid = params[2].get_str();
        delService.txid = uint256S(strtxid);
        EnsureWalletIsUnlocked();
        CKey vchSecret;
        if (!pwalletMain->GetKey(delService.pubKeyClusterAddress.GetID(), vchSecret)){
            return "delService Error not found privkey";
        }
        if(!delService.Sign(vchSecret,delService.pubKeyClusterAddress)){
            return "delService Sign Error";
        }
        g_connman->PushMessage(dockercluster.connectNode, NetMsgType::DELETESERVICE, delService);

        for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                return strServiceCode[dockercluster.dndata.errCode];
            }
        }
        return NullUniValue;
    }
    if(strCommand == "listuntlementtx"){
        if (!fDockerNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a dockernode");
        UniValue varr(UniValue::VARR);
        std::set<CWalletTx*> setWallet = timerModule.GetWalletTxSet();
        for(const auto& tx :setWallet)
            varr.push_back(tx->GetHash().ToString());
        return varr;
    }
    if(strCommand == "settlement"){
        if (!fDockerNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a dockernode");
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        std::string strtxid = params[1].get_str();
        uint256 txid = uint256S(strtxid);
        if (!pwalletMain->mapWallet.count(txid)){
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
        }
        CWalletTx& wtx = pwalletMain->mapWallet[txid];  //watch only not check
        timerModule.UpdateSet(wtx);
        return "insert successful ";
    }
    if(strCommand == "setpersistentstore"){
        if (!fDockerNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a dockernode");
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        bool fps = params[1].get_bool();
        dockerman.fPersistentStore = fps;
        return "PersistentStore has " + fps?"enable":"disable";
    }
    if(strCommand == "listprice"){
        if (!fDockerNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a dockernode");
        UniValue varr(UniValue::VARR);
        auto entries = dockerPriceConfig.getEntries();
        int entriesNum=1;
        for(auto it = entries.begin();it!= entries.end();++it){
        UniValue obj(UniValue::VOBJ);
        std::ostringstream streamFull;
        streamFull << it->getType() << " " <<
                        it->getName() << " " <<
                        (double)it->getPrice()/COIN;
        std::string strFull = streamFull.str();
        obj.push_back(Pair(std::to_string(entriesNum++),strFull));
        varr.push_back(obj);
        }
        return varr;
    }
    if(strCommand == "setprice"){
        if (!fDockerNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a dockernode");
        if (params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        std::string type = params[1].get_str();
        std::string name = params[2].get_str();
        std::string strPrice = params[3].get_str();
        auto entries = dockerPriceConfig.getEntries();
        for(auto it = entries.begin();it!= entries.end();++it){
            if(type == it->getType() && name == it->getName()){
                it->setPrice(strPrice);           
                dockerPriceConfig.writeSave();    
                return "modified " + it->getType() + " " +it->getName() + " " +std::to_string(it->getPrice());
            }
        }
        dockerPriceConfig.add(type,name,strPrice);
        dockerPriceConfig.writeSave();
        return "add new "+type + " " +name + " " +strPrice;
    }    
    if (strCommand == "setdockerfee"){
        if (!fDockerNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a dockernode");
        if (params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify an fee rate");
        std::string strfeeRate = params[1].get_str();
        double fee;
        try
        {
            fee = boost::lexical_cast<double>(strfeeRate);
        }
        catch(const boost::bad_lexical_cast &)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "bad_lexical_cast");
        }
        if(fee >=1 || fee <0)
            return "docker feerate set failed feeRate is :"+std::to_string(fee); 
        dockerServerman.feeRate = fee;
        return "docker feerate is" + std::to_string(dockerServerman.feeRate);
    }
    if(strCommand == "sendtomasternode"){
        LOCK2(cs_main, pwalletMain->cs_wallet);
        std::string strAddr = params[1].get_str();
        std::string mnOutPoint;
        std::string mnOutPointformat;
        std::map<COutPoint, CMasternode> mapMasternodes = mnodeman.GetFullMasternodeMap();
        for(auto it = mapMasternodes.begin();it!=mapMasternodes.end();++it){
            if(it->second.addr.ToString() == strAddr){
                mnOutPoint = it->first.ToStringShort();
                mnOutPointformat = strprintf("%s%08x", it->first.hash.ToString().substr(0,64), it->first.n);
                break;
            }
        }
        CMassGridAddress address(params[2].get_str());
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid MassGrid address");

        // Amount
        CAmount nAmount = AmountFromValue(params[3]);
        if (nAmount <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");

        // Wallet comments
        CWalletTx wtxNew;
        if(mnOutPoint.empty()){
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "MassGrid masternode outpoint not found");
        }
        wtxNew.Setmasternodeoutpoint(mnOutPoint);
        wtxNew.Setmasternodeip(strAddr);
        wtxNew.Setmasternodeaddress(params[2].get_str());
        wtxNew.Setorderstatus("0");
        EnsureWalletIsUnlocked();

        CAmount curBalance = pwalletMain->GetBalance();

        // Check amount

        if (nAmount > curBalance)
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

        if (pwalletMain->GetBroadcastTransactions() && !g_connman)
            throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

        // Parse MassGrid address
        CScript scriptPubKey = GetScriptForDestination(address.Get());

        // Create and send the transaction
        CReserveKey reservekey(pwalletMain);
        CAmount nFeeRequired;
        std::string strError;
        vector<CRecipient> vecSend;
        int nChangePosRet = -1;
        CRecipient recipient = {scriptPubKey, nAmount, false};
        vecSend.push_back(recipient);

        //op_return
        std::vector<unsigned char> pch = {0x6d, 0x67, 0x64};
        pch.push_back(0x11);
        std::vector<unsigned char> vchPayload = ParseHex(
        "00000000"+mnOutPointformat);
        pch.insert(pch.end(),vchPayload.begin(),vchPayload.end());
        CScript scriptMsg = CScript() << OP_RETURN << pch;
        CRecipient recipient2 = {scriptMsg, CAmount(0), false};
        vecSend.push_back(recipient2);
        CCoinControl coinControl;
        coinControl.fUseInstantSend = true;
        coinControl.destChange = CMassGridAddress(pwalletMain->vchDefaultKey.GetID()).Get();

        if (!pwalletMain->CreateTransaction(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet,
                                            strError, &coinControl, true,ALL_COINS, true,true,mnOutPointformat)) {
            if (nAmount + nFeeRequired > pwalletMain->GetBalance())
                strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
            throw JSONRPCError(RPC_WALLET_ERROR, strError);
        }
        if (!pwalletMain->CommitTransaction(wtxNew, reservekey, g_connman.get(), NetMsgType::TXLOCKREQUEST))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");


        return wtxNew.GetHash().GetHex();
    }
#endif // ENABLE_WALLET
    
    if (strCommand == "getdndata"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");
        std::string strIPPort;
        strIPPort = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strIPPort)){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "SetConnectDockerAddress error");
        }
        if(!dockercluster.ProcessDockernodeConnections()){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "ProcessDockernodeConnections error");
        }
        dockercluster.AskForDNData();

        UniValue varr(UniValue::VARR);
        for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                for(auto it = dockercluster.dndata.mapDockerServiceLists.begin();it != dockercluster.dndata.mapDockerServiceLists.end();++it){
                    UniValue obj(UniValue::VOBJ);
                    obj.push_back(Pair(it->first,Service::DockerServToJson(it->second)));
                    varr.push_back(obj);
                }
                UniValue varr2(UniValue::VARR);
                for(auto it = dockercluster.dndata.items.begin();it!= dockercluster.dndata.items.end();++it){
                    UniValue obj(UniValue::VOBJ);
                    obj.push_back(Pair("CpuType",it->first.cpu.Name));
                    obj.push_back(Pair("CpuCount",it->first.cpu.Count));
                    obj.push_back(Pair("MemSize",it->first.mem.Count));
                    obj.push_back(Pair("GpuType",it->first.gpu.Name));
                    obj.push_back(Pair("GpuCount",it->first.gpu.Count));
                    obj.push_back(Pair("Price",(double)it->second.price/COIN));
                    obj.push_back(Pair("UsageCount",it->second.count));
                    varr2.push_back(obj);
                }
                varr.push_back(varr2);
                UniValue obj(UniValue::VOBJ);
                obj.push_back(Pair("masternodeaddress",dockercluster.dndata.masternodeAddress));
                varr.push_back(obj);
                return varr;
            }
        }
    }
    if (strCommand == "gettransaction"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        
        if (params.size() != 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");
        std::string strIPPort;
        strIPPort = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddr(strIPPort)){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "SetConnectDockerAddress error");
        }
        if(!dockercluster.ProcessDockernodeConnections()){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "ProcessDockernodeConnections error");
        }
        std::string txid = params[2].get_str();
        dockercluster.AskForTransData(txid);

        UniValue varr(UniValue::VARR);
        for(int i=0;i<10;++i){
            MilliSleep(300);
            if(dockercluster.dtdata.msgStatus !=TASKDTDATA::DEFAULT){
                UniValue obj(UniValue::VOBJ);
                if(dockercluster.dtdata.msgStatus == TASKDTDATA::ERRORCODE){
                    obj.push_back(Pair("errMessage",dockercluster.dtdata.errCode));
                    return obj;
                }
                obj.push_back(Pair("txid",dockercluster.dtdata.txid.ToString()));
                obj.push_back(Pair("deleteTime",dockercluster.dtdata.deleteTime));
                obj.push_back(Pair("feeRate",dockercluster.dtdata.feeRate));
                obj.push_back(Pair("taskStatus",dockercluster.dtdata.errCode));
                obj.push_back(Pair("taskMessage",dockercluster.dtdata.taskStatus));
                obj.push_back(Pair("tlementtxid",dockercluster.dtdata.tlementtxid.ToString()));
                return obj;
            }
        }
    }
    return NullUniValue;
}

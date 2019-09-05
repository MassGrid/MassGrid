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
            strCommand != "create" && strCommand != "update"  && strCommand != "delete"  && strCommand != "sendtomasternode" &&
#endif // ENABLE_WALLET
            strCommand != "connect" && strCommand != "disconnect" && strCommand != "getdndata" && strCommand != "gettransaction" && strCommand != "listprice" && 
            strCommand != "getservice" && strCommand != "getservices" && strCommand != "setprice"&& strCommand != "setdockerfee" && strCommand != "setpersistentstore"))
            throw std::runtime_error(
                "docker \"command\"...\n"
                "Set of commands to execute docker related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
                "   getdndata      - - get infomation from dockernode Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)  Print number of all of yours docker services\n"
                "   getservice      - - get infomation from dockernode Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)  Print number of all of yours docker services\n"
                "       2. \"COutPoint hash (string, required)\n"
                "       3. \"COutPoint n (int,required)\n"
                "   getservices      - - get infomation from dockernode Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)  Print number of all of yours docker services\n"
                "       2. \"start \" (int64, required)  find start position\n"
                "       3. \"count \" (int64, required)  number of items\n"
                "       4. \"full \" (int64, required)  find by full mode\n"
                "   connect        - Connect to docker network\n"
                "       1. \"localAddress (IP)\" (string, required)\n"
                "       2. \"snAddress (IP:Port)\" (string, required)\n"

                "   disconnect     - Disconnect to docker network\n"
#ifdef ENABLE_WALLET
                "   create         - create a docker service Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)\n"
                "       2. \"COutPoint hash (string, required)\n"
                "       3. \"COutPoint n (int,required)\n"
                "       4. \"service name\" (string, required)\n"
                "       5. \"Image\"(string, required)\n"
                "       6. \"SSH_PUBKEY\"(string, required)\n"
                "       7. \"CPU name\"(string, required)\n"
                "       8. \"CPU thread count\"(int, required)\n"
                "       9. \"Memory name\"(string, required)\n"
                "       10. \"Memory GByte\"(int, required)\n"
                "       11. \"GPU Kind\"(string, required)\n"
                "       12. \"GPU count\"(int, required)\n"
                "       13. \"persistentStore\"(bool, optional)\n"
                "       14. \"environment\"(string,string , optional)\n"
                "           {\n"
                "               \"ENV1\": \"value1\"   (string,string, optional)"
                "               \"ENV2\": \"value2\"   (string,string, optional)"
                "               ...\n"
                "           }\n"
                "   update         - update a docker service Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)\n"
                "       2. \"COutPoint create service outpoint hash (string, required)\n"
                "       3. \"COutPoint create service outpoint n (int,required)\n"   
                "       4. \"COutPoint the update outpoint hash (string, required)\n"
                "       5. \"COutPoint the update outpoint n (int,required)\n"                
                "   delete         - delete a docker service Arguments: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)\n"
                "       2. \"COutPoint hash (string, required)\n"
                "       3. \"COutPoint n (int,required)\n"
                "sendtomasternode  - send to masternode address: \n"
                "       1. \"dockernode (IP:Port)\" (string, required)\n"
                "       2. \"MassGrid address\"(string, required)\n"
                "       3. \"amount\"(int, required)\n"
                "dockernode command:  "
                "setpersistentstore        - set tlement a txid\n"
                "       1. \"status\" (bool, optional)\n" 
                "listprice         - list all service items price\n"
                "setprice          - set item price\n"
                "       1. \"type\" (string, required)\n"
                "       2. \"name\"(string, required)\n"
                "       3. \"price\"(double, required)\n"
                "setdockerfee      - set docker masternode fee (0.01):\n"
                "       1. \"price\" (percent(double), required)\n"
                + HelpExampleCli("docker", "create \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\" 0 \"MassGrid\" \"massgrid/10.0-base-ubuntu16.04\" \"ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDPEBGcs6VnDI89aVZHBCoDVq57qh7WamwXW4IbaIMWPeYIXQGAaYt83tCmJAcVggM176KELueh7+d1VraYDAJff9V5CxVoMhdJf1AmcIHGCyEjHRf12+Lme6zNVa95fI0h2tsryoYt1GAwshM6K1jUyBBWeVUdITAXGmtwco4k12QcDhqkfMlYD1afKjcivwaXVawaopdNqUVY7+0Do5ct4S4DDbx6Ka3ow71KyZMh2HpahdI9XgtzE3kTvIcena9GwtzjN+bf0+a8+88H6mtSyvKVDXghbGjunj55SaHZEwj+Cyv6Q/3EcZvW8q0jVuJu2AAQDm7zjgUfPF1Fwdv/ MassGrid\" intel_i3 1 ddr 1 \"nvidia_p104_100_4g\" 1 false \"{\\\"ENV1\\\":\\\"value1\\\"}\"")
                + HelpExampleCli("docker", "update \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\" 0 \"ad5b29fdaa3fe32ff7d092e2fe7d083b1509c5a2ddbfa5ac332d7672d46c3620\" 1")
                + HelpExampleCli("docker", "delete \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\" 0")
                + HelpExampleCli("docker", "sendtomasternode \"119.3.66.159:19443\" \"mfb4XJGyaBwNK2Lf4a7r643U3JotRYNw2T\" 6.4")
                + HelpExampleCli("docker", "settlement \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\"")
                + HelpExampleCli("docker", "setpersistentstore \"1\"")
                + HelpExampleCli("docker", "listprice")
                + HelpExampleCli("docker", "setprice \"cpu intel_i3 0.8\"")
                + HelpExampleCli("docker", "setdockerfee 0.01")
#endif // ENABLE_WALLET
                + HelpExampleCli("docker", "getdndata \"119.3.66.159:19443\"")
                + HelpExampleCli("docker", "getservice \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\" 0")
                + HelpExampleCli("docker", "getservices \"119.3.66.159:19443\" 0 1 true")
                + HelpExampleCli("docker", "gettransaction \"119.3.66.159:19443\" \"b5f53c3e9884d23620f1c5b6f027a32e92d9c68a123ada86c55282acd326fde9\"")
                + HelpExampleCli("docker", "connect \"10.1.1.4\" \"119.3.66.159\"")
                + HelpExampleCli("docker", "disconnect")
                );


    
    if (strCommand == "connect")
    {
        if (params.size() != 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");

        std::string strLocalAddr = params[1].get_str();

        std::string netmask = "255.0.0.0";

        std::string strSnAddr = params[2].get_str();

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

        DockerCreateService dockerCreateService{};
        dockerCreateService.clusterServiceCreate.pubKeyClusterAddress = dockercluster.DefaultPubkey;
        dockerCreateService.clusterServiceCreate.OutPoint = COutPoint(uint256S(params[2].get_str()),params[3].get_int());

        std::string strServiceName = params[4].get_str();
        dockerCreateService.clusterServiceCreate.ServiceName = strServiceName;

        std::string strServiceImage = params[5].get_str();
        dockerCreateService.clusterServiceCreate.Image = strServiceImage;

        std::string strssh_pubkey = params[6].get_str();
        dockerCreateService.clusterServiceCreate.SSHPubkey = strssh_pubkey;

        dockerCreateService.clusterServiceCreate.hardware.CPUType = params[7].get_str();
        int64_t serviceCpu = params[8].get_int64();
        dockerCreateService.clusterServiceCreate.hardware.CPUThread= serviceCpu;

        dockerCreateService.clusterServiceCreate.hardware.MemoryType = params[9].get_str();
        int64_t serviceMemoey_byte = params[10].get_int64();
        dockerCreateService.clusterServiceCreate.hardware.MemoryCount = serviceMemoey_byte;

        std::string strServiceGpuName = params[11].get_str();
        dockerCreateService.clusterServiceCreate.hardware.GPUType = strServiceGpuName;

        int64_t serviceGpu = params[12].get_int64();
        dockerCreateService.clusterServiceCreate.hardware.GPUCount = serviceGpu;

        dockerCreateService.clusterServiceCreate.hardware.PersistentStore = params[13].get_str();
        

        if(params.size() > 14 ){
            UniValue envs = params[14].get_obj();
            std::vector<std::string> vKeys = envs.getKeys();
            for (unsigned int idx = 0; idx < vKeys.size(); idx++) {
                if(!envs[vKeys[idx]].isStr())
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected object");
                dockerCreateService.clusterServiceCreate.ENV[vKeys[idx]] = envs[vKeys[idx]].get_str();
            }
        }
        EnsureWalletIsUnlocked();
        if(!dockercluster.CreateAndSendSeriveSpec(dockerCreateService))
            return "CreateSpec Error Failed";

        for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                if (!dockercluster.vecServiceInfo.err.empty()){
                    return dockercluster.vecServiceInfo.err;
                }
                ServiceInfo serviceInfo{};
                for(auto &it :dockercluster.vecServiceInfo.servicesInfo){
                    if(it.second.CreateSpec.OutPoint == dockerCreateService.clusterServiceCreate.OutPoint){
                        serviceInfo = it.second;
                        dockercluster.saveServiceData(serviceInfo);
                        break;
                    }
                }
                UniValue obj(UniValue::VOBJ);
                obj.read(serviceInfo.jsonUniValue);
                return obj;
            }
        }
        return NullUniValue;
    }
    if(strCommand == "update"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        if (params.size() != 6)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        std::string strAddr = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strAddr))
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Invalid IP");
        if(!dockercluster.ProcessDockernodeConnections())
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Connect to Masternode failed");
        
        DockerUpdateService updateService{};
        updateService.clusterServiceUpdate.pubKeyClusterAddress = dockercluster.DefaultPubkey;
        updateService.clusterServiceUpdate.CrerateOutPoint = COutPoint(uint256S(params[2].get_str()),params[3].get_int());
        updateService.clusterServiceUpdate.OutPoint = COutPoint(uint256S(params[4].get_str()),boost::lexical_cast<int>(params[5].get_str())); 
        EnsureWalletIsUnlocked();
        if(!dockercluster.UpdateAndSendSeriveSpec(updateService))
            return "updateService Error Failed";
          for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                if (!dockercluster.vecServiceInfo.err.empty()){
                    return dockercluster.vecServiceInfo.err;
                }
                ServiceInfo serviceInfo{};
                for(auto &it :dockercluster.vecServiceInfo.servicesInfo){
                    if(it.second.CreateSpec.OutPoint == updateService.clusterServiceUpdate.CrerateOutPoint){
                        serviceInfo = it.second;
                        dockercluster.saveRerentServiceData(it.second.ServiceID,updateService);
                        break;
                    }
                }
                UniValue obj(UniValue::VOBJ);
                obj.read(serviceInfo.jsonUniValue);
                return obj;
            }
        }
        return NullUniValue;
    }
    if(strCommand == "delete"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        if (params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        std::string strAddr = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strAddr))
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Invalid IP");
        if(!dockercluster.ProcessDockernodeConnections())
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Connect to Masternode failed");
        
        DockerDeleteService delService{};

        delService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
        delService.CrerateOutPoint = COutPoint(uint256S(params[2].get_str()),params[3].get_int());
        EnsureWalletIsUnlocked();
        if(!dockercluster.DeleteAndSendServiceSpec(delService))
            return "delService Error Failed";
        for(int i=0;i<20;++i){
            MilliSleep(100);
             if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                if (!dockercluster.vecServiceInfo.err.empty()){
                    return dockercluster.vecServiceInfo.err;
                }
                if (!dockercluster.vecServiceInfo.msg.empty()){
                    return dockercluster.vecServiceInfo.msg;
                }
            }
        }
        return NullUniValue;
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
                entries.erase(it);
                dockerPriceConfig.add(type,name,strPrice);           
                dockerPriceConfig.writeSave();    
                return "modified " + type + " " +name + " " +strPrice;
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
                                            strError, &coinControl, true,ALL_COINS, true,false,mnOutPointformat)) {
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

        UniValue obj(UniValue::VOBJ);
        for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                UniValue varr2(UniValue::VARR);
                for(auto it = dockercluster.machines.items.begin();it!= dockercluster.machines.items.end();++it){
                    UniValue obj(UniValue::VOBJ);
                    obj.push_back(Pair("CPUType",it->first.cpu.Name));
                    obj.push_back(Pair("CPUThread",it->first.cpu.Count));
                    obj.push_back(Pair("MemorySize",it->first.mem.Count));
                    obj.push_back(Pair("GPUType",it->first.gpu.Name));
                    obj.push_back(Pair("GPUCount",it->first.gpu.Count));
                    obj.push_back(Pair("Price",(double)it->second.price/COIN));
                    obj.push_back(Pair("Quantity",it->second.count));
                    varr2.push_back(obj);
                }
                obj.pushKV("Machines",varr2);
                obj.push_back(Pair("Masternode_Beneficiary_Address",dockercluster.machines.masternodeAddress));
                obj.push_back(Pair("Token",dockercluster.machines.Token));
                obj.push_back(Pair("Total",dockercluster.machines.TotalCount));
                obj.push_back(Pair("Availability_Quantity",dockercluster.machines.AvailabilityCount));
                obj.push_back(Pair("Unused_Quantity",dockercluster.machines.UsableCount));
                obj.push_back(Pair("Error",dockercluster.machines.err));
                obj.push_back(obj);
                return obj;
            }
        }
    }
     if (strCommand == "getservice"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        
        if (params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");
        std::string strIPPort;
        strIPPort = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strIPPort)){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "SetConnectDockerAddress error");
        }
        if(!dockercluster.ProcessDockernodeConnections()){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "ProcessDockernodeConnections error");
        }

        COutPoint outpoint(uint256S(params[2].get_str()),params[3].get_int());
        dockercluster.AskForService(outpoint);
        for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                if (!dockercluster.vecServiceInfo.err.empty()){
                    return dockercluster.vecServiceInfo.err;
                }
                ServiceInfo serviceInfo{};
                for(auto &it :dockercluster.vecServiceInfo.servicesInfo){
                    if(it.second.CreateSpec.OutPoint == outpoint){
                        serviceInfo = it.second;
                        break;
                    }
                }
                UniValue obj(UniValue::VOBJ);
                obj.read(serviceInfo.jsonUniValue);
                return obj;
            }
        }
    }
    if (strCommand == "getservices"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        
        if (params.size() != 5)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");
        std::string strIPPort;
        strIPPort = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strIPPort)){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "SetConnectDockerAddress error");
        }
        if(!dockercluster.ProcessDockernodeConnections()){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "ProcessDockernodeConnections error");
        }
        int64_t start = boost::lexical_cast<int64_t>(params[2].get_str());
        int64_t count = params[3].get_int64();
        int64_t full =  boost::lexical_cast<int64_t>(params[4].get_str());
        dockercluster.AskForServices(start,count,full);

        UniValue varr(UniValue::VARR);
        for(int i=0;i<20;++i){
            MilliSleep(100);
            if(dockerServerman.getDNDataStatus() == CDockerServerman::DNDATASTATUS::Received){
                if (!dockercluster.vecServiceInfo.err.empty()){
                    return dockercluster.vecServiceInfo.err;
                }
                ServiceInfo serviceInfo{};
                for(auto &it :dockercluster.vecServiceInfo.servicesInfo){
                    UniValue obj(UniValue::VOBJ);
                    obj.read(it.second.jsonUniValue);
                    varr.push_back(obj);
                }
                return varr;
            }
        }
    }
    return NullUniValue;
}
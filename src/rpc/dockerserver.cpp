// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "base58.h"
#include "init.h"
#include "netbase.h"
#include "validation.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
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

#include <fstream>
#include <iomanip>
#include <univalue.h>

#ifdef ENABLE_WALLET
void EnsureWalletIsUnlocked();
#endif // ENABLE_WALLET

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
            strCommand != "create" &&
#endif // ENABLE_WALLET
            strCommand != "connect" && strCommand != "disconnect" && strCommand != "getdndata"))
            throw std::runtime_error(
                "docker \"command\"...\n"
                "Set of commands to execute docker related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
                "   getdndata    - \"dockernode (IP:Port)\" (string, required)  Print number of all of yours docker services\n"
                
                "   connect      - Connect to docker network\n"
                "   disconnect   - Disconnect to docker network\n"
#ifdef ENABLE_WALLET
                "   create       - create a docker service Arguments: \n"
                "                  \"dockernode (IP:Port)\" (string, required)\n"
                "                  \"service name\" (string, required)\n"
                "                  \"Image\"(string, required)\n"
                "                  \"CPU count\"(int, required)\n"
                "                  \"Memory Byte\"(int, required)\n"
                "                  \"GPU Kind\"(string, required)\n"
                "                  \"GPU count\"(int, required)\n"
                "                  \"NetWork Community\"(string, required)\n"
                "                  \"SSH_PUBKEY\"(string, required)\n"
                + HelpExampleCli("docker", "create \"119.3.66.159:19443\" \"MassGrid\" \"wany/cuda9.1-base\" 1000000000 1024000000 \"NVIDIA_GPUP104\" 1 \"massgridn2n\" \"ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDPEBGcs6VnDI89aVZHBCoDVq57qh7WamwXW4IbaIMWPeYIXQGAaYt83tCmJAcVggM176KELueh7+d1VraYDAJff9V5CxVoMhdJf1AmcIHGCyEjHRf12+Lme6zNVa95fI0h2tsryoYt1GAwshM6K1jUyBBWeVUdITAXGmtwco4k12QcDhqkfMlYD1afKjcivwaXVawaopdNqUVY7+0Do5ct4S4DDbx6Ka3ow71KyZMh2HpahdI9XgtzE3kTvIcena9GwtzjN+bf0+a8+88H6mtSyvKVDXghbGjunj55SaHZEwj+Cyv6Q/3EcZvW8q0jVuJu2AAQDm7zjgUfPF1Fwdv/ MassGrid\"")
#endif // ENABLE_WALLET
                + HelpExampleCli("docker", "getdndata \"119.3.66.159:19443\"")
                + HelpExampleCli("docker", "connect \"massgridn2n\" \"10.1.1.4\" \"119.3.66.159\"")
                + HelpExampleCli("docker", "disconnect")
                );


    
    if (strCommand == "connect")
    {
        if (params.size() != 4)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");

        std::string strCommunity = params[1].get_str();

        std::string strLocalAddr = params[2].get_str();

        std::string strSnAddr = params[3].get_str();

        if(ThreadEdgeStart(strCommunity,strLocalAddr,strSnAddr)){
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
        if (params.size() != 10)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid count parameter");
        std::string strAddr = params[1].get_str();
        if(!dockercluster.SetConnectDockerAddress(strAddr))
            throw JSONRPCError(RPC_CLIENT_INVALID_IP_OR_SUBNET, "Invalid IP");
        if(!dockercluster.ProcessDockernodeConnections())
            throw JSONRPCError(RPC_CLIENT_NODE_NOT_CONNECTED, "Connect to Masternode failed");

        DockerCreateService createService{};

        createService.pubKeyClusterAddress = dockercluster.DefaultPubkey;
        createService.vin = CTxIn();
        
        std::string strServiceName = params[2].get_str();
        createService.serviceName = strServiceName;

        std::string strServiceImage = params[3].get_str();
        createService.image = strServiceImage;

        int64_t strServiceCpu = params[4].get_int64();
        createService.cpu = strServiceCpu;
        int64_t strServiceMemoey_byte = params[5].get_int64();
        createService.memory_byte = strServiceMemoey_byte;
        
        std::string strServiceGpuName = params[6].get_str();
        createService.gpuname = strServiceGpuName;

        int64_t strServiceGpu = params[7].get_int64();
        createService.gpu = strServiceGpu;

        std::string strn2n_Community = params[8].get_str();
        createService.n2n_community = strn2n_Community;

        std::string strssh_pubkey = params[9].get_str();
        createService.ssh_pubkey = strssh_pubkey;
        EnsureWalletIsUnlocked();
        if(!dockercluster.CreateAndSendSeriveSpec(createService))
            return "CreateSpec Error";
        return "CreateSpec Successfully hash: "+createService.ToString();
    }


#endif // ENABLE_WALLET
    
    if (strCommand == "getdndata"){
        if (!masternodeSync.IsSynced())
            return "Need to Synced First";
        
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invaid count parameters");
        std::string strIPPort;
        strIPPort = params[1].get_str();
        dockercluster.sigTime=GetAdjustedTime();
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
            if(dockercluster.mapDockerServiceLists.size()){
                for(auto it = dockercluster.mapDockerServiceLists.begin();it != dockercluster.mapDockerServiceLists.end();++it){
                    UniValue obj(UniValue::VOBJ);
                    obj.push_back(Pair(it->first,Service::DockerServToJson(it->second)));
                    varr.push_back(obj);
                }
                return varr;
            }
        }
        return "getdndata Failed";
    }
    return NullUniValue;
}

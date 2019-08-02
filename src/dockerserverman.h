// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERSERVERMAN_H
#define DOCKERSERVERMAN_H

#include "net.h"
#include "key.h"
#include <vector>
#include <map>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"



#define DOCKERREQUEST_API_MINSUPPORT_VERSION 10061
#define DOCKERREQUEST_API_MAXSUPPORT_VERSION 10070

static const int DOCKER_MAX_CPU_NUM = 16;
static const int DOCKER_MAX_MEMORY_BYTE = 16;
static const int DOCKER_MAX_GPU_NUM = 12;
static const int WALLET_DATABASE_VERSION = 0x10;
static const int64_t K = 1000;
static const int64_t M = 1000 * K;
static const int64_t G = 1000 * M;
static const int TIMEOUT = 5 * 60;
static const int64_t MINER_MAX_TIME = 999999999;
class CDockerServerman;
struct ClusterServiceCreate_t;
struct ClusterServiceUpdate_t;
struct ServiceCreate;
struct ServiceUpdate;
extern CDockerServerman dockerServerman;
extern const char* strServiceCode[];
enum SERVICEMANCODE{
    SUCCESS = 0,
    SIGTIME_ERROR,
    VERSION_ERROR,
    CHECKSIGNATURE_ERROR,
    NO_THRANSACTION,
    TRANSACTION_NOT_CONFIRMS,
    TRANSACTION_DOUBLE_CREATE,
    SERVICEITEM_NOT_FOUND,
    SERVICEITEM_NO_RESOURCE,
    PAYMENT_NOT_ENOUGH,
    GPU_AMOUNT_ERROR,
    CPU_AMOUNT_ERROR,
    MEM_AMOUNT_ERROR,
    TRANSACTION_DOUBLE_TLEMENT,
    PUBKEY_ERROR,
    OUTPOINT_NOT_FOUND,
    NOT_DOCKERNODE,
    REQUEST_ERROR
};

class CDockerServerman{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    enum DNDATASTATUS{
        Free = 0,
        Ask, 
        Received,
        Creating,
        Deleteing
    };
    enum TRANSDATASTATUS{
        AskTD = 0,
        ReceivedTD,
        FreeTD
    };
    enum TLEMENTSTATE{
        FAILEDCONTINUE = -1,
        FAILEDREMOVE,
        SUCCESS
    };
    std::string LocalIP{};
    std::string defaultImage = "massgrid/10.0-autominer-ubuntu16.04";
    double feeRate = 0.01;  //set feeRate 1%
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    static bool CheckTransaction(COutPoint outpoint, CPubKey pubKeyClusterAddress, std::string& err);
    static bool CheckCreateService(ClusterServiceCreate_t& clusterServiceCreate, ServiceCreate& serviceCreate, std::string& err);
    static bool CheckUpdateService(ClusterServiceUpdate_t& clusterServiceUpdate, ServiceUpdate& serviceUpdate, std::string& err);
    DNDATASTATUS dndataStatus;
    TRANSDATASTATUS transdataStatus;

    void setDNDataStatus(DNDATASTATUS type){
        dndataStatus = type;
    }
    DNDATASTATUS getDNDataStatus(){
        return dndataStatus;
    }
    void setTRANSDataStatus(TRANSDATASTATUS type){
        transdataStatus = type;
    }
    TRANSDATASTATUS getTRANSDataStatus(){
        return transdataStatus;
    }

};

#endif  //DOCKERSERVERMAN_H
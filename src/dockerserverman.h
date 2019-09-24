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

#define DOCKERREQUEST_API_MINSUPPORT_VERSION 10071
#define DOCKERREQUEST_API_MAXSUPPORT_VERSION 10080

static const int TIMEOUT = 5 * 60;
class CDockerServerman;
extern CDockerServerman dockerServerman;

class DockerCreateService;
class DockerUpdateService;
class DockerDeleteService;
struct ServiceCreate;
struct ServiceUpdate;

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
    REQUEST_ERROR,
    PUBKEY_NOT_FOUND,
    SERVICE_NOT_FOUND,
    CREATE_TRANSACTION_ERROR,
    COMMIT_TRANSACTION_ERROR
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
        Updating,
        Deleteing
    };
    enum TRANSDATASTATUS{
        AskTD = 0,
        ReceivedTD,
        FreeTD
    };
    enum SERVICESTATUS{
        AskSD = 0,
        ReceivedSD,
        FreeSD,
        UpdatingSD
    };
    enum TLEMENTSTATE{
        FAILEDCONTINUE = -1,
        FAILEDREMOVE,
        SUCCESS
    };
    std::string LocalIP{};
    double feeRate = 0.01;  //set feeRate 1%
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    static bool CheckCreateService(DockerCreateService& dockerCreateService, ServiceCreate& serviceCreate, std::string& err);
    static bool CheckUpdateService(DockerUpdateService& dockerUpdateService, ServiceUpdate& serviceUpdate, std::string& err);
    static bool CheckDeleteService(DockerDeleteService& dockerDeleteService,std::string& msg, std::string& err);
    DNDATASTATUS dndataStatus;
    TRANSDATASTATUS transdataStatus;
    SERVICESTATUS serviceStatus;

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
    void setSERVICEDataStatus(SERVICESTATUS type){
        serviceStatus = type;
    }
    SERVICESTATUS getSERVICEStatus(){
        return serviceStatus;
    }
};

#endif  //DOCKERSERVERMAN_H
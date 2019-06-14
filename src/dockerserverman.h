// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERSERVERMAN_H
#define DOCKERSERVERMAN_H

#include "net.h"
#include "key.h"
#include <vector>
#include <map>
#include <dockerservice.h>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"



#define DOCKERREQUEST_API_VERSION 10061
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
class DockerCreateService;
class DockerDeleteService;
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
    OUTPOINT_NOT_FOUND
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
    std::string defaultImage = "massgrid/10.0-autominer-ubuntu16.04";
    double feeRate = 0.01;  //set feeRate 1%
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    bool CheckAndCreateServiveSpec(DockerCreateService Spec, int& ErrCode);
    bool CheckAndRemoveServiveSpec(DockerDeleteService delService, int& errCode);
    int SetTlementServiceWithoutDelete(uint256 serviceTxid);
    bool GetNodeIdAndstopMiner(Item item,std::string &nodeid);
    bool CreateMinerServiceSpec(std::string nodeid);
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
class DockerGetData{
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    int errCode{};
    CPubKey pubKeyClusterAddress{};
    int64_t sigTime{}; //dkct message times
    bool fPersistentStore{};
    std::map<std::string,Service> mapDockerServiceLists{};
    map<Item,Value_price> items{};
    string masternodeAddress{};
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
        READWRITE(fPersistentStore);
        READWRITE(mapDockerServiceLists);
        READWRITE(items);
        READWRITE(masternodeAddress);
        READWRITE(errCode);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << version;
        ss << pubKeyClusterAddress;
        ss << fPersistentStore;
        ss << mapDockerServiceLists;
        ss << items;
        ss << masternodeAddress;
        ss << errCode;
        return ss.GetHash();
    }
};
class DockerGetTranData{
public:
    enum TRANDATATYPE{
        DEFAULT = 0,
        ASKALL
    };
public:
    int64_t version = DOCKERREQUEST_API_VERSION;
    int64_t sigTime{0};
    uint256 txid;
    int askCode=TRANDATATYPE::DEFAULT;

public:
    DockerGetTranData() = default;
    DockerGetTranData(int64_t _sigTime,uint256 _txid,int _askCode=TRANDATATYPE::DEFAULT,int64_t _version=DOCKERREQUEST_API_VERSION):
                    txid(_txid),sigTime(_sigTime),askCode(_askCode),version(_version){};
    DockerGetTranData(const DockerGetTranData& from){
        version=from.version;
        sigTime=from.sigTime;
        txid=from.txid;
        askCode=from.askCode;
    }

    DockerGetTranData& operator = (DockerGetTranData const& from){
        version=from.version;
        sigTime=from.sigTime;
        txid=from.txid;
        askCode=from.askCode;
        return *this;
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(version);
        READWRITE(sigTime);
        READWRITE(txid);
        READWRITE(askCode);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << version;
        ss << sigTime;
        ss << txid;
        ss << askCode;
        return ss.GetHash();
    }
};
namespace TASKDTDATA{
    enum TASTUS{
        DEFAULT=0,
        SUCCESS,
        ERRORCODE
    };
}
class DockerTransData{
public:
    int64_t version = DOCKERREQUEST_API_VERSION;
    int64_t sigTime{0};
    uint256 txid;
    int64_t deleteTime{};
    double feeRate;
    int errCode{};
    std::string taskStatus;
    uint256 tlementtxid;
    std::map<std::string,std::string> extData;
    int msgStatus=TASKDTDATA::DEFAULT;
    
public:
    DockerTransData() = default;
    DockerTransData(int64_t _sigTime,uint256 _txid,int64_t _deleteTime,double _feeRate,int _errCode,
            std::string _taskStatus,uint256 _tlementtxid,std::map<std::string,std::string>_extData,int _msgStatus,int64_t _version=DOCKERREQUEST_API_VERSION):
            txid(_txid),sigTime(_sigTime),deleteTime(_deleteTime),feeRate(_feeRate),errCode(_errCode),taskStatus(_taskStatus),
            tlementtxid(_tlementtxid),extData(_extData),msgStatus(_msgStatus),version(_version){};
    DockerTransData(const DockerTransData& from){
        version=from.version;
        sigTime=from.sigTime;
        txid=from.txid;
        deleteTime=from.deleteTime;
        feeRate=from.feeRate;
        errCode=from.errCode;
        taskStatus=from.taskStatus;
        tlementtxid=from.tlementtxid;
        extData=from.extData;
        msgStatus=from.msgStatus;
    }

    DockerTransData& operator = (DockerTransData const& from){
        version=from.version;
        sigTime=from.sigTime;
        txid=from.txid;
        deleteTime=from.deleteTime;
        feeRate=from.feeRate;
        errCode=from.errCode;
        taskStatus=from.taskStatus;
        tlementtxid=from.tlementtxid;
        extData=from.extData;
        msgStatus=from.msgStatus;
        return *this;
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(version);
        READWRITE(sigTime);
        READWRITE(txid);
        READWRITE(deleteTime);
        READWRITE(feeRate);
        READWRITE(errCode);
        READWRITE(taskStatus);
        READWRITE(tlementtxid);
        READWRITE(extData);
        READWRITE(msgStatus);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << version;
        ss << sigTime;
        ss << txid;
        ss << deleteTime;
        ss << feeRate;
        ss << errCode;
        ss << taskStatus;
        ss << tlementtxid;
        ss << extData;
        ss << msgStatus;
        return ss.GetHash();
    }
    void Init()
    {
        version=DOCKERREQUEST_API_VERSION;
        sigTime=0;
        txid.SetNull();
        deleteTime=0;
        feeRate=0.0;
        errCode=0;
        taskStatus="";
        tlementtxid.SetNull();
        extData.clear();
        msgStatus=TASKDTDATA::DEFAULT;
    }
};
class DockerCreateService{
public:

    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    CPubKey pubKeyClusterAddress{};
    uint256 txid{};
    int64_t sigTime{}; // message times
    bool fPersistentStore{};
    std::string n2n_community{};
    std::string serviceName{};
    std::string image{};
    std::string ssh_pubkey{};
    Item item{};
    std::map<std::string,std::string> env{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vchSig);
        READWRITE(txid);
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
        READWRITE(fPersistentStore);
        READWRITE(n2n_community);
        READWRITE(serviceName);
        READWRITE(image);
        READWRITE(item);
        READWRITE(ssh_pubkey);
        READWRITE(env);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << txid;
        ss << version;
        ss << pubKeyClusterAddress;
        ss << sigTime;
        ss << fPersistentStore;
        ss << n2n_community;
        ss << serviceName;
        ss << image;
        ss << item;
        ss << ssh_pubkey;
        ss << env;
        return ss.GetHash();
    }
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);
    std::string ToString(){
        return GetHash().ToString();
    }
};
class DockerDeleteService{
public:

    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    CPubKey pubKeyClusterAddress{};
    uint256 txid{};
    int64_t sigTime{}; //dkct message times

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vchSig);
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(txid);
        READWRITE(sigTime);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << version;
        ss << pubKeyClusterAddress;
        ss << txid;
        ss << sigTime;

        return ss.GetHash();
    }
    bool Sign(const CKey& keyMasternode, const CPubKey& pubKeyMasternode);
    bool CheckSignature(CPubKey& pubKeyMasternode);
    std::string ToString(){
        return GetHash().ToString();
    }    
};
bool CheckAndGetTransaction(DockerGetTranData dtdata,int& errCode);
#endif  //DOCKERSERVERMAN_H
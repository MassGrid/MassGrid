#ifndef DOCKERSERVERMAN_H
#define DOCKERSERVERMAN_H

#include "net.h"
#include "key.h"
#include <vector>
#include <dockerservice.h>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"

static const int DOCKER_MAX_CPU_NUM = 16;
static const int DOCKER_MAX_MEMORY_BYTE = 16;
static const int DOCKER_MAX_GPU_NUM = 12;
static const int WALLET_DATABASE_VERSION = 0x10;
static const int64_t K = 1000;
static const int64_t M = 1000 * K;
static const int64_t G = 1000 * M;
static const int TIMEOUT = 5 * 60;
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
    enum TLEMENTSTATE{
        FAILEDCONTINUE = -1,
        FAILEDREMOVE,
        SUCCESS
    };
    double feeRate = 0.01;  //set feeRate 1%
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    bool CheckAndCreateServiveSpec(DockerCreateService Spec, int& ErrCode);
    bool CheckAndRemoveServiveSpec(DockerDeleteService delService, int& errCode);
    int SetTlementServiceWithoutDelete(uint256 serviceTxid);
    DNDATASTATUS dndataStatus;

    void setDNDataStatus(DNDATASTATUS type){
        dndataStatus = type;
    }
    DNDATASTATUS getDNDataStatus(){
        return dndataStatus;
    }

};
class DockerGetData{
public:
    uint64_t version = DOCKERREQUEST_API_VERSION;
    int errCode{};
    CPubKey pubKeyClusterAddress{};
    int64_t sigTime{}; //dkct message times
    std::map<std::string,Service> mapDockerServiceLists{};
    map<Item,Value_price> items{};
    string masternodeAddress{};
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
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
        ss << mapDockerServiceLists;
        ss << items;
        ss << masternodeAddress;
        ss << errCode;
        return ss.GetHash();
    }
};
class DockerGetTranData{
public:
    int64_t version = DOCKERREQUEST_API_VERSION;
    int64_t sigTime{0};
    uint256 txid;
public:
    DockerGetTranData() = default;
    DockerGetTranData(int64_t _sigTime,uint256 _txid,int64_t _version=DOCKERREQUEST_API_VERSION):
                    txid(_txid),sigTime(_sigTime),version(_version){};
    DockerGetTranData(const DockerGetTranData& from){
        version=from.version;
        sigTime=from.sigTime;
        txid=from.txid;
    }

    DockerGetTranData& operator = (DockerGetTranData const& from){
        version=from.version;
        sigTime=from.sigTime;
        txid=from.txid;
        return *this;
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(version);
        READWRITE(sigTime);
        READWRITE(txid);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << version;
        ss << sigTime;
        ss << txid;
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
    int msgStatus=TASKDTDATA::DEFAULT;
    
public:
    DockerTransData() = default;
    DockerTransData(int64_t _sigTime,uint256 _txid,int64_t _deleteTime,double _feeRate,int _errCode,
            std::string _taskStatus,uint256 _tlementtxid,int _msgStatus,int64_t _version=DOCKERREQUEST_API_VERSION):
            txid(_txid),sigTime(_sigTime),deleteTime(_deleteTime),feeRate(_feeRate),errCode(_errCode),taskStatus(_taskStatus),
            tlementtxid(_tlementtxid),msgStatus(_msgStatus),version(_version){};
    DockerTransData(const DockerTransData& from){
        version=from.version;
        sigTime=from.sigTime;
        txid=from.txid;
        deleteTime=from.deleteTime;
        feeRate=from.feeRate;
        errCode=from.errCode;
        taskStatus=from.taskStatus;
        tlementtxid=from.tlementtxid;
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
        msgStatus=TASKDTDATA::DEFAULT;
    }
};
class DockerCreateService{
public:

    uint64_t version = DOCKERREQUEST_API_VERSION;
    std::vector<unsigned char> vchSig{};
    CPubKey pubKeyClusterAddress{};
    uint256 txid{};
    int64_t sigTime{}; //dkct message times
    std::string n2n_community{};
    std::string serviceName{};
    std::string image{};
    std::string ssh_pubkey{};
    Item item{};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(vchSig);
        READWRITE(txid);
        READWRITE(version);
        READWRITE(pubKeyClusterAddress);
        READWRITE(sigTime);
        READWRITE(n2n_community);
        READWRITE(serviceName);
        READWRITE(image);
        READWRITE(item);
        READWRITE(ssh_pubkey);
    }
    uint256 GetHash() const
    {
        CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << txid;
        ss << version;
        ss << pubKeyClusterAddress;
        ss << sigTime;
        ss << n2n_community;
        ss << serviceName;
        ss << image;
        ss << item;
        ss << ssh_pubkey;
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
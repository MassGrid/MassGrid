#include "networkers.h"
#include "dockerserverman.h"
#include "instantx.h"
#include "wallet/wallet.h"
#include "init.h"
#include "sync.h"
#include "validation.h"

#define LOADRESOURCETIMEOUT 30
#define ASKSERVICESDATATIMEOUT 20

AskDNDataWorker::AskDNDataWorker(QObject* parent) :
    QObject(parent)
{

}

AskDNDataWorker::~AskDNDataWorker()
{

}

void AskDNDataWorker::startTask()
{
    LogPrintf("AskDNDataWorker::startTask Ask.\n");

    int index = 0;
    bool isTaskFinished = false;
    while(isNeedToWork()){
        if(isAskDNDataFinished()){
            isTaskFinished = true;
            break;
        }
        QThread::sleep(1);
        Q_EMIT updateTaskTime(++index);
        if(index >= LOADRESOURCETIMEOUT)
            break;
    }
    Q_EMIT askDNDataFinished(isTaskFinished);
}

bool AskDNDataWorker::isAskDNDataFinished()
{
    if(dockerServerman.getDNDataStatus() == CDockerServerman::Ask){
        LogPrintf("AskDNDataWorker get DNData Status:CDockerServerman::Asking\n");
        return false;
    }
    else if(dockerServerman.getDNDataStatus() == CDockerServerman::Received ||
            dockerServerman.getDNDataStatus() == CDockerServerman::Free){
        LogPrintf("AskDNDataWorker -> isAskDNDataFinished\n");
        return true;
    }
}

AskServicesWorker::AskServicesWorker(QObject* parent) :
    QObject(parent),
    m_isNeedToWork(true)
{

}

AskServicesWorker::~AskServicesWorker()
{

}

void AskServicesWorker::startTask()
{
    LogPrintf("AskServicesWorker::startTask Ask.\n");

    int index = 0;
    bool isTaskFinished = false;
    while(isNeedToWork()){
        if(isAskServicesFinished()){
            isTaskFinished = true;
            break;
        }
        QThread::sleep(2);
        Q_EMIT updateTaskTime(++index);
        if(index >= ASKSERVICESDATATIMEOUT)
            break;
    }
    Q_EMIT askServicesFinished(isTaskFinished);
}

bool AskServicesWorker::isAskServicesFinished()
{
    if(dockerServerman.getSERVICEStatus() == CDockerServerman::SERVICESTATUS::AskSD || 
       dockerServerman.getSERVICEStatus() == CDockerServerman::SERVICESTATUS::UpdatingSD){
        LogPrintf("AskServicesWorker get service CDockerServerman::SERVICESTATUS::Asking\n");
        return false;
    }
    else if(dockerServerman.getSERVICEStatus() == CDockerServerman::SERVICESTATUS::ReceivedSD ||
            dockerServerman.getSERVICEStatus() == CDockerServerman::SERVICESTATUS::FreeSD){
        LogPrintf("AskServicesWorker -> isAskServicesFinished\n");
        return true;
    }
}

CheckoutTransaction::CheckoutTransaction(std::string txid,QObject* parent) :
    QObject(parent),
    m_isNeedToWork(true)
{
    m_txid = txid;
}

CheckoutTransaction::~CheckoutTransaction()
{

}

void CheckoutTransaction::startTask()
{
    std::string strErr;
    int index = 0;
    bool isFinished = false;
    while(isNeedToWork()){
        if(isTransactionFinished(m_txid,strErr)){
            isFinished = true;
            break;
        }
        QThread::sleep(2);
        Q_EMIT updateTaskTime(++index);
    }
    if(isFinished){
        Q_EMIT checkTransactionFinished();
    }
    else
    {
        Q_EMIT threadStopped(); 
    }
}

bool CheckoutTransaction::isTransactionFinished(std::string txid,std::string& strErr)
{
    CWalletTx& wtx = pwalletMain->mapWallet[uint256S(txid)];  //watch only not check

    //check tx in block
    bool fLocked = instantsend.IsLockedInstantSendTransaction(wtx.GetHash());
    int confirms = wtx.GetDepthInMainChain(false);
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
    LogPrintf("AddDockerServiceDlg::is Transaction Finished\n");
    return true;
}
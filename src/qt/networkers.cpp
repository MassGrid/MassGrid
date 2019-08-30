#include "networkers.h"
#include "dockerserverman.h"
#define LOADRESOURCETIMEOUT 30

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
        LogPrintf("AddDockerServiceDlg get DNData Status:CDockerServerman::Asking\n");
        return false;
    }
    else if(dockerServerman.getDNDataStatus() == CDockerServerman::Received ||
            dockerServerman.getDNDataStatus() == CDockerServerman::Free){
        LogPrintf("AskDNDataWorker -> isAskDNDataFinished\n");
        return true;
    }
}
#ifndef DOCKERSERVERMAN_H
#define DOCKERSERVERMAN_H

#include "net.h"
#include "key.h"
#include <vector>
#include <dockerservice.h>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"

#define DOCKER_MAX_CPU_NUM 1000000000
#define DOCKER_MAX_MEMORY_BYTE 2048000000
#define DOCKER_MAX_GPU_NUM 2
class CDockerServerman;
extern CDockerServerman dockerServerman;

class CDockerServerman{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:
    enum DNDATASTATUS{
        Free = 0,
        Ask,
        Received
    };
    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    bool CheckAndCreateServiveSpec(DockerCreateService Spec);
    bool CheckAndUpdateServiceSpec(DockerUpdateService Spec);
    DNDATASTATUS dndataStatus;

    void setDNDataStatus(DNDATASTATUS type){
        dndataStatus = type;
    }
    DNDATASTATUS getDNDataStatus(){
        return dndataStatus;
    }

};
#endif  //DOCKERSERVERMAN_H
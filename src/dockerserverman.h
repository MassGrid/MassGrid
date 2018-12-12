#ifndef DOCKERSERVERMAN_H
#define DOCKERSERVERMAN_H

#include "net.h"
#include "key.h"
#include <vector>
#include <dockerservice.h>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"

#define DOCKER_MAX_CPU_NUM 4
#define DOCKER_MAX_MEMORY_BYTE 4143001600
#define DOCKER_MAX_GPU_NUM 12
class CDockerServerman;
extern CDockerServerman dockerServerman;


class CDockerServerman{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    bool CheckAndCreateServiveSpec(DockerCreateService Spec);
    bool CheckAndUpdateServiceSpec(DockerUpdateService Spec);
};
#endif  //DOCKERSERVERMAN_H
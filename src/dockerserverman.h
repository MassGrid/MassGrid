#ifndef DOCKERSERVERMAN_H
#define DOCKERSERVERMAN_H

#include "net.h"
#include "key.h"
#include <vector>
#include <dockerservice.h>
#include <dockertask.h>
#include "base58.h"
#include "primitives/transaction.h"
class CDockerServerman;
extern CDockerServerman dockerServerman;


class CDockerServerman{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;
public:

    void ProcessMessage(CNode* pfrom, std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    bool CheckAndCreateServerSpec(DockerCreateService Spec);
    bool CheckAndUpdateServerSpec(DockerUpdateService Spec);
};
#endif  //DOCKERSERVERMAN_H
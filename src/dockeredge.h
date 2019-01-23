#ifndef DOCKEREDGE_H
#define DOCKEREDGE_H
#include "util.h"
bool ThreadEdgeStart(std::string community,std::string localaddr,std::string snaddr,std::function<void(bool)>start = nullptr);
void ThreadEdgeStop();
void threadTunStop();
#endif //DOCKEREDGE_H
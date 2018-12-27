#ifndef DOCKEREDGE_H
#define DOCKEREDGE_H
#include "util.h"
#include "init.h"
bool ThreadEdgeStart(std::string community,std::string localaddr,std::string snaddr,std::function<void(bool)>start = nullptr);
void ThreadEdgeStop();
#endif //DOCKEREDGE_H
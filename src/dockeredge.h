#ifndef DOCKEREDGE_H
#define DOCKEREDGE_H
#include "util.h"
#include "init.h"
bool ThreadEdgeStart(std::string community,std::string localaddr,std::string snaddr,std::function<void()>start = nullptr,std::function<void()>stop = nullptr);
void ThreadEdgeStop();
#endif //DOCKEREDGE_H
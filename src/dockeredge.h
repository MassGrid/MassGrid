// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKEREDGE_H
#define DOCKEREDGE_H
#include "util.h"
bool ThreadEdgeStart(std::string community,std::string localaddr,std::string netmask,std::string snaddr,std::function<void(bool)>start = nullptr);
void ThreadEdgeStop();
void threadTunStop();
bool IsThreadRunning();
#endif //DOCKEREDGE_H
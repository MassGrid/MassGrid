// Copyright (c) 2014-2017 The MassGrid developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERMAN_H
#define DOCKERMAN_H
#include "http.h"
#include "dockernode.h"
#include "dockerswarm.h"
#include "dockerservice.h"
#include "dockertask.h"

static const char* strMethed[] = {"nodes","services","swarm","tasks","info","version"};
enum Methed{
    METHED_NODES = 0,
    METHED_SERVICES,
    METHED_SWARM,
    METHED_TASKS,
    METHED_INFO,
    METHED_VERSION
};
class dockerman{
private:
    const char* address = "127.0.0.1";
    uint32_t port = 2375;

public:
    std::map<std::string,Node> mapDockerNodeLists;
    std::map<std::string,Service> mapDockerServiceLists;
    std::map<std::string,Task> mapDockerTaskLists;
    Swarm swarm;

    void Update(); //update all data;
    void RequestMessages(Methed mtd,)
    void PushMessage(Methed mtd,);

    void SetPort(uint32_t p){port = p;}
    uint32_t GetPort(){return port;}
    
};

#endif //DOCKERMAN_H

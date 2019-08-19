// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZESCLIENT_H
#define PRIZESCLIENT_H
#include "dockerpriceconfig.h"
#include "dockertask.h"
#include "net.h"
#include "netbase.h"
#include "prizes_node.h"
#include "prizes_order.h"
#include "prizes_service.h"
#include <set>
#include <map>
class PrizesClient{
    std::string unix_sock_address{};
    std::string APIAddr{};
    int APIport{};
public:
    void SetDockerApiConnection(std::string in);
    void SetDockerUnixSock(std::string in);
    bool GetNodeList(NodeListStatistics& nodeListStatistics,std::string& err);
    bool GetService(std::string ServiceID, ServiceInfo& serviceInfo, std::string& err);
    bool PrizesClient::GetServiceDelete(std::string ServiceID,uint256 statementid, std::string& err);
    bool GetServiceFromPubkey(std::string strPubkey,int64_t start,int64_t count,bool full, std::vector<ServiceInfo>& vecServiceInfo, std::string& err);
    bool PostServiceCreate(ServiceCreate& serviceCreate, std::string& ServiceID, std::string& err);
    bool PostServiceUpdate(ServiceUpdate& serviceUpdate, std::string& err);
    bool GetMachines(ResponseMachines& machines, std::string& err);
    static void ParseNode2Price(NodeListStatistics& nodeListStatistics, map<Item, Value_price>& price);
};
extern PrizesClient prizesClient;

#endif //PRIZESCLIENT_H

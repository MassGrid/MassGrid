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
    PrizesClient();
    bool GetNodeList(NodeListStatistics& nodeListStatistics);
    bool GetService(std::string ServiceID, ServiceInfo& serviceInfo);
    bool GetServiceFromPubkey(std::string strPubkey, std::vector<ServiceInfo>& vecServiceInfo);
    bool PostServiceCreate(ServiceCreate& serviceCreate, std::string& ServiceID, std::string& err);
    bool PostServiceUpdate(ServiceUpdate& serviceUpdate, std::string& err);
    bool GetMachines(ResponseMachines& machines);
    static void ParseNode2Price(NodeListStatistics& nodeListStatistics, map<Item, Value_price>& price);
};
extern PrizesClient prizesClient;

#endif //PRIZESCLIENT_H

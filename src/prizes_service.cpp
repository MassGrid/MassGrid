#include "prizes_service.h"
#include "boost/lexical_cast.hpp"
#include "messagesigner.h"
#include <boost/algorithm/string.hpp>


void ServiceInfo::DecodeFromJson(const UniValue& data, ServiceInfo& serviceInfo)
{
    LogPrint("docker","ServiceInfo::DecodeFromJson\n");
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "service_id")
            serviceInfo.ServiceID = tdata.get_str();
        else if (vKeys[i] == "create_at")
            serviceInfo.CreatedAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "delete_at")
            serviceInfo.DeleteAt = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "next_check_time")
            serviceInfo.NextCheckTime = getDockerTime(tdata.get_str());
        else if (vKeys[i] == "order") {
            for (int64_t j = 0; j < tdata.size(); ++j) {
                ServiceOrder serviceOrder{};
                ServiceOrder::DecodeFromJson(tdata[j], serviceOrder);
                serviceInfo.Order.push_back(serviceOrder);
            }
        } else if (vKeys[i] == "create_spec") {
            ServiceCreate::DecodeFromJson(tdata,serviceInfo.CreateSpec);
        } else if (vKeys[i] == "update_spec") {
            for (int64_t j = 0; j < tdata.size(); ++j) {
                ServiceUpdate serviceUpdate{};
                ServiceUpdate::DecodeFromJson(tdata[j], serviceUpdate);
                serviceInfo.UpdateSpec.push_back(serviceUpdate);
            }
        } else if (vKeys[i] == "state")
            serviceInfo.State = tdata.get_str();
        else if (vKeys[i] == "task_info"){
            Task task{};
            Task::DecodeFromJson(tdata,task);
            serviceInfo.TaskInfo.push_back(task);
        }
    }
    serviceInfo.jsonUniValue = data;
}

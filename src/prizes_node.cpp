#include<prizes_node.h>
std::string HardWare::ToString()
{
    return CPUType + std::to_string(CPUThread) + MemoryType + std::to_string(MemoryCount) + GPUType + std::to_string(GPUCount) + PersistentStore;
}
UniValue HardWare::ToJson(HardWare& hardware)
{
    UniValue data(UniValue::VOBJ);
    {
        data.push_back(Pair("cpu_type", hardware.CPUType));
        data.push_back(Pair("cpu_thread", hardware.CPUThread));
        data.push_back(Pair("memory_type", hardware.MemoryType));
        data.push_back(Pair("memory_count", hardware.MemoryCount));
        data.push_back(Pair("gpu_type", hardware.GPUType));
        data.push_back(Pair("gpu_count", hardware.GPUCount));
        data.push_back(Pair("persistent_store", hardware.PersistentStore));
    }
    return data;
}
void HardWare::DecodeFromJson(const UniValue& data, HardWare& hardware)
{
    LogPrint("docker","HardWare::DecodeFromJson\n");
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "cpu_type")
            hardware.CPUType = tdata.get_str();
        else if (vKeys[i] == "cpu_thread")
            hardware.CPUThread = tdata.get_int64();
        else if (vKeys[i] == "memory_type")
            hardware.MemoryType = tdata.get_str();
        else if (vKeys[i] == "memory_count")
            hardware.MemoryCount = tdata.get_int64();
        else if (vKeys[i] == "gpu_type")
            hardware.GPUType = tdata.get_str();
        else if (vKeys[i] == "gpu_count")
            hardware.GPUCount = tdata.get_int64();
        else if (vKeys[i] == "persistent_store")
            hardware.PersistentStore = tdata.get_str();
    }
}

void NodeInfo::DecodeFromJson(const UniValue& data, NodeInfo& node)
{
    LogPrint("docker","NodeInfo::DecodeFromJson\n");
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "node_id")
            node.NodeID = tdata.get_str();
        else if (vKeys[i] == "noed_state")
            node.NodeState = tdata.get_str();
        else if (vKeys[i] == "reach_address")
            node.ReachAddress = tdata.get_str();
        else if (vKeys[i] == "hardware")
            HardWare::DecodeFromJson(tdata,node.hardware);
        else if (vKeys[i] == "onworking")
            node.OnWorking = tdata.get_bool();
        else if(vKeys[i]=="labels"){
            ParseLabels(tdata, node.Labels);
        }
    }
}
void NodeListStatistics::DecodeFromJson(const UniValue& data, NodeListStatistics& nodelist)
{
    LogPrint("docker","NodeListStatistics::DecodeFromJson\n");
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (vKeys[i] == "worker_token")
            nodelist.Token = tdata.get_str();
        else if (vKeys[i] == "total_count")
            nodelist.TotalCount = tdata.get_int();
        else if (vKeys[i] == "availability_count")
            nodelist.AvailabilityCount = tdata.get_int();
        else if (vKeys[i] == "usable_count")
            nodelist.UsableCount = tdata.get_int();
        else if (vKeys[i] == "list") {
            for (size_t j = 0; j < tdata.size(); j++) {
                NodeInfo node{};
                NodeInfo::DecodeFromJson(tdata[j], node);
                nodelist.list.push_back(node);
            }
        }
    }
}
void ParseLabels(const UniValue& data, std::map<std::string, std::string>& labels)
{
    std::vector<std::string> vKeys = data.getKeys();
    for (size_t i = 0; i < data.size(); i++) {
        UniValue tdata(data[vKeys[i]]);
        if (data[vKeys[i]].isStr()) {
            labels.insert(std::make_pair(vKeys[i], tdata.get_str()));
        }
    }
}
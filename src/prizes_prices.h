// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PRIZES_PRICES_H
#define PRIZES_PRICES_H

#include "key.h"
#include "sync.h"
#include "util.h"
#include "utilstrencodings.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <univalue.h>
#include <vector>

class Info{
public:
    std::string Type;
    std::string Name;
    int Count;
    Info() = default;
    Info(std::string type,std::string name,int count){
        Type=type;
        Name=name;
        Count=count;
    }
    Info(const Info& from){
        Type = from.Type;
        Name = from.Name;
        Count = from.Count;
    }
    Info& operator = (Info const& from){
        Type = from.Type;
        Name = from.Name;
        Count = from.Count;
        return *this;
    }
    bool operator < (const Info &a) const{
        if(Type != a.Type)
            return Type < a.Type;
        if(Name != a.Name)
            return Name < a.Name;
        if(Count != a.Count)
            return Count < a.Count;
        return false;
    }
    bool operator != (const Info &a) const{
        if(Type != a.Type)
            return true;
        if(Name != a.Name)
            return true;
        if(Count != a.Count)
            return true;
        return false;
    }
    
    bool operator == (const Info &a) const{
        if(*this != a)
            return false;
        return true;
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(Type);
        READWRITE(Name);
        READWRITE(Count);
    }
    std::string ToString(){
        std::ostringstream out;
        out << " Type :" <<Type << " Name :" <<Name <<" Count: "<<Count;
        return out.str();
    }
};
class Value_price{
public:
    CAmount price;
    int count;
    Value_price() = default;
    Value_price(CAmount p,int c):price(p),count(c){}
    Value_price(const Value_price &from){
        price = from.price;
        count = from.count;
    }
    Value_price& operator=(Value_price const& from){
        price=from.price;
        count=from.count;
        return *this;
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(price);
        READWRITE(count);
    }
};
class Item{
public:
    Info cpu;
    Info mem;
    Info gpu;
public:
    Item(){
        cpu.Type = "cpu";
        cpu.Name = "intel_i3";
        cpu.Count = 1;
        mem.Type = "mem";
        mem.Name = "ddr";
        mem.Count = 1;
        gpu.Type = "gpu";
        gpu.Name = "nvidia_p106_400_3g";
        gpu.Count = 1;

    }
    Item(const Item &from){
        cpu = from.cpu;
        mem = from.mem;
        gpu = from.gpu;
    }
    Item& operator =(Item const& from){
        cpu = from.cpu;
        mem = from.mem;
        gpu = from.gpu;
        return *this;
    }
    Item(std::string cpuname,int cpucount,std::string memname,int memcount,std::string gpuname,int gpucount){
        cpu.Type = "cpu";
        cpu.Name = cpuname;
        cpu.Count = cpucount;
        mem.Type = "mem";
        mem.Name = memname;
        mem.Count = memcount;
        gpu.Type = "gpu";
        gpu.Name = gpuname;
        gpu.Count = gpucount;
    }
    void Set(std::string cpuname,int cpucount,std::string memname,int memcount,std::string gpuname,int gpucount){
        cpu.Name = cpuname;
        cpu.Count = cpucount;
        mem.Name = memname;
        mem.Count = memcount;
        gpu.Name = gpuname;
        gpu.Count = gpucount;
    }
    bool operator != (const Item &a) const{
        if(cpu != a.cpu)
            return true;
        if(gpu != a.gpu)
            return true;
        if(mem != a.mem)
            return true;
        return false;
    }
    bool operator < (const Item &a) const {
        if(cpu != a.cpu)
            return cpu < a.cpu;
        
        if(mem != a.mem)
            return mem < a.mem;

        if(gpu != a.gpu)
            return gpu < a.gpu;
        return false;
    }
    std::string ToString(){
        return cpu.ToString()+" "+mem.ToString()+" "+gpu.ToString();
    }
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(cpu);
        READWRITE(mem);
        READWRITE(gpu);
    }
};

uint64_t getDockerTime(const std::string& timeStr);
uint64_t TimeestampStr(const char* nTimeStr);
std::string unixTime2Str(uint64_t unixtime);

#endif //PRIZES_PRICES_H
#ifndef DOCKERPRICECONFIG_H
#define DOCKERPRICECONFIG_H
#include "amount.h"
#include <vector>
#include <set>
#include <string>
#include "boost/lexical_cast.hpp"
class CDockerPriceConfig;
extern CDockerPriceConfig dockerPriceConfig;

class CDockerPriceConfig
{

public:

    class CDockerPriceEntry {

    private:
        std::string type;    //cpu mem gpu
        std::string name;    //
        CAmount price;
    public:

        CDockerPriceEntry(std::string Type, std::string Name, std::string Price) {
            this->type = Type;
            this->name = Name;
            this->price = boost::lexical_cast<double>(Price) *COIN;
        }

        const std::string& getType() const {
            return type;
        }

        void setType(const std::string& type) {
            this->type = type;
        }
        
        const std::string& getName() const {
            return name;
        }

        void setName(const std::string& name) {
            this->name = name;
        }
        CAmount getPrice() const {
            return price;
        }

        void setPrice(const std::string& price) {
            this->price = boost::lexical_cast<double>(price) *COIN;
        }
    };

    CDockerPriceConfig() {
        entries = std::vector<CDockerPriceEntry>();
    }

    void clear();
    bool read(std::string& strErr);
    void add(std::string Type, std::string Name, std::string Price);

    std::vector<CDockerPriceEntry>& getEntries() {
        return entries;
    }

    int getCount() {
        return (int)entries.size();
    }
    CAmount getPrice(std::string type,std::string name){
        for(auto it= entries.begin();it!=entries.end();++it){
            if(it->getType() == type && it->getName() == name)
                return it->getPrice();
        }
        return CAmount(0);
    }
    std::set<std::string>getNameSet(std::string type){
        std::set<std::string> s;
        for(auto it= entries.begin();it!=entries.end();++it){
            if(it->getType() == type)
                s.insert(it->getName());
        }
        return s;
    }
private:
    std::vector<CDockerPriceEntry> entries;


};
#endif //DOCKERPRICECONFIG_H
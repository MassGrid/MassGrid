
#include "netbase.h"
#include "dockerpriceconfig.h"
#include "util.h"
#include "chainparams.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

CDockerPriceConfig dockerPriceConfig;

void CDockerPriceConfig::add(std::string Type, std::string Name, std::string Price){
    for(auto it=entries.begin();it!=entries.end();++it){
        if(Type == it->getType() && Name ==it->getName()){
            it->setPrice(Price);
            return;
            }
    }
    CDockerPriceEntry cme(Type, Name, Price);
    entries.push_back(cme);
}
void CDockerPriceConfig::writeSave() {

    boost::filesystem::path pathDockerPriceConfigFile = GetDockerPriceConfigFile();
    boost::filesystem::ifstream streamConfig(pathDockerPriceConfigFile);

    FILE* configFile = fopen(pathDockerPriceConfigFile.string().c_str(), "a");
    if (configFile != NULL) {
        std::string strHeader = "# DockerPrice config file\n"
                        "# price :cpu: MGD/thread mem: MGD/GB gpu: MGD/count\n"
                        "# Format: type name price(MGD/hour)\n"
                        "# Example0: cpu i7 0.2\n"
                        "# Example1: mem ddr4 0.1\n"
                        "# Example3: gpu nvidia_p106_100_6g 0.2\n";
        std::string strBody;
        for(auto it=entries.begin();it!=entries.end();++it){
            strBody += strprintf("%s %s %lf\n",it->getType(),it->getName(),(double)it->getPrice()/ COIN);
        }
        std::string strAll = strHeader + strBody;
        fwrite(strAll.c_str(), std::strlen(strAll.c_str()), 1, configFile);
        fclose(configFile);
    }
}
bool CDockerPriceConfig::read(std::string& strErr) {
    int linenumber = 1;
    boost::filesystem::path pathDockerPriceConfigFile = GetDockerPriceConfigFile();
    boost::filesystem::ifstream streamConfig(pathDockerPriceConfigFile);

    if (!streamConfig.good()) {
        FILE* configFile = fopen(pathDockerPriceConfigFile.string().c_str(), "a");
        if (configFile != NULL) {
            std::string strHeader = "# DockerPrice config file\n"
                          "# price :cpu: MGD/thread mem: MGD/GB gpu: MGD/count\n"
                          "# Format: type name price(MGD/hour)\n"
                          "# Example0: cpu i7 0.2\n"
                          "# Example1: mem ddr4 0.1\n"
                          "# Example3: gpu nvidia_p106_100_6g 0.2\n";
            fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
            fclose(configFile);
        }
        return true; // Nothing to read, so just return
    }

    for(std::string line; std::getline(streamConfig, line); linenumber++)
    {
        if(line.empty()) continue;

        std::istringstream iss(line);
        std::string comment, type, name, price;

        if (iss >> comment) {
            if(comment.at(0) == '#') continue;
            iss.str(line);
            iss.clear();
        }

        if (!(iss >> type >> name >> price)) {
            iss.str(line);
            iss.clear();
            if (!(iss >> type >> name >> price)) {
                strErr = _("Could not parse dockerprice.conf") + "\n" +
                        strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"";
                streamConfig.close();
                return false;
            }
        }


        add(type, name, price);
    }

    streamConfig.close();
    return true;
}

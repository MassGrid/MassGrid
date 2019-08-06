#include "prizes_service.h"
#include "boost/lexical_cast.hpp"
#include "messagesigner.h"
#include <boost/algorithm/string.hpp>

std::string DockerDeleteService::ToString(){
    return pubKeyClusterAddress.ToString().substr(0, 66) + CrerateOutPoint.ToStringShort();
}
bool DockerDeleteService::Sign(const CKey& keyClusterAddress, const CPubKey& pubKeyClusterAddress)
{
    std::string strError;

    // TODO: add sentinel data
    sigTime = GetAdjustedTime();
    std::string strMessage = this->ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);

    if (!CMessageSigner::SignMessage(strMessage, vchSig, keyClusterAddress)) {
        LogPrintf("DockerUpdateService::Sign -- SignMessage() failed\n");
        return false;
    }
    if (!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerUpdateService::Sign -- VerifyMessage() failed, error: %s\n", strError);
        return false;
    }

    return true;
}

bool DockerDeleteService::CheckSignature(CPubKey& pubKeyClusterAddress)
{
    // TODO: add sentinel data
    std::string strMessage = this->ToString() + boost::lexical_cast<std::string>(version) + boost::lexical_cast<std::string>(sigTime);
    std::string strError = "";

    if (!CMessageSigner::VerifyMessage(pubKeyClusterAddress, vchSig, strMessage, strError)) {
        LogPrintf("DockerUpdateService::CheckSignature -- Got bad signature, error: %s\n", strError);
        return false;
    }
    return true;
}
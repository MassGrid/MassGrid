#include "msghandler.h"
#include "keyagreement.h"
#include "pubkey.h"
#include "base58.h"
#include "utilstrencodings.h"

#include "genkey.h"
#include "util.h"
#include "msghelper.h"
//sign
const std::string strMessageMagic = "MassGrid Signed Message:\n";

bool encrypto(std::string strWifPrivkey,std::string strpubkey,std::string plainText,std::string &clipText)
{
    unsigned char result[32];
     if(!MakeNewAESKey(strWifPrivkey,strpubkey,result)){
        LogPrintf("MakeNewAESKey error!\n");
        return false;
    }
    std::string tmp=CharToString(result);
    CKey key;
    key.Set(result,result+32,true);

    CPubKey pubkey=key.GetPubKey();

    std::vector<unsigned char> vchCryptedSecret;
    CKeyingMaterial vchSecret(plainText.begin(),plainText.end());
    CKeyingMaterial tmpkey = key.GetPrivKey();
    CKeyingMaterial vMasterKey(WALLET_CRYPTO_KEY_SIZE);
    memcpy(&vMasterKey[0],&tmpkey[0],WALLET_CRYPTO_KEY_SIZE);

    if (!MsgEncryptSecret(vMasterKey, vchSecret, pubkey.GetHash(), vchCryptedSecret))//
        return false;
    clipText=EncodeBase64(VecCharToStr(vchCryptedSecret));
    return true;
}
bool decrypto(std::string strWifPrivkey,std::string strpubkey,std::string plainText,std::string &clipText)
{
    unsigned char result[32];
     if(!MakeNewAESKey(strWifPrivkey,strpubkey,result)){
        LogPrintf("MakeNewAESKey error!\n");
        return false;
    }
    std::string tmp=CharToString(result);
    CKey key;
    key.Set(result,result+32,true);
    CPubKey pubkey=key.GetPubKey();
    CKeyingMaterial vchPlaintext;

    std::vector<unsigned char> vchCryptedSecret=DecodeBase64(plainText.c_str());

    CKeyingMaterial tmpkey = key.GetPrivKey();
    CKeyingMaterial vMasterKey(WALLET_CRYPTO_KEY_SIZE);
    memcpy(&vMasterKey[0],&tmpkey[0],WALLET_CRYPTO_KEY_SIZE);

    if(!MsgDecryptSecret(vMasterKey, vchCryptedSecret, pubkey.GetHash(), vchPlaintext))
        return false;
    clipText=CVecCharToStr(vchPlaintext);

    return true;
}
bool signMessage(std::string str,std::string messages,std::string &result,bool fCompressed)
{
    CMassGridSecret secret;
    secret.SetString(str);
    CKey key=secret.GetKey();
    CPubKey pubkey=key.GetPubKey();
    std::string strGenPrivkey=GetFormPrivK(key.toString(),fCompressed);
    std::string strGenPubkey=GetFormPubK(pubkey.ToString(),fCompressed);

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << messages;
    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
    {
        LogPrintf("Message signing failed\n");
        return false;
    }
    result=EncodeBase64(&vchSig[0],vchSig.size());
    return true;
}
bool signVerifyMessage(std::string address,std::string encodeMsg,std::string strMsg,bool fCompressed)
{
    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(encodeMsg.c_str(),&fInvalid);
    if (fInvalid)
    {
        LogPrintf("The signature could not be decoded. Please check the signature and try again.\n");
        return false;
    }
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMsg;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
    {
        LogPrintf("The signature did not match the message digest. Please check the signature and try again.\n");
        return false;
    }
    std::string strPubkey=GetFormPubK(pubkey.ToString(),fCompressed);
    std::string walletaddr="";
    GetAddress(strPubkey,walletaddr);
    if (!(walletaddr == address))
    {
        LogPrintf("Message verification failed.\n");
        return false;
    }
    return true;
}
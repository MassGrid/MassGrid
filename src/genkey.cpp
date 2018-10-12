
#include "genkey.h"
#include "chainparams.h"
#include "key.h"
#include "pubkey.h"
#include "random.h"


#include "util.h"
#include "hdchain.h"
#include "msghelper.h"


#include <assert.h>
//gen new key
void GenerateNewKey(std::string &strprivkey,std::string &strpubkey,bool fCompressed)
{
    CKey secret;
    secret.MakeNewKey(fCompressed);
    CKey key;
    key.Set(secret.begin(),secret.begin()+32,fCompressed);
    CPrivKey privkey = key.GetPrivKey();
    CPubKey pubkey;
    pubkey=key.GetPubKey();
    assert(key.VerifyPubKey(pubkey));
    strpubkey=GetFormPubK(pubkey.ToString(),fCompressed);
    strprivkey=GetFormPrivK(key.toString(),fCompressed);
    return;
}

void base58check(std::string privkey,std::string &wifPrivkey)
{
    CSHA256 sha256_1;
    unsigned char dest1[65],dest2[65];
    HexStrToByte(privkey.c_str(),dest1,privkey.size());
    sha256_1.Write(dest1, privkey.size()/2).Finalize((unsigned char*)&dest2);
    std::string shaStr_1="";
    shaStr_1=CharToString(dest2);
    CSHA256 sha256_2;
    sha256_2.Write(dest2,32).Finalize((unsigned char*)&dest1);
    std::string shaStr_2="";
    shaStr_2=CharToString(dest1);
    std::string append=shaStr_2.substr(0,8);
    privkey+=append;
    wifPrivkey=EncodeBase58(ParseHex(privkey));
    return;
}

void Wif(std::string privkey,std::string &wifPrivkey,bool fCompress)
{
    std::string parsefex=VecToHexStr(Params().Base58Prefix(CChainParams::SECRET_KEY));
    privkey=parsefex+privkey;
    if(fCompress) privkey+="01";
    base58check(privkey,wifPrivkey);
    return;
}

void GetAddress(std::string pubkey,std::string &waddress)
{
    unsigned char dest1[70],dest2[70];
    HexStrToByte(pubkey.c_str(),dest1,pubkey.size());

    CSHA256 sha256;
    sha256.Write(dest1, pubkey.size()/2).Finalize((unsigned char*)&dest2);
    std::string shaStr="";
    shaStr=CharToString(dest2);

    CRIPEMD160 ripemd160;
    ripemd160.Write(dest2,32).Finalize((unsigned char*)&dest1);
    std::string ripemd160Str="";
    ripemd160Str=CharToString(dest1,20);
    std::string parsefex=VecToHexStr(Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS));
    ripemd160Str=parsefex+ripemd160Str;
    base58check(ripemd160Str,waddress);
    return;
}

std::string GetFormPubK(std::string ckeys,bool fCompressed)
{
    if(fCompressed) return ckeys.substr(0,66);
    return ckeys;
}
std::string GetFormPrivK(std::string ckeys,bool fCompressed)
{
    if(fCompressed) return ckeys.substr(0,64);
    return ckeys;
}
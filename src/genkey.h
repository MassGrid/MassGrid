#ifndef MASSGRID_GENKEY_H
#define MASSGRID_GENKEY_H
#include<cstring>
#include<algorithm>
#include "crypto/common.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"
#include "base58.h"
#include "crypto/sha256.h"
#include "crypto/ripemd160.h"
#include "key.h"
#include "wallet/crypter.h"
#include "utilstrencodings.h"
#include "messagesigner.h"
#include<fstream>
#include <cstring>
#include <vector>
#include <algorithm>


std::string GetFormPubK(std::string ckeys,bool fCompressed);
std::string GetFormPrivK(std::string ckeys,bool fCompressed);

void base58check(std::string privkey,std::string &wifPrivkey);
void Wif(std::string privkey,std::string &wifPrivkey,bool fCompress);
void GetAddress(std::string pubkey,std::string &waddress);
void BrutePass(std::string fname,bool fCompressed);
void getFilePass(std::string fname,std::vector<std::string> &files);
void GenerateNewKey(std::string &strprivkey,std::string &strpubkey,bool fCompressed);
void GenerateNewHDChain(bool fCompressed);
#endif /*MASSGRID_GENKEY_H*/
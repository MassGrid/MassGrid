#ifndef MASSGRID_MSGHANDLER_H
#define MASSGRID_MSGHANDLER_H
#include <cstring>
#include <vector>
#include <algorithm>
#include "keyagreement.h"
#include "pubkey.h"
#include "base58.h"
#include "utilstrencodings.h"

#include "genkey.h"
#include "util.h"
#include "msghelper.h"
#include "key.h"
#include "wallet/crypter.h"
bool encrypto(std::string strWifPrivkey,std::string strpubkey,std::string plainText,std::string &clipText);
bool decrypto(std::string strWifPrivkey,std::string strpubkey,std::string plainText,std::string &clipText);

bool signMessage(std::string str,std::string messages,std::string &result,bool fCompressed);
bool signVerifyMessage(std::string address,std::string encodeMsg,std::string strMsg,bool fCompressed);
//test
bool encrypt_tests();
#endif //MASSGRID_MSGHANDLER_H
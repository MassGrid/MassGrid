#ifndef MASSGRID_MSGHELPER_H
#define MASSGRID_MSGHELPER_H
#include <cstring>
#include <vector>
#include <algorithm>

#include "wallet/crypter.h"
#include "keystore.h"
std::string VecToHexStr(const std::vector<unsigned char> vch);
std::string CVecCharToStr(CKeyingMaterial &vch);
void HexStrToVecChar(const char* source, std::vector<unsigned char> &vchCryptedSecret, int sourceLen);
std::string VecCharToStr(std::vector<unsigned char> &vch);
void HexStrToByte(const char* source, unsigned char* dest, int sourceLen);
std::string CharToString(unsigned char *vch,int len=32);
#endif //MASSGRID_MSGHELPER_H
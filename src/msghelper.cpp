#include "msghelper.h"

void HexStrToByte(const char* source, unsigned char* dest, int sourceLen)
{
    short i;
    unsigned char highByte, lowByte;
    
    for (i = 0; i < sourceLen; i += 2)
    {
        highByte = toupper(source[i]);
        lowByte  = toupper(source[i + 1]);
        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;


        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;


        dest[i / 2] = (highByte << 4) | lowByte;
    }
    return ;
}
/** Sanity checks
 *  Ensure that MassGrid is running in a usable environment with all
 *  necessary library support.
 */
std::string CharToString(unsigned char *vch,int len)
{
    int i;
    char szTmp[3];
    std::string str;
    for (i = 0; i < len; i++)
    {
        sprintf(szTmp, "%02x", vch[i]);
        str.push_back(szTmp[0]);
        str.push_back(szTmp[1]);
    }
    return str;
}
std::string VecToHexStr(const std::vector<unsigned char> vch)
{
    int  i;
    int nlen=vch.size();
    char szTmp[3];
    std::string str;
    for (i = 0; i < nlen; i++)
    {
        sprintf(szTmp, "%02x", vch[i]);
        str.push_back(szTmp[0]);
        str.push_back(szTmp[1]);
    }
    return str;
}
std::string CVecCharToStr(CKeyingMaterial &vch)
{
    std::string res(vch.begin(),vch.end());
    // res.insert(res.begin(),vch.begin(),vch.end());
    return res;
}
void HexStrToVecChar(const char* source, std::vector<unsigned char> &vchCryptedSecret, int sourceLen)
{
    short i;
    unsigned char highByte, lowByte;
    
    for (i = 0; i < sourceLen; i += 2)
    {
        highByte = toupper(source[i]);
        lowByte  = toupper(source[i + 1]);
        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;


        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;


        vchCryptedSecret[i / 2] = (highByte << 4) | lowByte;
    }
    return ;
}
std::string VecCharToStr(std::vector<unsigned char> &vch)
{
    std::string res(vch.begin(),vch.end());
    return res;
}
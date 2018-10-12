#ifndef MASSGRID_KEYAGREEMENT_H
#define MASSGRID_KEYAGREEMENT_H

#include<fstream>
#include <cstring>
#include <vector>
#include <algorithm>

bool MakeNewAESKey(std::string strWifPrivkey,std::string pubkeytmp,unsigned char *result);
#endif
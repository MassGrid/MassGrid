#include "crypto/w_blake.h"
#include "crypto/w_bmw.h"
#include "crypto/w_groestl.h"
#include "crypto/w_jh.h"
#include "crypto/w_keccak.h"
#include "crypto/w_skein.h"
#include "crypto/w_luffa.h"
#include "crypto/w_cubehash.h"
#include "crypto/w_shavite.h"
#include "crypto/w_simd.h"
#include "crypto/w_echo.h"
#include "crypto/w_hamsi.h"
#include "crypto/w_fugue.h"
#include "stdlib.h"
static const int hashcount=13;
static void (*jump[])(unsigned char* input, unsigned char* output)={
    blake_scanHash_post,bmw_scanHash_post,groestl_scanHash_post,skein_scanHash_post,
    jh_scanHash_post,keccak_scanHash_post,luffa_scanHash_post,cubehash_scanHash_post,
    shavite_scanHash_post, simd_scanHash_post, echo_scanHash_post, hamsi_scanHash_post, fugue_scanHash_post
};
static void Hex2Str(unsigned char *sSrc, unsigned char *sDest, int nSrcLen)
{
	int  i;
	 char szTmp[3];

	for (i = 0; i < nSrcLen; i++)
	{
		sprintf(szTmp, "%02X", (unsigned char)sSrc[i]);
		memcpy(&sDest[i * 2], szTmp, 2);
	}
	return;
}
static int getcount(){return hashcount;};
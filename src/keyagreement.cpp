#include "keyagreement.h"

#include "key.h"
#include "pubkey.h"
#include "random.h"
#include "crypto/common.h"
#include "secp256k1.h"
#include "secp256k1_ecdh.h"
#include "base58.h"
#include "crypto/sha256.h"
#include "crypto/ripemd160.h"

#include "util.h"
#include "genkey.h"
#include "msghelper.h"
//gen ecdh key
bool MakeNewAESKey(std::string strWifPrivkey,std::string pubkeytmp,unsigned char *result)
{
    CMassGridSecret secret;
    secret.SetString(strWifPrivkey);
    CKey key=secret.GetKey();//privkey
    unsigned char privkey[32];
    memcpy(privkey,key.begin(), key.size());
    unsigned char pubkey[33];
    HexStrToByte(pubkeytmp.c_str(),pubkey,66);
    
    secp256k1_context* ctxs= secp256k1_context_create(SECP256K1_FLAGS_TYPE_CONTEXT);
    assert(ctxs!=NULL);
    secp256k1_pubkey ecdh_pubkey;

    if(!(secp256k1_ec_pubkey_parse(ctxs, &ecdh_pubkey,pubkey, sizeof(pubkey)))){
        LogPrintf("secp256k1_ec_pubkey_parse error!\n");
        return false;
    }
    if(secp256k1_ecdh(ctxs,result,&ecdh_pubkey,privkey)!=1){
        LogPrintf("secp256k1_ecdh error!\n");
        return false;
    }
    return true;
}
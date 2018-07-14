// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"
#include "crypto/hashpow.h"
#include "crypto/jumphash.h"
#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "streams.h"
#include "util.h"

#include "versionbits.h"
//boost::mutex _l;
 uint256 CBlockHeader::ComputePowHash(uint32_t Nonce)const
 {
    //boost::mutex::scoped_lock lock(_l);
    CSHA256 sha256hasher,sha256hasher2;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *this;
    assert(ss.size() == 80);
    
    int32_t nVersion=(this->nVersion);

    //LogPrintf("block new version: %d\n", nVersion);
    if(nVersion<=4){  
        uint256 base,output,output2;
        sha256hasher.Write((uint8_t*)&ss[0], 76);
        CSHA256(sha256hasher).Write((uint8_t*)&Nonce, 4).Finalize((uint8_t*)&base);

        hashPow* hashp=hashPow::getinstance();
        int id1=(((uint16_t *)&base)[0])%hashp->getcount();
        int id2=(((uint16_t *)&base)[1])%hashp->getcount();
        hashp->compute(id1,(uint8_t*)&base,(uint8_t*)&output);
        hashp->compute(id2,(uint8_t*)&output,(uint8_t*)&output2);
        CScrypt256 hasher;
        uint256 powHash;
        CScrypt256(hasher).Write((uint8_t*)&output2, sizeof(output2)).Finalize((uint8_t*)&powHash);
    return powHash;
    }else if(nVersion==5){
        uint256 base,powHash;
        uint8_t output0[64],output1[64];
        sha256hasher.Write((uint8_t*)&ss[0], 76).Finalize((uint8_t*)&base);
        int id=(((uint16_t *)&this->hashPrevBlock)[0])%13;
        Hex2Str((uint8_t*)&base,output0,32);
        ((uint32_t *)output0)[14]^=((uint32_t *)output0)[15];
        ((uint32_t *)output0)[15]=Nonce;
        jump[id](output0,output1);
        sha256hasher2.Write(output1, 64).Finalize((uint8_t*)&powHash);
        return powHash;
    }else if(nVersion==6){
        uint256 base,powHash;
        uint8_t output0[64],output1[64];
        sha256hasher.Write((uint8_t*)&ss[0], 76).Finalize((uint8_t*)&base);
        int id=(((uint16_t *)&this->hashPrevBlock)[0])%18;
        Hex2Str((uint8_t*)&base,output0,32);
        ((uint32_t *)output0)[14]^=((uint32_t *)output0)[15];
        ((uint32_t *)output0)[15]=Nonce;
        jump2[id](output0,output1);
        sha256hasher2.Write(output1, 64).Finalize((uint8_t*)&powHash);
        return powHash;

    }else{
        return uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    }

}
uint256 CBlockHeader::GetHash() const
{
    return ComputePowHash(nNonce);//Hash(BEGIN(nVersion), END(nNonce));  //  return SerializeHash(*this);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i].ToString() << "\n";
    }
    return s.str();
}
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "crypto/hashpow.h"
#include "crypto/jumphash.h"
#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"

#include "streams.h"
#include "util.h"
#include <boost/thread/mutex.hpp>
boost::mutex _l;
 uint256 CBlockHeader::ComputePowHash(uint32_t Nonce)const
 {
    // boost::mutex::scoped_lock lock(_l);
    // boost::unique_lock<boost::mutex> lock(_l);
    CSHA256 sha256hasher,sha256hasher2;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *this;
    assert(ss.size() == 80);
    if(this->nVersion<=4){  
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
    }else if(this->nVersion==5){
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
    }else{return ~(uint256)0;}

}
uint256 CBlockHeader::GetHash() const
{
    return ComputePowHash(nNonce);//Hash(BEGIN(nVersion), END(nNonce));
}
uint256 CBlockHeader::GetpowHash() const
{
    return Hash(BEGIN(nVersion), END(nNonce));
}

uint256 CBlock::BuildMerkleTree(bool* fMutated) const
{
    /* WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given time
       is odd, the last one is duplicated before computing the next level (which
       is unusual in Merkle trees). This results in certain sequences of
       transactions leading to the same merkle root. For example, these two
       trees:

                    A               A
                  /  \            /   \
                B     C         B       C
               / \    |        / \     / \
              D   E   F       D   E   F   F
             / \ / \ / \     / \ / \ / \ / \
             1 2 3 4 5 6     1 2 3 4 5 6 5 6

       for transaction lists [1,2,3,4,5,6] and [1,2,3,4,5,6,5,6] (where 5 and
       6 are repeated) result in the same root hash A (because the hash of both
       of (F) and (F,F) is C).

       The vulnerability results from being able to send a block with such a
       transaction list, with the same merkle root, and the same block hash as
       the original without duplication, resulting in failed validation. If the
       receiving node proceeds to mark that block as permanently invalid
       however, it will fail to accept further unmodified (and thus potentially
       valid) versions of the same block. We defend against this by detecting
       the case where we would hash two identical hashes at the end of the list
       together, and treating that identically to the block having an invalid
       merkle root. Assuming no double-SHA256 collisions, this will detect all
       known ways of changing the transactions without affecting the merkle
       root.
    */
    vMerkleTree.clear();
    vMerkleTree.reserve(vtx.size() * 2 + 16); // Safe upper bound for the number of total nodes.
    for (std::vector<CTransaction>::const_iterator it(vtx.begin()); it != vtx.end(); ++it)
        vMerkleTree.push_back(it->GetHash());
    int j = 0;
    bool mutated = false;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            if (i2 == i + 1 && i2 + 1 == nSize && vMerkleTree[j+i] == vMerkleTree[j+i2]) {
                // Two identical hashes at the end of the list at a particular level.
                mutated = true;
            }
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
                                       BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
        }
        j += nSize;
    }
    if (fMutated) {
        *fMutated = mutated;
    }
    return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
}

std::vector<uint256> CBlock::GetMerkleBranch(int nIndex) const
{
    if (vMerkleTree.empty())
        BuildMerkleTree();
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

uint256 CBlock::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
    if (nIndex == -1)
        return 0;
    for (std::vector<uint256>::const_iterator it(vMerkleBranch.begin()); it != vMerkleBranch.end(); ++it)
    {
        if (nIndex & 1)
            hash = Hash(BEGIN(*it), END(*it), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(*it), END(*it));
        nIndex >>= 1;
    }
    return hash;
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
    s << "  vMerkleTree: ";
    for (unsigned int i = 0; i < vMerkleTree.size(); i++)
        s << " " << vMerkleTree[i].ToString();
    s << "\n";
    return s.str();
}

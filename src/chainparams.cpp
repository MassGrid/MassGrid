// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>
#include <stdio.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
static Checkpoints::MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        //( 0, uint256("0x001"));
	( 0, uint256("0x0000000020bc2c5ec220e3f660c5a9b59ff2f21ca054bcbe8c207eaa0292cce2"));
	/*( 11111, uint256("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d"))
        ( 33333, uint256("0x000000002dd5588a74784eaa7ab0507a18ad16a236e7b1ce69f00d7ddfb5d0a6"))
        ( 74000, uint256("0x0000000000573993a3c9e41ce34471c079dcf5f52a0e824a81e7f953b8661a20"))
        (105000, uint256("0x00000000000291ce28027faea320c8d2b054b2e0fe44a773f3eefb151d6bdc97"))
        (134444, uint256("0x00000000000005b12ffd4cd315cd34ffd4a594f430ac814c91184a0d42d2b0fe"))
        (168000, uint256("0x000000000000099e61ea72015e79632f216fe6cb33d7899acb35b75c8303b763"))
        (193000, uint256("0x000000000000059f452a5f7340de6682a977387c17010ff6e6c3bd83ca8b1317"))
        (210000, uint256("0x000000000000048b95347e83192f69cf0366076336c639f9b7228e9ba171342e"))
        (216116, uint256("0x00000000000001b4f4b433e81ee46494af945cf96014816a4e2370f11b23df4e"))
        (225430, uint256("0x00000000000001c108384350f74090433e7fcf79a606b8e797f065b130575932"))
        (250000, uint256("0x000000000000003887df1f29024b06fc2200b55f8af8f35453d7be294df2d214"))
        (279000, uint256("0x0000000000000001ae8c72a0b0c301f67e3afca10e819efa9041e458e9bd7e40"))
        (295000, uint256("0x00000000000000004d9b4ef50f0f9d686fd69db2e03af35a100370c64632a983"))
        ;
*/
static const Checkpoints::CCheckpointData data = {
        &mapCheckpoints,
        1501262349, // * UNIX timestamp of last checkpoint block
        0,   // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        60000.0     // * estimated number of transactions per day after checkpoint
    };

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        //( 0, uint256("0x001"));
	( 0, uint256("0x0000000020bc2c5ec220e3f660c5a9b59ff2f21ca054bcbe8c207eaa0292cce2"))
        ;
static const Checkpoints::CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1501262349,
        0,
        300
    };

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256("0x001"));
//( 0, uint256("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"))
        ;
static const Checkpoints::CCheckpointData dataRegtest = {
        &mapCheckpointsRegtest,
        0,
        0,
        0
    };

class CMainParams : public CChainParams {
public:
    CMainParams() {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /** 
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0x35;
        pchMessageStart[1] = 0x65;
        pchMessageStart[2] = 0x01;
        pchMessageStart[3] = 0x22;
        vAlertPubKey = ParseHex("04a48a5a9ee0c9837b38e3cfe558883893cf5f6beb264b0055caaf7ddb552621ff8e3b06f551b7e298020e7f3949b1c14d91c7b9bcb4d63cb442c1a801011be49a");
        nDefaultPort = 9443;
        bnProofOfWorkLimit = ~uint256(0) >> 20;
        nSubsidyHalvingInterval = 210000;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0;
        nTargetTimespan = 60 * 60* 5; // two weeks
        nTargetSpacing = 5 * 60;
        nMaxTipAge = 24 * 60 * 60;

        /**
         * Build the genesis block. Note that the output of the genesis coinbase cannot
         * be spent as it did not originally exist in the database.
         * 
         * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
         *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
         *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
         *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
         *   vMerkleTree: 4a5e1e
         */
        const char* pszTimestamp = "MLGB create first block in Shenzhen, on 27th July., 2017";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("0486661df18672bc959f622d09ad550f56154a4b3c812671ea601aff934324ed1cf8457b9015290d3c94fb6c140e92f3c1a59dddb07e49a12df41b2f2ea687b8e6") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        // genesis.nTime    = 1501262349;
        // genesis.nBits    = 0x1d00ffff;
        // genesis.nNonce   = 3131586093;//-1163381203;


        // hashGenesisBlock = genesis.GetHash();
        // assert(hashGenesisBlock == uint256("0x0000000020bc2c5ec220e3f660c5a9b59ff2f21ca054bcbe8c207eaa0292cce2"));

        genesis.nTime    = 1506050827;
        genesis.nBits    = 0x1e0ffff0;
        genesis.nNonce   = 17367;//-1163381203;

        // genesis.nTime=GetTime();
        // genesis.nNonce=0;
        // while(true)
        // {
        //     genesis.nNonce++;

        //     if(genesis.GetHash()<uint256("0x00000ffff412dfe38e544f6628adfc756cff9486679e51027a14ab65946751ab")&&genesis.GetHash()!=0)
        //     {
        //         printf("hash :%s \nnTime:%d\nnNonce:%d\n",genesis.GetHash().GetHex().c_str(),genesis.nTime,genesis.nNonce);
        //     }
        //     if ((genesis.nNonce& 0xffff) == 0)
        //     {
        //         printf("run%d",genesis.nNonce);
        //         genesis.nTime=GetTime();
        //         genesis.nNonce=0;
        //     }
        //    // printf("run%d",genesis.nNonce);
        // }
        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x000000000029a36746cc135f8ef8ae452a79b7f5d18a25e6f1fcd59cb39bf9a3bd08b"));
        assert(genesis.hashMerkleRoot == uint256("0x010150a88cf516ade90a91f9198bc80eb59a110134c1f84abe75377165f82dc0"));

        /*vSeeds.push_back(CDNSSeedData("mlgbcoin.sipa.be", "seed.mlgbcoin.sipa.be"));
        vSeeds.push_back(CDNSSeedData("bluematt.me", "dnsseed.bluematt.me"));
        vSeeds.push_back(CDNSSeedData("dashjr.org", "dnsseed.mlgbcoin.dashjr.org"));
        vSeeds.push_back(CDNSSeedData("mlgbcoinstats.com", "seed.mlgbcoinstats.com"));
        vSeeds.push_back(CDNSSeedData("xf2.org", "bitseed.xf2.org"));
*/
	vFixedSeeds.clear();
	vSeeds.clear();


        base58Prefixes[PUBKEY_ADDRESS] = list_of(50);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(38);
        base58Prefixes[SECRET_KEY] =     list_of(25);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4);

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0x87;
        pchMessageStart[1] = 0x81;
        pchMessageStart[2] = 0x35;
        pchMessageStart[3] = 0x25;
        vAlertPubKey = ParseHex("0485d7e5aaf9e52cf4b6b951f2c661be84e94cdb728fccb213174001acdf0f0c6d0faf4b1ac37593b8f8a3e4302e7ffea88f58eef50c78ae1d8b62cbb6d9c732c8");
        nDefaultPort = 19443;
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetTimespan = 60 * 60; //! two weeks
        nTargetSpacing = 1 * 60;
        nMaxTipAge = 0x7fffffff;

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        // genesis.nTime = 1501262349;
        // genesis.nNonce = 3131586093;//-1163381203;



        // hashGenesisBlock = genesis.GetHash();
        // assert(hashGenesisBlock == uint256("0x0000000020bc2c5ec220e3f660c5a9b59ff2f21ca054bcbe8c207eaa0292cce2"));
        genesis.nTime    = 1506050827;
        genesis.nNonce   = 17367;//-1163381203;
        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0000029a36746cc135f8ef8ae452a79b7f5d18a25e6f1fcd59cb39bf9a3bd08b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        /*vSeeds.push_back(CDNSSeedData("alexykot.me", "testnet-seed.alexykot.me"));
        vSeeds.push_back(CDNSSeedData("mlgbcoin.petertodd.org", "testnet-seed.mlgbcoin.petertodd.org"));
        vSeeds.push_back(CDNSSeedData("bluematt.me", "testnet-seed.bluematt.me"));
        vSeeds.push_back(CDNSSeedData("mlgbcoin.schildbach.de", "testnet-seed.mlgbcoin.schildbach.de"));
	*/

        base58Prefixes[PUBKEY_ADDRESS] = list_of(111);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(196);
        base58Prefixes[SECRET_KEY]     = list_of(239);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94);

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        pchMessageStart[0] = 0x45;
        pchMessageStart[1] = 0x65;
        pchMessageStart[2] = 0x76;
        pchMessageStart[3] = 0x10;
        nSubsidyHalvingInterval = 150;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan =  60 * 60; //! 1 minutes
        nTargetSpacing = 1 * 60;
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        nMaxTipAge = 24 * 60 * 60;
        // genesis.nTime = 1501262349;
        // genesis.nBits = 0x207fffff;
        // genesis.nNonce = 2;


        // hashGenesisBlock = genesis.GetHash();
        // nDefaultPort = 18444;
        // assert(hashGenesisBlock == uint256("0x614e2ffb27342a41cedb1762ba3d17783668fc7d1fd27501e5baa2eac3ba367a"));

        genesis.nTime = 1506050827;
        genesis.nBits = 0x1e0ffff0;
        genesis.nNonce = 17367;//-1163381203;


        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        assert(hashGenesisBlock == uint256("0x0000029a36746cc135f8ef8ae452a79b7f5d18a25e6f1fcd59cb39bf9a3bd08b"));
        
        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams {
public:
    CUnitTestParams() {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 18445;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Unit test mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval)  { nSubsidyHalvingInterval=anSubsidyHalvingInterval; }
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority)  { nEnforceBlockUpgradeMajority=anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority)  { nRejectBlockOutdatedMajority=anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority)  { nToCheckBlockUpgradeMajority=anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks)  { fDefaultConsistencyChecks=afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) {  fAllowMinDifficultyBlocks=afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams *pCurrentParams = 0;

CModifiableParams *ModifiableParams()
{
   assert(pCurrentParams);
   assert(pCurrentParams==&unitTestParams);
   return (CModifiableParams*)&unitTestParams;
}

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams &Params(CBaseChainParams::Network network) {
    switch (network) {
        case CBaseChainParams::MAIN:
            return mainParams;
        case CBaseChainParams::TESTNET:
            return testNetParams;
        case CBaseChainParams::REGTEST:
            return regTestParams;
        case CBaseChainParams::UNITTEST:
            return unitTestParams;
        default:
            assert(false && "Unimplemented network");
            return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network) {
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}

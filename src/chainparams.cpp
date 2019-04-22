// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"
// #include "arith_uint256.h"
static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "MLGB create first block in Shenzhen, on 27th July., 2017";
    const CScript genesisOutputScript = CScript() << ParseHex("0486661df18672bc959f622d09ad550f56154a4b3c812671ea601aff934324ed1cf8457b9015290d3c94fb6c140e92f3c1a59dddb07e49a12df41b2f2ea687b8e6") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */


class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 420768;   //4 years
        consensus.nMasternodePaymentsStartBlock = 111000; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 17568; // actual historical value 2 month 288*61
        consensus.nInstantSendKeepLock = 24;
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256S("0x000007d91d1254d60e2dd1ae580383070a4ddffa4c64c2eeb4a2f9ecc0414343");
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 24 * 60 * 60; // MassGrid: 1 day
        consensus.nPowTargetSpacing = 5 * 60; // MassGrid: 5 minutes
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false; // Retargeting

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000013ceafdd3e7344d8"); // 77922  // total POW

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000000000019144f85ab6fb476b11bea0c87f4bf2915873e04d7c9ad8736ff5"); // 77922

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x35;
        pchMessageStart[1] = 0x65;
        pchMessageStart[2] = 0x01;
        pchMessageStart[3] = 0x22;
        vAlertPubKey = ParseHex("038CEB40FA498FFEE9C53DDAA72ACD65048BCDDC752C796C7AECBBBA377A733D1D");
        nDefaultPort = 9443;
        nMaxTipAge = 24 * 60 * 60;
        nDelayGetHeadersTime = 24 * 60 * 60;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1507956294, 53408, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000006cda968d9b220b264050676efed86e2db52e29619ed3ef94fcf23cd86f4"));
        assert(genesis.hashMerkleRoot == uint256S("0x010150a88cf516ade90a91f9198bc80eb59a110134c1f84abe75377165f82dc0"));

        vFixedSeeds.clear();
        vSeeds.clear();

        vSeeds.push_back(CDNSSeedData("seed1.massgrid.net", "seed1.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("seed2.massgrid.net", "seed2.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("seed3.massgrid.net", "seed3.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("seed4.massgrid.net", "seed4.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("seed5.massgrid.net", "seed5.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("seed6.massgrid.net", "seed6.massgrid.net"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,50);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,38);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,25);
        // MassGrid BIP32 pubkeys start with 'xpub' (MassGrid defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        // MassGrid BIP32 prvkeys start with 'xprv' (MassGrid defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // MassGrid BIP44 coin type is '5'
        nExtCoinType = 5;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour
        strSporkPubKey = "038CEB40FA498FFEE9C53DDAA72ACD65048BCDDC752C796C7AECBBBA377A733D1D";

          checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 77922, uint256S("0x0000000000019144f85ab6fb476b11bea0c87f4bf2915873e04d7c9ad8736ff5")),
            1530526800, // * UNIX timestamp of last checkpoint block
            0,   // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            60000.0     // * estimated number of transactions per day after checkpoint
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 1;
        consensus.nMasternodePaymentsStartBlock = 100; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 87840; // actual historical value 2 month 1440*61
        consensus.nInstantSendKeepLock = 6;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256S("0x000007d91d1254d60e2dd1ae580383070a4ddffa4c64c2eeb4a2f9ecc0414343");
        consensus.powLimit = uint256S("0x00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      	consensus.nPowTargetTimespan = 60 * 60; //60 blocks
        consensus.nPowTargetSpacing = 1 * 60;    //a minutes
        consensus.fPowAllowMinDifficultyBlocks = true; // AllowMinDifficultyBlocks
        consensus.fPowNoRetargeting = false; // // Retargeting
        consensus.nMinimumChainWork = uint256S("0x00"); // 00

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00"); // 0

        pchMessageStart[0] = 0x87;
        pchMessageStart[1] = 0x81;
        pchMessageStart[2] = 0x35;
        pchMessageStart[3] = 0x25;
        vAlertPubKey = ParseHex("03E85467AF94A912DB61CABB54D6CBB08A5148A97D69024657665744AB8EA559C5");
        nDefaultPort = 19443;
        nMaxTipAge = 0x7fffffff;
        nMaxTipAge = 0x7fffffff;
        nPruneAfterHeight = 1000;

        // /*get testnet genesis block*/
        // uint32_t nonce=0;
        // int64_t time=GetTime();
        // for(;UintToArith256(genesis.GetHash()) > arith_uint256().SetCompact(0x1e0ffff0);++nonce){
        //     genesis = CreateGenesisBlock(time, nonce, 0x1e0ffff0, 1, 50 * COIN);
        //     if ((nonce& 0xffff) == 0)
        //      {
        //          std::cout<<"run out"<<std::endl;
        //          time=GetTime();
        //          nonce=0;
        //      }
        // }
        //     --nonce;
        //     std::cout<<"result : "<<genesis.GetHash().ToString()<<std::endl<<
        //     "time : "<<time<<std::endl<<"nonce : "<<nonce<<std::endl<<
        //     "target : "<<arith_uint256().SetCompact(0x1e0ffff0).GetHex()<<std::endl;
        //  /*get testnet genesis block*/

        genesis = CreateGenesisBlock(1551684552, 46315, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000004d4842fcce4b741b48e0dffdb721ae326954dcbdd68e1788cf9c0a3e4bf"));
		assert(genesis.hashMerkleRoot == uint256S("0x010150a88cf516ade90a91f9198bc80eb59a110134c1f84abe75377165f82dc0"));
        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("testseed1.massgrid.net", "testseed1.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("testseed2.massgrid.net", "testseed2.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("testseed3.massgrid.net", "testseed3.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("testseed4.massgrid.net", "testseed4.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("testseed5.massgrid.net", "testseed5.massgrid.net"));
        vSeeds.push_back(CDNSSeedData("testseed6.massgrid.net", "testseed6.massgrid.net"));


        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet MassGrid BIP32 pubkeys start with 'tpub' (MassGrid defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet MassGrid BIP32 prvkeys start with 'tprv' (MassGrid defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet MassGrid BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        strSporkPubKey = "03E85467AF94A912DB61CABB54D6CBB08A5148A97D69024657665744AB8EA559C5";

          checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x0000000020bc2c5ec220e3f660c5a9b59ff2f21ca054bcbe8c207eaa0292cce2")),
            1501262349,
            0,
            300
        };
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMasternodePaymentsStartBlock = 240;
        consensus.nMasternodePaymentsIncreaseBlock = 87840;
        consensus.nInstantSendKeepLock = 6;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 1 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0x45;
        pchMessageStart[1] = 0x65;
        pchMessageStart[2] = 0x76;
        pchMessageStart[3] = 0x10;
        nMaxTipAge = 24 * 60 * 60;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1506050827, 17367, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000029a36746cc135f8ef8ae452a79b7f5d18a25e6f1fcd59cb39bf9a3bd08b"));
        assert(genesis.hashMerkleRoot == uint256S("0x010150a88cf516ade90a91f9198bc80eb59a110134c1f84abe75377165f82dc0"));

        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0x001")),
            0,
            0,
            0
        };
        // Regtest MassGrid addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Regtest MassGrid BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;
   }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

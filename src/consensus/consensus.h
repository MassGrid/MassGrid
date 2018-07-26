// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_CONSENSUS_CONSENSUS_H
#define MASSGRID_CONSENSUS_CONSENSUS_H

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_LEGACY_BLOCK_SIZE = 1000000;
static const unsigned int MAX_DIP0001_BLOCK_SIZE = 2000000;


static const unsigned int MAX_BLOCK_SIZE = 1000000;

static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;

/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 50;

/** Flags for nSequence and nLockTime locks */
enum {
    /* Interpret sequence numbers as relative lock-time constraints. */
    LOCKTIME_VERIFY_SEQUENCE = (1 << 0),

    /* Use GetMedianTimePast() instead of nTime for end point timestamp. */
    LOCKTIME_MEDIAN_TIME_PAST = (1 << 1),
};

#endif // MASSGRID_CONSENSUS_CONSENSUS_H

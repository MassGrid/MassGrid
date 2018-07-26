// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MASSGRID_ZMQ_ZMQCONFIG_H
#define MASSGRID_ZMQ_ZMQCONFIG_H

#if defined(HAVE_CONFIG_H)
#include "config/massgrid-config.h"
#endif

#include <stdarg.h>
#include <string>

#if ENABLE_ZMQ
#include <zmq.h>
#endif

#include "primitives/block.h"
#include "primitives/transaction.h"

void zmqError(const char *str);

#endif // MASSGRID_ZMQ_ZMQCONFIG_H

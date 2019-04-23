// Copyright (c) 2017-2019 The MassGrid developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef DOCKERSUPERNODE_H
#define DOCKERSUPERNODE_H
#include "util.h"
void ThreadSnStart();
static int DEFAULT_SN_PORT = 8999;
void SetSNPort(int port);
int GetSNPort();
#endif //DOCKERSUPERNODE_H
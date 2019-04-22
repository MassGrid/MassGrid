#ifndef DOCKERSUPERNODE_H
#define DOCKERSUPERNODE_H
#include "util.h"
void ThreadSnStart();
static int DEFAULT_SN_PORT = 8999;
void SetSNPort(int port);
int GetSNPort();
#endif //DOCKERSUPERNODE_H
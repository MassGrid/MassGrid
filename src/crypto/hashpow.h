
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <ctime>

#include "crypto/sph_blake.h"
#include "crypto/sph_bmw.h"
#include "crypto/sph_groestl.h"
#include "crypto/sph_jh.h"
#include "crypto/sph_keccak.h"
#include "crypto/sph_skein.h"
#include "crypto/sph_luffa.h"
#include "crypto/sph_cubehash.h"
#include "crypto/sph_shavite.h"
#include "crypto/sph_simd.h"
#include "crypto/sph_echo.h"
#include "crypto/sph_hamsi.h"
#include "crypto/sph_fugue.h"
class hashPow
{
protected:
	const int count=13;
	static hashPow* _p;
	sph_blake512_context     ctx_blake;
	sph_bmw512_context       ctx_bmw;
	sph_groestl512_context   ctx_groestl;
	sph_skein512_context     ctx_skein;
	sph_jh512_context        ctx_jh;
	sph_keccak512_context    ctx_keccak;
	sph_luffa512_context	ctx_luffa1;
	sph_cubehash512_context	ctx_cubehash1;
	sph_shavite512_context	ctx_shavite1;
	sph_simd512_context		ctx_simd1;
	sph_echo512_context		ctx_echo1;
	sph_hamsi512_context	ctx_hamsi1;
	sph_fugue512_context	ctx_fugue1;
	hashPow();
public:
	static hashPow* getinstance();
	void compute(int id, unsigned char * input, unsigned char * hash);
	//void write(int id, unsigned char * input);
	int getcount(){return count;};
	//void addFinalize(int id, int nNonce, unsigned char* hash);
	
}; 
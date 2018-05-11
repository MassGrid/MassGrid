/*
* blake256 kernel implementation.
*
* ==========================(LICENSE BEGIN)============================
* Copyright (c) 2014 djm34
* Copyright (c) 2014 tpruvot
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* ===========================(LICENSE END)=============================
*
* @author   djm34
*/

#include "util_hash.cl"


__constant static const int sigma[16][16] = {
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
		{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
		{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
		{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
		{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
		{ 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
		{ 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
		{ 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
		{ 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
		{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
		{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
		{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
		{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 }
};


__constant  const uint  c_IV256[8] = {
	SPH_C32(0x6A09E667), SPH_C32(0xBB67AE85),
	SPH_C32(0x3C6EF372), SPH_C32(0xA54FF53A),
	SPH_C32(0x510E527F), SPH_C32(0x9B05688C),
	SPH_C32(0x1F83D9AB), SPH_C32(0x5BE0CD19)
};

/* Second part (64-80) msg never change, store it */
__constant  const uint  c_Padding[16] = {
	0, 0, 0, 0,
	SPH_C32(0x80000000), 0, 0, 0,
	0, 0, 0, 0,
	0, 1, 0, 640,
};
__constant  const uint  c_u256[16] = {
	SPH_C32(0x243F6A88), SPH_C32(0x85A308D3),
	SPH_C32(0x13198A2E), SPH_C32(0x03707344),
	SPH_C32(0xA4093822), SPH_C32(0x299F31D0),
	SPH_C32(0x082EFA98), SPH_C32(0xEC4E6C89),
	SPH_C32(0x452821E6), SPH_C32(0x38D01377),
	SPH_C32(0xBE5466CF), SPH_C32(0x34E90C6C),
	SPH_C32(0xC0AC29B7), SPH_C32(0xC97C50DD),
	SPH_C32(0x3F84D5B5), SPH_C32(0xB5470917)
};
// rotate SPH_ROTL32
#define GS(a,b,c,d,x) { \
	const uint idx1 = sigma[r][x]; \
	const uint idx2 = sigma[r][x+1]; \
	v[a] += (m[idx1] ^ c_u256[idx2]) + v[b]; \
	v[d] ^= v[a]; \
    v[d] = rotate (v[d], 16U); \
	v[c] += v[d]; \
    v[b] ^= v[c]; \
	v[b] = rotate (v[b], 20U); \
\
	v[a] += (m[idx2] ^ c_u256[idx1]) + v[b]; \
    v[d] ^= v[a]; \
	v[d] = rotate (v[d], 24U); \
	v[c] += v[d]; \
    v[b] ^= v[c]; \
	v[b] = rotate (v[b], 25U); \
}

void blake256(hash_t* hash)
{
 //uint gid = get_global_id(0);
 //__global hash_t *hash = (__global hash_t *)(hashes + (4 * sizeof(ulong)* (gid - get_global_offset(0))));
//  __global hash_t *hash = &(hashes[gid-get_global_offset(0)]);

    unsigned int h[8];
	unsigned int m[16];
	unsigned int v[16];


	h[0]=hash->h4[0];
	h[1]=hash->h4[1];
	h[2]=hash->h4[2];
	h[3]=hash->h4[3];
	h[4]=hash->h4[4];
	h[5]=hash->h4[5];
	h[6]=hash->h4[6];
	h[7]=hash->h4[7];
	// compress 2nd round, last 12 bytes of original message

	 m[0] = hash->h4[13];
	 m[1] = hash->h4[14];
	 m[2] = hash->h4[15];
	 m[3] = SWAP4(1);

		for (int i = 4; i < 16; i++) {m[i] = c_Padding[i];}

		for (int i = 0; i < 8; i++) {v[i] = h[i];}

		v[8] =  c_u256[0];
		v[9] =  c_u256[1];
		v[10] = c_u256[2];
		v[11] = c_u256[3];
		v[12] = c_u256[4] ^ 640;
		v[13] = c_u256[5] ^ 640;
		v[14] = c_u256[6];
		v[15] = c_u256[7];

		for (int r = 0; r < 14; r++) 
		{	
			GS(0, 4, 0x8, 0xC, 0x0);
			GS(1, 5, 0x9, 0xD, 0x2);
			GS(2, 6, 0xA, 0xE, 0x4);
			GS(3, 7, 0xB, 0xF, 0x6);
			GS(0, 5, 0xA, 0xF, 0x8);
			GS(1, 6, 0xB, 0xC, 0xA);
			GS(2, 7, 0x8, 0xD, 0xC);
			GS(3, 4, 0x9, 0xE, 0xE);
		}

		for (int i = 0; i < 16; i++) 
		{
			int j = i & 7;
			h[j] ^= v[i];
		}

	for (int i=0;i<8;i++) 
	{
		hash->h4[i]=SWAP4(h[i]);
	}

	barrier(CLK_LOCAL_MEM_FENCE);
}
__kernel void scanHash_pre(__global char* input, __global char* output, const uint nonceStart)
{
	uint gid = get_global_id(0);
	//todo : assemble hash data inside kernel with nonce
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	hashdata.h4[14] = hashdata.h4[14] ^ hashdata.h4[15];
	hashdata.h4[15] = gid + nonceStart;

	blake256(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[64 * gid + i] = hashdata.h1[i];
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}
__kernel void scanHash_post(__global char* input, __global char* output, __global uint* goodNonce, const ulong target)
{
	uint gid = get_global_id(0);

	//todo : assemble hash data inside kernel with nonce
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[64 * gid + i];
	}
	blake256(&hashdata);

	ulong outcome = hashdata.h8[3];
	bool result = (outcome <= target);

	if (result) {
		//printf("gid %d hit target!\n", gid);
		goodNonce[0] = gid;
		for (int i = 0; i < 64; i++) {
			output[i] = hashdata.h1[i];
		}
		//[atomic_inc(output + (0xFF)] = SWAP4(gid);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}
__kernel void scanHash_check_pre(__global char* input, __global char* output, const uint nonce)
{
	uint gid = get_global_id(0);
	//todo : assemble hash data inside kernel with nonce
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	hashdata.h4[14] = hashdata.h4[14] ^ hashdata.h4[15];
	hashdata.h4[15] = nonce;

	blake256(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[64 * gid + i] = hashdata.h1[i];
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}


__kernel void hashTest(__global char* in, __global char* out)
{
	hash_t input;
	for (int i = 0; i < 64; i++) {
		input.h1[i] = in[i];
	}

	blake256(&input);

	for (int i = 0; i < 64; i++) {
		out[i] = input.h1[i];
	}
}


__kernel void helloworld(__global char* in, __global char* out)
{
	int num = get_global_id(0);
	out[num] = in[num] + 1;
}

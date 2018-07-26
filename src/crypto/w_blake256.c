/*
* Lyra2 kernel implementation.
*
* ==========================(LICENSE BEGIN)============================
* Copyright (c) 2014 djm34
*
*
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
#ifdef __cplusplus
extern "C"{
#endif
#include "w_blake256.h"
#include "wutil.h"
#define uint sph_u32

static const int sigma[16][16] = {
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


static const uint  c_IV256[8] = {
	SPH_C32(0x6A09E667), SPH_C32(0xBB67AE85),
	SPH_C32(0x3C6EF372), SPH_C32(0xA54FF53A),
	SPH_C32(0x510E527F), SPH_C32(0x9B05688C),
	SPH_C32(0x1F83D9AB), SPH_C32(0x5BE0CD19)
};

/* Second part (64-80) msg never change, store it */
static const uint  c_Padding[16] = {
	0, 0, 0, 0,
	SPH_C32(0x80000000), 0, 0, 0,
	0, 0, 0, 0,
	0, 1, 0, 640,
};
static const uint  c_u256[16] = {
	SPH_C32(0x243F6A88), SPH_C32(0x85A308D3),
	SPH_C32(0x13198A2E), SPH_C32(0x03707344),
	SPH_C32(0xA4093822), SPH_C32(0x299F31D0),
	SPH_C32(0x082EFA98), SPH_C32(0xEC4E6C89),
	SPH_C32(0x452821E6), SPH_C32(0x38D01377),
	SPH_C32(0xBE5466CF), SPH_C32(0x34E90C6C),
	SPH_C32(0xC0AC29B7), SPH_C32(0xC97C50DD),
	SPH_C32(0x3F84D5B5), SPH_C32(0xB5470917)
};

#define GS(a,b,c,d,x) { \
	const uint idx1 = sigma[r][x]; \
	const uint idx2 = sigma[r][x+1]; \
	v[a] += (m[idx1] ^ c_u256[idx2]) + v[b]; \
	v[d] ^= v[a]; \
    v[d] = SPH_ROTL32(v[d], 16U); \
	v[c] += v[d]; \
    v[b] ^= v[c]; \
	v[b] = SPH_ROTL32(v[b], 20U); \
\
	v[a] += (m[idx2] ^ c_u256[idx1]) + v[b]; \
    v[d] ^= v[a]; \
	v[d] = SPH_ROTL32(v[d], 24U); \
	v[c] += v[d]; \
    v[b] ^= v[c]; \
	v[b] = SPH_ROTL32(v[b], 25U); \
}

void blake256_s(sph_s32* input,sph_s32 *output)
{
 //uint gid = get_global_id(0);
 //__global hash_t *hash = (__global hash_t *)(hashes + (4 * sizeof(ulong)* (gid - get_global_offset(0))));

//  __global hash_t *hash = &(hashes[gid-get_global_offset(0)]);

	sph_u32 h[8];
	sph_u32 m[16];
	sph_u32 v[16];

	for (int u = 0; u < 8; u++)
	{
		h[u] = input[u];
	}
	// compress 2nd round, last 12 bytes of original message

	 m[0] = input[13];
	 m[1] = input[14];
	 m[2] = input[15];
	 SWAP4(1,m[3]);

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
		output[i]=h[i];
	}
	//barrier(CLK_LOCAL_MEM_FENCE);
}
void blake256(hash_t* hash)
{
	sph_s32 input1[16],input2[16];
	sph_s32 output1[8],output2[8];
	for (int i = 0; i < 16; i++)
	{
		input1[i] = hash->h4[i];
		input2[15 - i] = input1[i];
	}
	blake256_s(input1,&output1);
	blake256_s(input2, &output2);
	for (int i = 0; i < 8; i++)
	{
		SWAP4(output1[i], hash->h4[i]);
		SWAP4(output2[i], hash->h4[i+8]);
	}
	//barrier(CLK_LOCAL_MEM_FENCE);

}
void blake256_scanHash_pre(unsigned char* input, unsigned char* output, const unsigned int nonce)
{
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	hashdata.h4[14] = hashdata.h4[14] ^ hashdata.h4[15];
	hashdata.h4[15] = nonce;
	blake256(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[i] = hashdata.h1[i];
	}
}
void blake256_scanHash_post(unsigned char* input, unsigned char* output)
{
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	blake256(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[i] = hashdata.h1[i];
	}
}
#ifdef __cplusplus
}
#endif

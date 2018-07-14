/* $Id: panama.c 216 2010-06-08 09:46:57Z tp $ */
/*
 * PANAMA implementation.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
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
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */
 
#ifdef __cplusplus
extern "C" {
#endif

#include "crypto/w_panama.h"
#include "crypto/wutil.h"


	 void sph_enc32be(void *dst, sph_u32 val)
	{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_LITTLE_ENDIAN
		SWAP4(val,val);
#endif
		*(sph_u32 *)dst = val;
#else
		if (((SPH_UPTR)dst & 3) == 0) {
#if SPH_LITTLE_ENDIAN
			SWAP4(val,val);
#endif
			*(sph_u32 *)dst = val;
		}
		else {
			((unsigned char *)dst)[0] = (val >> 24);
			((unsigned char *)dst)[1] = (val >> 16);
			((unsigned char *)dst)[2] = (val >> 8);
			((unsigned char *)dst)[3] = val;
		}
#endif
#else
		((unsigned char *)dst)[0] = (val >> 24);
		((unsigned char *)dst)[1] = (val >> 16);
		((unsigned char *)dst)[2] = (val >> 8);
		((unsigned char *)dst)[3] = val;
#endif
	}

	 void sph_enc32le(void *dst, sph_u32 val)
	{
#if defined SPH_UPTR
#if SPH_UNALIGNED
#if SPH_BIG_ENDIAN
		val = SWAP4(val);
#endif
		*(sph_u32 *)dst = val;
#else
		if (((SPH_UPTR)dst & 3) == 0) {
#if SPH_BIG_ENDIAN
			val = SWAP4(val);
#endif
			*(sph_u32 *)dst = val;
	}
		else {
			((unsigned char *)dst)[0] = val;
			((unsigned char *)dst)[1] = (val >> 8);
			((unsigned char *)dst)[2] = (val >> 16);
			((unsigned char *)dst)[3] = (val >> 24);
		}
#endif
#else
		((unsigned char *)dst)[0] = val;
		((unsigned char *)dst)[1] = (val >> 8);
		((unsigned char *)dst)[2] = (val >> 16);
		((unsigned char *)dst)[3] = (val >> 24);
#endif
	}

	 sph_u32 sph_dec32le_aligned(const void *src)
	{
#if SPH_LITTLE_ENDIAN
		return *(const sph_u32 *)src;
#elif SPH_BIG_ENDIAN
		return SWAP4(*(const sph_u32 *)src);
#else
		return (sph_u32)(((const unsigned char *)src)[0])
			| ((sph_u32)(((const unsigned char *)src)[1]) << 8)
			| ((sph_u32)(((const unsigned char *)src)[2]) << 16)
			| ((sph_u32)(((const unsigned char *)src)[3]) << 24);
#endif
	}


#define LVAR17(b)  sph_u32 \
  b ## 0, b ## 1, b ## 2, b ## 3, b ## 4, b ## 5, \
  b ## 6, b ## 7, b ## 8, b ## 9, b ## 10, b ## 11, \
  b ## 12, b ## 13, b ## 14, b ## 15, b ## 16;

#define LVARS   \
  LVAR17(a) \
  LVAR17(g) \
  LVAR17(p) \
  LVAR17(t)

#define M17(macro)   do { \
    macro( 0,  1,  2,  4); \
    macro( 1,  2,  3,  5); \
    macro( 2,  3,  4,  6); \
    macro( 3,  4,  5,  7); \
    macro( 4,  5,  6,  8); \
    macro( 5,  6,  7,  9); \
    macro( 6,  7,  8, 10); \
    macro( 7,  8,  9, 11); \
    macro( 8,  9, 10, 12); \
    macro( 9, 10, 11, 13); \
    macro(10, 11, 12, 14); \
    macro(11, 12, 13, 15); \
    macro(12, 13, 14, 16); \
    macro(13, 14, 15,  0); \
    macro(14, 15, 16,  1); \
    macro(15, 16,  0,  2); \
    macro(16,  0,  1,  3); \
  } while (0)

#define BUPDATE1(n0, n2)   do { \
    buffer[ptr24][n0] ^= buffer[ptr31][n2]; \
    buffer[ptr31][n2] ^= INW2(n2); \
  } while (0)

#define BUPDATE   do { \
    BUPDATE1(0, 2); \
    BUPDATE1(1, 3); \
    BUPDATE1(2, 4); \
    BUPDATE1(3, 5); \
    BUPDATE1(4, 6); \
    BUPDATE1(5, 7); \
    BUPDATE1(6, 0); \
    BUPDATE1(7, 1); \
  } while (0)

#define RSTATE(n0, n1, n2, n4)    (a ## n0 = state[n0])

#define WSTATE(n0, n1, n2, n4)    (state[n0] = a ## n0 )

#define GAMMA(n0, n1, n2, n4)   \
  (g ## n0 = a ## n0 ^ (a ## n1 | SPH_T32(~a ## n2)))

#define PI_ALL   do { \
    p0  = g0; \
    p1  = SPH_ROTL32( g7,  1); \
    p2  = SPH_ROTL32(g14,  3); \
    p3  = SPH_ROTL32( g4,  6); \
    p4  = SPH_ROTL32(g11, 10); \
    p5  = SPH_ROTL32( g1, 15); \
    p6  = SPH_ROTL32( g8, 21); \
    p7  = SPH_ROTL32(g15, 28); \
    p8  = SPH_ROTL32( g5,  4); \
    p9  = SPH_ROTL32(g12, 13); \
    p10 = SPH_ROTL32( g2, 23); \
    p11 = SPH_ROTL32( g9,  2); \
    p12 = SPH_ROTL32(g16, 14); \
    p13 = SPH_ROTL32( g6, 27); \
    p14 = SPH_ROTL32(g13,  9); \
    p15 = SPH_ROTL32( g3, 24); \
    p16 = SPH_ROTL32(g10,  8); \
  } while (0)

#define THETA(n0, n1, n2, n4)   \
  (t ## n0 = p ## n0 ^ p ## n1 ^ p ## n4)

#define SIGMA_ALL   do { \
    a0 = t0 ^ 1; \
    a1 = t1 ^ INW2(0); \
    a2 = t2 ^ INW2(1); \
    a3 = t3 ^ INW2(2); \
    a4 = t4 ^ INW2(3); \
    a5 = t5 ^ INW2(4); \
    a6 = t6 ^ INW2(5); \
    a7 = t7 ^ INW2(6); \
    a8 = t8 ^ INW2(7); \
    a9  =  t9 ^ buffer[ptr16][0]; \
    a10 = t10 ^ buffer[ptr16][1]; \
    a11 = t11 ^ buffer[ptr16][2]; \
    a12 = t12 ^ buffer[ptr16][3]; \
    a13 = t13 ^ buffer[ptr16][4]; \
    a14 = t14 ^ buffer[ptr16][5]; \
    a15 = t15 ^ buffer[ptr16][6]; \
    a16 = t16 ^ buffer[ptr16][7]; \
  } while (0)


#define PANAMA_STEP   do { \
    unsigned ptr16, ptr24, ptr31; \
    ptr24 = (ptr0 - 8) & 31; \
    ptr31 = (ptr0 - 1) & 31; \
    BUPDATE; \
    M17(GAMMA); \
    PI_ALL; \
    M17(THETA); \
    ptr16 = ptr0 ^ 16; \
    SIGMA_ALL; \
    ptr0 = ptr31; \
  } while (0)

	/*
	* These macros are used to compute
	*/
#define INC0     1
#define INC1     2
#define INC2     3
#define INC3     4
#define INC4     5
#define INC5     6
#define INC6     7
#define INC7     8


void panama(hash_t* hash)
{
	sph_u32 buffer[32][8];
	sph_u32 state[17];
	for (int i = 0; i < 32; i++)
		for (int j = 0; j < 8; j++)
			buffer[i][j] = 0x0;
	memset(buffer, 0, sizeof buffer);

	for (int i = 0; i < 16; i++)
	{
		state[i] = hash->h4[i];
	}
	state[16] = 0x20000000;


	LVARS
	unsigned ptr0 = 0;
	#define INW1(i)   state[(i)]
	#define INW2(i)   INW1(i)

	  M17(RSTATE);

	   PANAMA_STEP;

	#undef INW1
	#undef INW2

	   
	#define INW1(i)   state[(8+i)]
	#define INW2(i)   INW1(i)
	  
	    PANAMA_STEP;

	    M17(WSTATE);


	#undef INW1
	#undef INW2
		
#define INW1(i)   (sph_u32) (i == 0)
#define INW2(i)   INW1(i)
	
		M17(RSTATE);
	
		PANAMA_STEP;
		M17(WSTATE);

#undef INW1
#undef INW2
		
#define INW1(i)     INW_H1(INC ## i)
#define INW_H1(i)   INW_H2(i)
#define INW_H2(i)   a ## i
#define INW2(i)     buffer[ptr4][i]

		M17(RSTATE);
		for (int i = 0; i < 32; i++) {
			unsigned ptr4 = (ptr0 + 4) & 31;

			PANAMA_STEP;
		}

		M17(WSTATE);
#undef INW1
#undef INW_H1
#undef INW_H2
#undef INW2
		for (int u = 0; u <16; u++)
		{
			hash-> h4[u] = state[u];
		}

	//	barrier(CLK_GLOBAL_MEM_FENCE);
		// bool result = ((((sph_u64) state[16] << 32) | state[15]) <= target);
		// if (result)
		// output[output[0xFF]++] = SWAP4(gid);
	}


void panama_scanHash_pre(unsigned char* input, unsigned char* output, const unsigned int nonce)
{
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	hashdata.h4[14] = hashdata.h4[14] ^ hashdata.h4[15];
	hashdata.h4[15] = nonce;
	panama(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[i] = hashdata.h1[i];
	}
}
void panama_scanHash_post(unsigned char* input, unsigned char* output)
{
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	panama(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[i] = hashdata.h1[i];
	}
}
#ifdef __cplusplus
}
#endif
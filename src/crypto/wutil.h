#ifndef __WUTIL__
#define __WUTIL__
#define __ENDIAN_LITTLE__ 1
#if __ENDIAN_LITTLE__
#define SPH_LITTLE_ENDIAN 1
#else
#define SPH_BIG_ENDIAN 1
#endif

#define SPH_UPTR sph_u64

typedef unsigned int sph_u32;
typedef int sph_s32;

#ifndef __OPENCL_VERSION__
typedef unsigned long long sph_u64;
typedef long long sph_s64;
#else
typedef unsigned long sph_u64;
typedef long sph_s64;
#endif

#define SPH_64 1
#define SPH_64_TRUE 1

#define SPH_C32(x)    ((sph_u32)(x ## U))
#define SPH_T32(x)    ((x) & SPH_C32(0xFFFFFFFF))
#define SPH_ROTL32(x, n)   SPH_T32(((x) << (n)) | ((x) >> (32 - (n))))
#define SPH_ROTR32(x, n)   SPH_ROTL32(x, (32 - (n)))

#define SPH_C64(x)    ((sph_u64)(x ## UL))
#define SPH_T64(x)    ((x) & SPH_C64(0xFFFFFFFFFFFFFFFF))
#define SPH_ROTL64(x, n)   SPH_T64(((x) << (n)) | ((x) >> (64 - (n))))
#define SPH_ROTR64(x, n)   SPH_ROTL64(x, (64 - (n)))

#define SPH_ECHO_64 1
#define SPH_KECCAK_64 1
#define SPH_JH_64 1
#define SPH_SIMD_NOCOPY 0
#define SPH_KECCAK_NOCOPY 0
#define SPH_COMPACT_BLAKE_64 0
#define SPH_LUFFA_PARALLEL 0
#define SPH_SMALL_FOOTPRINT_GROESTL 0
#define SPH_GROESTL_BIG_ENDIAN 0
#define SPH_CUBEHASH_UNROLL 0
#define SPH_KECCAK_UNROLL   0
#define SPH_HAMSI_EXPAND_BIG 4


//#if SPH_BIG_ENDIAN
//#define DEC64E(x) SWAP8(x)
//#define DEC64BE(x) (*(const __global sph_u64 *) (x));
//#else
//#define DEC64E(x) SWAP8(x)
//#define DEC64BE(x) SWAP8(*(const __global sph_u64 *) (x));
//#endif


#define SWAP4(x,y) do {	\
typedef union \
	{	\
	unsigned int uint;\
	unsigned char uchar[4];\
	}swap4;\
	swap4 sw4;\
	sw4.uint = x;\
	sw4.uchar[0] ^= sw4.uchar[3]; sw4.uchar[3] ^= sw4.uchar[0]; sw4.uchar[0] ^= sw4.uchar[3];\
	sw4.uchar[1] ^= sw4.uchar[2]; sw4.uchar[2] ^= sw4.uchar[1]; sw4.uchar[1] ^= sw4.uchar[2];\
	y=sw4.uint;\
}while (0)
#define SWAP8(x,y) do {	\
typedef union \
	{	\
	unsigned long long ulong;\
	unsigned char uchar[8];\
	}swap8;\
	swap8 sw8;\
	sw8.ulong = x;\
	sw8.uchar[0] ^= sw8.uchar[7]; sw8.uchar[7] ^= sw8.uchar[0]; sw8.uchar[0] ^= sw8.uchar[7];\
	sw8.uchar[1] ^= sw8.uchar[6]; sw8.uchar[6] ^= sw8.uchar[1]; sw8.uchar[1] ^= sw8.uchar[6];\
	sw8.uchar[2] ^= sw8.uchar[5]; sw8.uchar[5] ^= sw8.uchar[2]; sw8.uchar[2] ^= sw8.uchar[5];\
	sw8.uchar[3] ^= sw8.uchar[4]; sw8.uchar[4] ^= sw8.uchar[3]; sw8.uchar[3] ^= sw8.uchar[4];\
	y=sw8.ulong;\
}while (0)
typedef union {
	unsigned char h1[64];
	unsigned int h4[16];
	unsigned long long h8[8];
} hash_t;
#endif
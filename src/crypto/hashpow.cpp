#include"hashpow.h"
hashPow* hashPow::_p = NULL;
hashPow::hashPow()
{
	sph_blake512_init(&ctx_blake);
	sph_bmw512_init(&ctx_bmw);
	sph_groestl512_init(&ctx_groestl);
	sph_skein512_init(&ctx_skein);
	sph_jh512_init(&ctx_jh);
	sph_keccak512_init(&ctx_keccak);
	sph_luffa512_init(&ctx_luffa1);
	sph_cubehash512_init(&ctx_cubehash1);
	sph_shavite512_init(&ctx_shavite1);
	sph_simd512_init(&ctx_simd1);
	sph_echo512_init(&ctx_echo1);
	sph_hamsi512_init(&ctx_hamsi1);
	sph_fugue512_init(&ctx_fugue1);
}
hashPow * hashPow::getinstance()
{
	if (hashPow::_p == NULL)
	{
		hashPow::_p = new hashPow();
	}
	return hashPow::_p;
}
void hashPow::compute(int id, unsigned char * input, unsigned char * hash)
{
	switch (id)
	{
	case 0:sph_blake512(&ctx_blake, input, 64);
		sph_blake512_close(&ctx_blake, hash);
		break;
	case 1:sph_bmw512(&ctx_bmw, input, 64);
		sph_bmw512_close(&ctx_bmw, hash);
		break;
	case 2:sph_groestl512(&ctx_groestl, input, 64);
		sph_groestl512_close(&ctx_groestl, hash);
		break;
	case 3:sph_skein512(&ctx_skein, input, 64);
		sph_skein512_close(&ctx_skein, hash);
		break;
	case 4:sph_jh512(&ctx_jh, input, 64);
		sph_jh512_close(&ctx_jh, hash);
		break;
	case 5:sph_keccak512(&ctx_keccak, input, 64);
		sph_keccak512_close(&ctx_keccak, hash);
		break;
	case 6:sph_luffa512(&ctx_luffa1, input, 64);
		sph_luffa512_close(&ctx_luffa1, hash);
		break;
	case 7:sph_cubehash512(&ctx_cubehash1, input, 64);
		sph_cubehash512_close(&ctx_cubehash1, hash);
		break;
	case 8:sph_shavite512(&ctx_shavite1, input, 64);
		sph_shavite512_close(&ctx_shavite1, hash);
		break;
	case 9:sph_simd512(&ctx_simd1, input, 64);
		sph_simd512_close(&ctx_simd1, hash);
		break;
	case 10:sph_echo512(&ctx_echo1, input, 64);
		sph_echo512_close(&ctx_echo1, hash);
		break;
	case 11:sph_hamsi512(&ctx_hamsi1, input, 64);
		sph_hamsi512_close(&ctx_hamsi1, hash);
		break;
	default:
		sph_fugue512(&ctx_fugue1, input, 64);
		sph_fugue512_close(&ctx_fugue1, hash);
		break;
	}
}

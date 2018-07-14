#ifndef _W_SHA2BIG_H_
#define _W_SHA2BIG_H_
#ifdef __cplusplus
extern "C" {
#endif
	void sha2big_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void sha2big_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
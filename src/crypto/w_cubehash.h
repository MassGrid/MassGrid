#ifndef _W_CUBEHASH_H_
#define _W_CUBEHASH_H_

#ifdef __cplusplus
extern "C" {
#endif
	void cubehash_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void cubehash_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
#ifndef _W_BLAKE256_H_
#define _W_BLAKE256_H_
#ifdef __cplusplus
extern "C" {
#endif
	void blake256_scanHash_pre(unsigned char* input, unsigned char* output, const unsigned int nonce);
	void blake256_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
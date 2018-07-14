#ifndef _W_SHEIN256_H_
#define _W_SKEIN256_H_
#ifdef __cplusplus
extern "C" {
#endif
	void skein256_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void skein256_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
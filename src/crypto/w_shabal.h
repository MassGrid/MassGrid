#ifndef _W_SHABAL_H_
#define _W_SHABAL_H_
#ifdef __cplusplus
extern "C" {
#endif
	void shabal_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void shabal_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
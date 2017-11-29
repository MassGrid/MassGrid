#ifndef _W_SIMD_H_
#define _W_SIMD_H_

#ifdef __cplusplus
extern "C" {
#endif
	void simd_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void simd_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
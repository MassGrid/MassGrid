#ifndef _W_KECCAK_H_
#define _W_KECCAK_H_

#ifdef __cplusplus
extern "C" {
#endif
	void keccak_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void keccak_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
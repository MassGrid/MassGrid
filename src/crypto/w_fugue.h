#ifndef _W_FUGUE_H_
#define _W_FUGUE_H_

#ifdef __cplusplus
extern "C" {
#endif
	void fugue_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void fugue_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
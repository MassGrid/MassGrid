#ifndef _W_SHAVITE_H_
#define _W_SHAVITE_H_

#ifdef __cplusplus
extern "C" {
#endif
	void shavite_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void shavite_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
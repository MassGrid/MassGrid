#ifndef _W_HAVAL_H_
#define _W_HAVAL_H_
#ifdef __cplusplus
extern "C" {
#endif
	void haval_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void haval_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
#ifndef _W_GROESTL_H_
#define _W_GROESTL_H_

#ifdef __cplusplus
extern "C" {
#endif
	void groestl_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void groestl_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
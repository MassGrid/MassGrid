#ifndef _W_JH_H_
#define _W_JH_H_

#ifdef __cplusplus
extern "C" {
#endif
	void jh_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void jh_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
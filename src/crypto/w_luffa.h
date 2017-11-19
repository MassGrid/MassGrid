#ifndef _W_LUFFA_H_
#define _W_LUFFA_H_

#ifdef __cplusplus
extern "C" {
#endif
	void luffa_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void luffa_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
#ifndef _W_BMW_H_
#define _W_BMW_H_

#ifdef __cplusplus
extern "C" {
#endif
	void bmw_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void bmw_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif
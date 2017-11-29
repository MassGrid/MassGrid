#ifndef _W_SHEIN_H_
#define _W_SKEIN_H_

#ifdef __cplusplus
extern "C" {
#endif
	void skein_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void skein_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
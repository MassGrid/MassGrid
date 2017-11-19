#ifndef _W_HAMSI_H_
#define _W_HAMSI_H_

#ifdef __cplusplus
extern "C" {
#endif
	void hamsi_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void hamsi_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
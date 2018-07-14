#ifndef _W_PANAMA_H_
#define _W_PANAMA_H_
#ifdef __cplusplus
extern "C" {
#endif
	void panama_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void panama_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif 
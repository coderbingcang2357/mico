#ifndef TEA_H
#define TEA_H

#include <stdint.h>

int tea_enc(char *in, uint32_t len, char *out, char *key_str, uint32_t key_len);
int tea_dec(char *in, uint32_t len, char *out, char *key_str, uint32_t key_len);

#endif
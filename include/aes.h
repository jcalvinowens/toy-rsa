#pragma once
#include <stdint.h>

struct aes_ctx;

struct aes_ctx *aes128_expand_key(void *in_key);
void aes128_encrypt(struct aes_ctx *ctx, void *buf);
void aes128_decrypt(struct aes_ctx *ctx, void *buf);
int aes_cipher_test(void);

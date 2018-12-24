#pragma once

#include <limits.h>

#define BFI_BITSHIFT 6
#define BFI_BITMASK ((1UL << BFI_BITSHIFT) - 1)

struct bfi;

struct bfi *bfi_alloc(int bitlen);
struct bfi *bfi_copy(struct bfi *b);
void bfi_extend(struct bfi *b, int new_bitlen);
void bfi_free(struct bfi *b);

void bfi_print(struct bfi *b);
int bfi_len(struct bfi *b);
int bfi_sign(struct bfi *b);
unsigned long *bfi_raw(struct bfi *b);

int bfi_cmp(struct bfi *a, struct bfi *b);

void bfi_inc(struct bfi *b);
void bfi_dec(struct bfi *b);
void bfi_shr(struct bfi *b);
void bfi_shl(struct bfi *b);
void bfi_multiple_shl(struct bfi *b, int n);

int bfi_is_zero(struct bfi *b);
int bfi_is_one(struct bfi *b);
int bfi_bit_set(struct bfi *b, int bit);
int bfi_is_divby_three(struct bfi *b);
int bfi_most_sig_bit(struct bfi *b);

void bfi_add(struct bfi *a, struct bfi *b);
void bfi_sub(struct bfi *a, struct bfi *b);
void bfi_modulo(struct bfi *b, struct bfi *div);
struct bfi *bfi_multiply(struct bfi *a, struct bfi *b);
struct bfi *bfi_divide(struct bfi *a, struct bfi *b, struct bfi **rem);

struct bfi *bfi_gcd(struct bfi *a, struct bfi *b);
struct bfi *bfi_mod_exp(struct bfi *base, struct bfi *exp, struct bfi *mod);

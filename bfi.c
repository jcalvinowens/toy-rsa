/*
 * bfi.c: Simple BIGNUM library
 *
 * Copyright (C) 2014 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "common.h"
#include "bfi.h"

/*
 * BFIs ("Big Fucking Integers") are stored as arrays of full-width integers of
 * the underlying word size of the machine: you can think of this as an integer
 * expressed in base (ULONG_MAX + 1). They are sign-magnitude.
 *
 * Internally, the BFI may have more memory allocated than it needs: we keep
 * track of the allocated length, but allow the "active" length of the BFI to
 * shrink and grow so we avoid needlessly processing zero words at the most
 * significant end of the array.
 *
 * The code is portable with the exception of multiplication: in order to run on
 * a new CPU type, you need to implement inline assembly for expanding
 * multiplication.
 */

struct bfi {
	char sign;
	int alloclen;
	int len;

	unsigned long n[];
};

static unsigned long bfi_safe(struct bfi *b, int index)
{
	fatal_on(index < 0, "Negative BFI index!\n");

	if (index < b->len)
		return b->n[index];

	return 0;
}

static struct bfi *__bfi_alloc(int len)
{
	struct bfi *ret;

	fatal_on(!len, "Attempt to allocate zero-length BFI\n");

	ret = calloc(1, sizeof(*ret) + (len + 1) * sizeof(unsigned long));
	fatal_on(!ret, "ENOMEM allocating BFI\n");

	ret->sign = 0;
	ret->alloclen = len + 1;
	ret->len = 1;

	return ret;
}

static inline int bitlen_to_words(int bitlen)
{
	return bitlen / LONG_BIT + !!(bitlen & (LONG_BIT - 1));
}

struct bfi *bfi_alloc(int bitlen)
{
	return __bfi_alloc(bitlen_to_words(bitlen));
}

void bfi_free(struct bfi *b)
{
	free(b);
}

static void shrink_bfi(struct bfi *b)
{
	while (b->len > 1 && !b->n[b->len - 1])
		b->len--;
}

static void __bfi_dup(struct bfi *dst, struct bfi *src)
{
	unsigned long tmp;

	shrink_bfi(src);
	fatal_on(dst->alloclen < src->len, "__bfi_dup() can't allocate\n");

	tmp = dst->alloclen;
	memcpy(dst, src, sizeof(*src) + src->len * sizeof(unsigned long));
	dst->alloclen = tmp;

	memset(&dst->n[dst->len], 0,
	       (dst->alloclen - dst->len) * sizeof(unsigned long));
}

struct bfi *bfi_copy(struct bfi *b)
{
	struct bfi *ret;

	ret = __bfi_alloc(b->len);
	__bfi_dup(ret, b);
	return ret;
}

static void __bfi_extend(struct bfi *b, int newlen)
{
	int oldlen = b->len;

	if (newlen <= oldlen)
		return;

	if (newlen <= b->alloclen) {
		b->len = newlen;
		return;
	}

	fatal("%p overflow: %d > %d, was %d\n", b, newlen, b->alloclen, b->len);
}

void bfi_extend(struct bfi *b, int new_bitlen)
{
	__bfi_extend(b, bitlen_to_words(new_bitlen));
}

void bfi_print(struct bfi *b)
{
	int i, j;

	printf("[%d] ", b->sign);
	for (j = 1, i = b->len - 1; i >= 0; j++, i--) {
		#if LONG_BIT == 64
		printf("%016lx", b->n[i]);
		if (j % 4 == 0)
			printf("\n%s", "       ");
		#elif LONG_BIT == 32
		printf("%08lx", b->n[i]);
		if (j % 8 == 0)
			printf("\n%s", "       ");
		#else
		#error "WAT"
		#endif
	}
	puts("\n");
}

int bfi_len(struct bfi *b)
{
	return b->len * LONG_BIT;
}

int bfi_sign(struct bfi *b)
{
	return b->sign;
}

unsigned long *bfi_raw(struct bfi *b)
{
	return b->n;
}

static int ulong_cmp(unsigned long a, unsigned long b)
{
	if (a > b)
		return 1;
	else if (b > a)
		return -1;
	else
		return 0;
}

int bfi_cmp(struct bfi *a, struct bfi *b)
{
	int i;

	for (i = max(a->len, b->len) - 1; i >= 0; i--)
		if (bfi_safe(a, i) != bfi_safe(b, i))
			return ulong_cmp(bfi_safe(a, i), bfi_safe(b, i));

	return 0;
}

void bfi_inc(struct bfi *b)
{
	int i = 0;

	while (!(++b->n[i++]));
}

void bfi_dec(struct bfi *b)
{
	int i = 0;

	while (!(b->n[i++]--));
}

int bfi_is_zero(struct bfi *b)
{
	int i;

	for (i = 0; i < b->len; i++)
		if (b->n[i])
			return 0;

	return 1;
}

int bfi_is_one(struct bfi *b)
{
	int i;

	if (b->n[0] != 1)
		return 0;

	for (i = 1; i < b->len; i++)
		if (b->n[i])
			return 0;

	return 1;
}

int bfi_bit_set(struct bfi *b, int bit)
{
	return !!(b->n[bit / LONG_BIT] & (1UL << (bit % LONG_BIT)));
}

int bfi_is_divby_three(struct bfi *b)
{
	unsigned long sum = 0;
	int i;

	for (i = 0; i < b->len; i++) {
		sum += __builtin_popcountl(b->n[i] & 0x5555555555555555UL);
		sum -= __builtin_popcountl(b->n[i] & 0xaaaaaaaaaaaaaaaaUL);
	}

	return sum % 3 == 1;
}

int bfi_most_sig_bit(struct bfi *b)
{
	int i;

	for (i = b->len - 1; i > 0 && !b->n[i]; i--);

	return LONG_BIT - __builtin_clzl(b->n[i]) - 1 + (i * LONG_BIT);
}

/*
 * Addition and subtraction are in place: so anytime we generate a carry, that
 * carry could potentially generate another carry, and so on.
 */

static void add_chained_carry(unsigned long *arr, unsigned long v)
{
	unsigned long tmp;

	if (!v)
		return;

	tmp = *arr;
	*arr += v;

	while (*arr++ < tmp) {
		tmp = *arr;
		(*arr)++;
	}
}

static void subtract_chained_borrow(unsigned long *arr, unsigned long v)
{
	unsigned long tmp;

	if (!v)
		return;

	tmp = *arr;
	*arr -= v;

	while (*arr++ > tmp) {
		tmp = *arr;
		(*arr)--;
	}
}

static void __bfi_add(struct bfi *a, struct bfi *b)
{
	int i;

	__bfi_extend(a, max(a->len, b->len) + 1);
	for (i = 0; i < b->len; i++)
		add_chained_carry(&a->n[i], b->n[i]);
}

static void __bfi_sub(struct bfi *a, struct bfi *b)
{
	int i;

	shrink_bfi(b);
	fatal_on(b->len > a->len, "Bad subtraction: %d > %d\n", b->len, a->len);

	for (i = 0; i < b->len; i++)
		subtract_chained_borrow(&a->n[i], b->n[i]);

	if (bfi_is_zero(a))
		a->sign = 0;
}

static int __bfi_shl_cmp(struct bfi *a, struct bfi *b, int b_shift)
{
	unsigned long adj, mask;
	int i, n, w;

	w = b_shift / LONG_BIT;
	n = b_shift % LONG_BIT;
	adj = (LONG_BIT - n) % LONG_BIT;
	mask = ~((1UL << adj) - 1);

	for (i = max(a->len, b->len + w + !!n) - 1; i >= 0; i--) {
		unsigned long a_val = bfi_safe(a, i);
		unsigned long b_val;

		b_val = bfi_safe(b, i - w) << n;
		if (i - w != 0 && adj)
			b_val |= (bfi_safe(b, i - w - 1) & mask) >> adj;

		if (a_val != b_val)
			return ulong_cmp(a_val, b_val);
	}

	return 0;
}

static void __bfi_shl_sub(struct bfi *a, struct bfi *b, int b_shift)
{
	unsigned long mask, nxt, prv, adj, tmp;
	int i, n, w;

	w = b_shift / LONG_BIT;
	n = b_shift % LONG_BIT;
	adj = (LONG_BIT - n) % LONG_BIT;
	mask = ~((1UL << adj) - 1);

	shrink_bfi(b);
	fatal_on(b->len + w > a->len, "Bad shftsub: %d > %d << %d\n",
		 b->len, a->len, b_shift);

	prv = 0;
	for (i = 0; i < b->len; i++) {
		nxt = b->n[i] & mask;
		tmp = b->n[i] << n;
		tmp |= prv;

		subtract_chained_borrow(&a->n[i + w], tmp);

		if (adj)
			prv = nxt >> adj;
	}

	if (prv)
		subtract_chained_borrow(&a->n[i + w], prv);

	if (bfi_is_zero(a))
		a->sign = 0;
}

static void __bfi_inv_sub(struct bfi *a, struct bfi *b)
{
	struct bfi *copy;

	copy = bfi_copy(b);
	__bfi_sub(copy, a);
	__bfi_dup(a, copy);
	free(copy);

	a->sign ^= 1;
}

void bfi_add(struct bfi *a, struct bfi *b)
{
	if (a->sign ^ b->sign) {
		if (bfi_cmp(a, b) < 0) {
			__bfi_inv_sub(a, b);
			return;
		}

		__bfi_sub(a, b);
		return;
	}

	__bfi_add(a, b);
}

void bfi_sub(struct bfi *a, struct bfi *b)
{
	if (a->sign ^ b->sign) {
		__bfi_add(a, b);
		return;
	}

	if (bfi_cmp(a, b) < 0) {
		__bfi_inv_sub(a, b);
		return;
	}

	__bfi_sub(a, b);
}

static void bfi_add_pow2(struct bfi *b, int pow2)
{
	int word, bit;

	word = pow2 / LONG_BIT;
	bit = pow2 % LONG_BIT;

	add_chained_carry(&b->n[word], 1UL << bit);
}

struct dword {
	unsigned long hi;
	unsigned long lo;
};

/*
 * Produces a double-width product from single-width inputs.
 */
static inline struct dword multiply(unsigned long a, unsigned long b)
{
	struct dword r;

	#if defined(__x86_64__)
	asm ("mulq %0" : "=d" (r.hi), "=a" (r.lo) : "a" (a), "0" (b) : "cc");
	#elif defined(__i386__)
	asm ("mull %0" : "=a" (r.hi), "=d" (r.lo) : "a" (a), "0" (b) : "cc");
	#elif defined(__arm__)
	asm ("umull %0,%1,%2,%3" : "=r" (r.lo), "=r" (r.hi) : "r" (a), "r" (b));
	#else
	#error "Sorry, no expanding multiply assembly for this CPU"
	#endif

	return r;
}

/*
 * This is exactly what you learned in school, except that we repeatedly add
 * into a single result array rather than generating multiple partial results
 * and summing them at the end.
 */
struct bfi *bfi_multiply(struct bfi *a, struct bfi *b)
{
	struct dword tmp;
	struct bfi *res;
	int i, j;

	shrink_bfi(a);
	shrink_bfi(b);

	res = __bfi_alloc(a->len + b->len);
	__bfi_extend(res, a->len + b->len);

	for (i = 0; i < a->len; i++) {
		for (j = 0; j < b->len; j++) {
			tmp = multiply(a->n[i], b->n[j]);
			add_chained_carry(&res->n[i + j + 1], tmp.hi);
			add_chained_carry(&res->n[i + j], tmp.lo);
		}
	}

	res->sign = a->sign ^ b->sign;

	shrink_bfi(res);
	return res;
}

void bfi_modulo(struct bfi *b, struct bfi *div)
{
	int bits;

	bits = bfi_most_sig_bit(b) - bfi_most_sig_bit(div);
	if (bits < 0)
		return;

	while (1) {
		if (__bfi_shl_cmp(b, div, bits) >= 0) {
			__bfi_shl_sub(b, div, bits);
			continue;
		}

		if (!bits)
			break;

		bits--;
	}

	b->sign = b->sign ^ div->sign;

	shrink_bfi(b);
	shrink_bfi(div);
}

struct bfi *bfi_divide(struct bfi *a, struct bfi *b, struct bfi **rem)
{
	struct bfi *quotient, *divisor, *dividend;
	int bits;

	bits = bfi_most_sig_bit(a) - bfi_most_sig_bit(b);
	fatal_on(bits < 0, "Divisor cannot be larger than dividend\n");

	quotient = __bfi_alloc(a->len);
	__bfi_extend(quotient, a->len);

	dividend = bfi_copy(a);
	divisor = bfi_copy(b);

	while (1) {
		if (__bfi_shl_cmp(dividend, divisor, bits) >= 0) {
			__bfi_shl_sub(dividend, divisor, bits);
			bfi_add_pow2(quotient, bits);
			continue;
		}

		if (!bits)
			break;

		bits--;
	}

	if (rem)
		*rem = dividend;
	else
		bfi_free(dividend);

	quotient->sign = a->sign ^ b->sign;

	bfi_free(divisor);
	return quotient;
}

struct bfi *bfi_gcd(struct bfi *a, struct bfi *b)
{
	struct bfi *ra = bfi_copy(a);
	struct bfi *rb = bfi_copy(b);
	struct bfi *t = __bfi_alloc(a->len);

	while (!bfi_is_zero(rb)) {
		xchg(t, rb);
		bfi_modulo(ra, t);
		xchg(ra, rb);
		xchg(ra, t);
	}

	bfi_free(rb);
	bfi_free(t);
	return ra;
}

struct bfi *bfi_mod_exp(struct bfi *base, struct bfi *exp, struct bfi *mod)
{
	struct bfi *res, *tmp;
	int bit;

	res = __bfi_alloc(mod->len);
	res->n[0] = 1;

	bit = bfi_most_sig_bit(exp);
	while (bit >= 0) {
		tmp = bfi_multiply(res, res);
		xchg(res, tmp);
		bfi_free(tmp);

		bfi_modulo(res, mod);

		if (bfi_bit_set(exp, bit--)) {
			tmp = bfi_multiply(res, base);
			xchg(res, tmp);
			bfi_free(tmp);

			bfi_modulo(res, mod);
		}
	}

	return res;
}

/*
 * Returns the modular multiplicative inverse of e mod tot
 *
 * https://en.wikipedia.org/wiki/Modular_multiplicative_inverse
 * https://en.wikipedia.org/wiki/Extended_Euclidean_algorithm
 */
struct bfi *mod_inv(struct bfi *e, struct bfi *tot)
{
	struct bfi *a = bfi_copy(e);
	struct bfi *b = bfi_copy(tot);
	struct bfi *m = bfi_alloc(bfi_len(tot));
	struct bfi *x_last = bfi_alloc(bfi_len(tot));
	struct bfi *x = bfi_alloc(bfi_len(tot));
	struct bfi *q = NULL;
	struct bfi *r = NULL;
	struct bfi *tmp = NULL;

	bfi_extend(m, bfi_len(tot));
	bfi_extend(x_last, bfi_len(tot));
	bfi_extend(x, bfi_len(tot));

	bfi_raw(x)[0] = 1;
	while (!bfi_is_zero(a)) {
		q = bfi_divide(b, a, &r);

		xchg(m, x_last);
		tmp = bfi_multiply(q, x);
		bfi_sub(m, tmp);
		bfi_free(tmp);

		xchg(x_last, x);
		xchg(x, m);
		xchg(b, a);
		xchg(a, r);

		bfi_free(r);
		bfi_free(q);
	}

	if (bfi_sign(x_last))
		bfi_add(x_last, tot);

	bfi_modulo(x_last, tot);

	bfi_free(a);
	bfi_free(b);
	bfi_free(m);
	bfi_free(x);
	return x_last;
}

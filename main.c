/*
 * toy-rsa: Toy RSA Implementation
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


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/random.h>

#include "common.h"
#include "bfi.h"

static void rng_fill_mem(void *mem, int len)
{
	char *cmem = mem;

	do {
		int r = getrandom(cmem, len, 0);

		if (r == -1)
			fatal("Bad getrandom: %m\n");

		cmem += r;
		len -= r;

	} while (len > 0);
}

struct rsa_key {
	struct bfi *exp;
	struct bfi *mod;
};

static void free_key(struct rsa_key *r)
{
	bfi_free(r->exp);
	bfi_free(r->mod);
	free(r);
}

/*
 * Use the Fermat primality test to determine with a very high probability
 * whether @b is likely to be a prime number.
 *
 * https://en.wikipedia.org/wiki/Fermat_primality_test
 */
static int rsa_bfi_is_prime(struct bfi *n)
{
	struct bfi *rnd = bfi_alloc(bfi_len(n));
	struct bfi *nminusone = bfi_copy(n);
	struct bfi *res;
	int i = 10, ret = -1;

	bfi_dec(nminusone);
	do {
		bfi_extend(rnd, bfi_len(n));
		rng_fill_mem(bfi_raw(rnd), bfi_len(rnd) / CHAR_BIT);
		res = bfi_mod_exp(rnd, nminusone, n);

		ret = !!bfi_is_one(res);
		bfi_free(res);

		if (ret == 0) {
			putchar(i == 10 ? '.' : '!');
			break;
		}

		putchar('+');

	} while(i--);

	bfi_free(rnd);
	bfi_free(nminusone);
	return ret;
}

/*
 * Find a random prime number of the specified length (or less)
 */
static struct bfi *rsa_find_prime(unsigned int bits)
{
	struct bfi *prime = bfi_alloc(bits);

	printf("Searching for %u bit prime: ", bits);
	while (1) {
		rng_fill_mem(bfi_raw(prime), bits / CHAR_BIT);
		bfi_extend(prime, bits);

		/*
		 * Don't waste time on even numbers.
		 */
		bfi_raw(prime)[0] |= 0x01UL;

		/*
		 * Checking for divisibility by 3 is cheap, don't waste time
		 * those numbers either.
		 */
		if (!bfi_is_divby_three(prime) && rsa_bfi_is_prime(prime))
			break;
	}

	puts(" done!");
	return prime;
}

static void rsa_generate_keypair(struct rsa_key **pub, struct rsa_key **priv, int bits)
{
	struct bfi *p, *q, *d, *mod, *tot;
	struct bfi *e = bfi_alloc(17);

	p = rsa_find_prime(bits >> 1);
	q = rsa_find_prime(bits >> 1);

	printf("\nGENERATED %d BIT RSA KEY:\n\n", bits);
	printf("p: ");
	bfi_print(p);
	printf("q: ");
	bfi_print(q);

	mod = bfi_multiply(p, q);

	printf("m: ");
	bfi_print(mod);

	bfi_dec(p);
	bfi_dec(q);
	tot = bfi_multiply(p, q);

	printf("t: ");
	bfi_print(tot);

	bfi_raw(e)[0] = 65537;

	printf("e: ");
	bfi_print(e);

	d = mod_inv(e, tot);

	printf("d: ");
	bfi_print(d);

	*pub = calloc(1, sizeof(**pub));
	(*pub)->exp = e;
	(*pub)->mod = mod;
	*priv = calloc(1, sizeof(**priv));
	(*priv)->exp = d;
	(*priv)->mod = bfi_copy(mod);

	bfi_free(p);
	bfi_free(q);
	bfi_free(tot);
}

static int rsa_cipher_test(int bits)
{
	struct rsa_key *pub, *priv;
	struct bfi *secret, *ciphertext, *decrypted;
	int ret = -1;

	rsa_generate_keypair(&pub, &priv, bits);

	printf("\nTESTING %d BIT RSA KEY:\n\n", bits);

	secret = bfi_alloc(128);
	#if LONG_BIT == 64
	bfi_raw(secret)[0] = 0xbeefbeefbeefbeefUL;
	bfi_raw(secret)[1] = 0xbeefbeefbeefbeefUL;
	#elif LONG_BIT == 32
	bfi_raw(secret)[0] = 0xbeefbeefUL;
	bfi_raw(secret)[1] = 0xbeefbeefUL;
	bfi_raw(secret)[2] = 0xbeefbeefUL;
	bfi_raw(secret)[3] = 0xbeefbeefUL;
	#else
	#error "LONG_BIT is not 32 or 64"
	#endif

	printf("S: ");
	bfi_print(secret);

	ciphertext = bfi_mod_exp(secret, pub->exp, pub->mod);

	printf("C: ");
	bfi_print(ciphertext);

	decrypted = bfi_mod_exp(ciphertext, priv->exp, priv->mod);

	printf("D: ");
	bfi_print(decrypted);

	if (!bfi_cmp(decrypted, secret))
		ret = 0;

	bfi_free(secret);
	bfi_free(ciphertext);
	bfi_free(decrypted);
	free_key(pub);
	free_key(priv);

	puts("");
	return ret;
}

static int count = 1;
static int bits = 512;

static void parse_args(int argc, char **argv)
{
	static const struct option opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "bits", required_argument, NULL, 'b' },
		{ "count", no_argument, NULL, 'c' },
		{},
	};

	while (1) {
		int i = getopt_long(argc, argv, "hb:c:", opts, NULL);

		switch (i) {
		case -1:
			return;
		case 'b':
			bits = atoi(optarg);
			printf("Will make %d bit keys\n", bits);
			break;
		case 'c':
			count = atoi(optarg);
			printf("Will run %d tests\n", count);
			break;
		case 'h':
			printf("Usage: %s [-b bits] [-c count]\n", argv[0]);
			exit(0);
		default:
			exit(1);
		}
	}
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);

	setvbuf(stdout, NULL, _IONBF, 0);
	while (count--) {
		if (rsa_cipher_test(bits)) {
			puts("FAILED!");
			return 1;
		}
	}

	return 0;
}

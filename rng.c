/*
 * bfi.c: Simple RNG, random AES128 encryption of a monotonic counter.
 *
 * Copyright (C) 2014 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "include/common.h"
#include "include/aes.h"
#include "include/rng.h"

#define AES_BLOCKSIZE 16
#define BLOCK_LONGS ((AES_BLOCKSIZE / sizeof(unsigned long)))

static struct aes_ctx *rng_key;
static unsigned long ctr[BLOCK_LONGS];

void init_rng(void)
{
	int fd, ret, tmp_len;
	char entropy[AES_BLOCKSIZE * 2], *tmp_ptr;

	fd = open(ENTROPY_SOURCE, O_RDONLY);
	if (fd == -1)
		fatal("Couldn't open entropy source\n");


	printf("Waiting for %d bytes of entropy: ", AES_BLOCKSIZE * 2);

	tmp_ptr = entropy;
	tmp_len = AES_BLOCKSIZE * 2;
	do {
		ret = read(fd, tmp_ptr, tmp_len);
		if (ret == -1)
			fatal("Error reading from entropy source: %m\n");

		printf("%ld/%d ", tmp_ptr + ret - entropy, AES_BLOCKSIZE * 2);
		tmp_ptr += ret;
		tmp_len -= ret;
	} while (tmp_len);

	puts("RNG ready!");
	close(fd);

	rng_key = aes128_expand_key(entropy);
	memcpy(ctr, &entropy[AES_BLOCKSIZE], AES_BLOCKSIZE);
}

void free_rng(void)
{
	free(rng_key);
}

static void bump_counter(void)
{
	unsigned i;

	for (i = 0; i < BLOCK_LONGS && !++ctr[i]; i++);
}

static void fill_one_block(void *mem)
{
	memcpy(mem, ctr, AES_BLOCKSIZE);
	aes128_encrypt(rng_key, mem);
	bump_counter();
}

void rng_fill_mem(void *mem, int len)
{
	char *cmem = mem;
	char buf[AES_BLOCKSIZE];
	int i, full_blocks;

	full_blocks = len / AES_BLOCKSIZE;
	for (i = 0; i < full_blocks; i++)
		fill_one_block(cmem + i * AES_BLOCKSIZE);

	if (i * AES_BLOCKSIZE == len)
		return;

	fill_one_block(buf);
	memcpy(cmem + i * AES_BLOCKSIZE, buf, len % AES_BLOCKSIZE);
}

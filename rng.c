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
#include <sys/random.h>

#include "common.h"
#include "rng.h"

void rng_fill_mem(void *mem, int len)
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

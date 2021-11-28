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

#include "common.h"
#include "rsa.h"
#include "rng.h"

static int count = 1;
static int bits = 1024;

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
			printf("Will make %d bit keys\n", count);
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

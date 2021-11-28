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

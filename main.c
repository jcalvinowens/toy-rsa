#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "include/common.h"
#include "include/aes.h"
#include "include/rsa.h"
#include "include/rng.h"

int main(int argc, char **argv)
{
	long testcount = LONG_MAX;
	int bits = 256;

	if (argc > 1)
		bits = atoi(argv[1]);

	if (argc > 2)
		testcount = atol(argv[2]);

	printf("Will run %ld %d bit RSA tests\n", testcount, bits);
	setvbuf(stdout, NULL, _IONBF, 0);

	init_rng();
	while (testcount--) {
		if (rsa_cipher_test(bits)) {
			puts("FAILED!");
			return 1;
		}
	}

	free_rng();
	return 0;
}

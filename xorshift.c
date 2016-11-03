#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "samlib.h"

/* https://en.wikipedia.org/wiki/Xorshift */

static uint64_t global_seed[2];

void xorshift_seed(uint64_t *seed)
{
	if (!seed)
		seed = global_seed;

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0) {
		int n = read(fd, seed, 128 / 8);
		close(fd);
		if (n == (128 / 8))
			return;
	}

	printf("WARNING: Unable to get seed\n");
	exit(1);
}

uint64_t xorshift128plus_r(uint64_t *seed)
{
	uint64_t x = seed[0];
	uint64_t const y = seed[1];
	seed[0] = y;
	x ^= x << 23; // a
	seed[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
	return seed[1] + y;
}

uint64_t xorshift128plus(void)
{
	return xorshift128plus_r(global_seed);
}

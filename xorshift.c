#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "samlib.h"

/* https://en.wikipedia.org/wiki/Xorshift */

static xorshift_seed_t global_seed;
static int seeded;

int xorshift_seed(xorshift_seed_t *seed)
{
	if (!seed)
		seed = &global_seed;

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0) {
		int n = read(fd, seed, sizeof(xorshift_seed_t));
		close(fd);
		if (n == sizeof(xorshift_seed_t)) {
			if (seed == &global_seed)
				seeded = 1;
			return 0;
		}
	}

	return -1;
}

void must_xorshift_seed(xorshift_seed_t *seed)
{
	if (xorshift_seed(seed)) {
		if (must_helper)
			must_helper("xorshift_seed", 0);
		else {
			printf("WARNING: Unable to get seed\n");
			exit(1);
		}
	}
}

uint64_t xorshift128plus_r(xorshift_seed_t *seed)
{
	uint64_t x = seed->seed[0];
	uint64_t const y = seed->seed[1];
	seed->seed[0] = y;
	x ^= x << 23; // a
	seed->seed[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
	return seed->seed[1] + y;
}

uint64_t xorshift128plus(void)
{
	if (!seeded)
		must_xorshift_seed(&global_seed);
	return xorshift128plus_r(&global_seed);
}

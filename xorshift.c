#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef WIN32
#include <Windows.h>
#include <wincrypt.h>
#endif

#include "samlib.h"

/* https://en.wikipedia.org/wiki/Xorshift */

static xorshift_seed_t global_seed;
static int seeded;

int xorshift_seed(xorshift_seed_t *seed)
{
#ifdef WIN32
	HCRYPTPROV hProv;
#endif
	int rc = -1;

	if (!seed)
		seed = &global_seed;

#ifdef WIN32
	if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, 0))
		if (CryptGenRandom(hProv, sizeof(*seed), (BYTE *)seed))
			rc = 0;
#else
	int n, fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0) {
		do
			n = read(fd, seed, sizeof(xorshift_seed_t));
		while (n != sizeof(xorshift_seed_t));
		close(fd);
		rc = 0;
	}
#endif

	if (rc == 0 && seed == &global_seed)
		seeded = 1;

	return rc;
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

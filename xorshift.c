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

void xorshift_seed(xorshift_seed_t *seed)
{
#ifdef WIN32
	HCRYPTPROV hProv;
#endif

	if (!seed) {
		seed = &global_seed;
		seeded = 1;
	}

#ifdef WIN32
	if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, 0))
		if (CryptGenRandom(hProv, sizeof(*seed), (BYTE *)seed))
			return;
#else
	int n, fd = open("/dev/urandom", O_RDONLY);
	if (fd >= 0) {
		do
			n = read(fd, seed, sizeof(xorshift_seed_t));
		while (n != sizeof(xorshift_seed_t));
		close(fd);
		return;
	}
#endif

	/* Default to rand. rand() on windows is only 15 bits... so make
	 * everybody suffer. Seriously, you shouldn't get here.
	 */
	struct timeval now;
	gettimeofday(&now, NULL);
	srand(now.tv_sec ^ (now.tv_usec << 8));
	uint8_t *p = (uint8_t *)seed;
	for (int i = 0; i < sizeof(*seed); ++i)
		*p++ = rand();
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
		xorshift_seed(&global_seed);
	return xorshift128plus_r(&global_seed);
}

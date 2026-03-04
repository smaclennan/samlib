#include "samlib.h"

static uint64_t seed[2];

#ifdef __x86_64__
static int have_rdseed(void)
{
	uint32_t regs[4];
	cpuid(0, regs);

	if (regs[0] >= 7) {
		cpuid(7, regs);
		return regs[1] & (1 << 18);
	}

	return 0;
}

/* For x86_64 we try to use rdseed. Most processors should have it.
 *
 * Returns 0 if no rdseed
 * Returns 1, 2 if we only got one rdseed
 * Returns 3 if we got both
 * I have never seen 1 or 2
 */
int init_seed(void)
{
	seed[0] = rdtsc();
	seed[1] = rdtsc();
	
	if (have_rdseed() == 0)
		return 0;

	uint64_t rand;
	unsigned char ok;
	int flags = 0;

	asm volatile ("rdseed %0; setc %1" : "=r" (rand), "=r" (ok) :: "cc");
	if (ok) {
		seed[0] = rand;
		flags |= 1;
	}
	asm volatile ("rdseed %0; setc %1" : "=r" (rand), "=r" (ok) :: "cc");
	if (ok) {
		seed[1] = rand;
		flags |= 2;
	}

	return flags;
}

void finish_seed(void)
{
	if (have_rdseed() == 0)
		seed[1] = rdtsc();
}

#else

/* If we don't have rdseed we use rdtsc() for the seeds.
 * If you can, call finish_seed() at a random time after init_seed().
 */
int init_seed(void)
{
	seed[0] = rdtsc();
	seed[1] = rdtsc();
	return 0;
}

void finish_seed(void)
{
	seed[1] = rdtsc();
}
#endif

#if 0 // xorshift128+
uint64_t rand128(void)
{
	uint64_t x = seed[0];
	uint64_t const y = seed[1];
	seed[0] = y;
	x ^= x << 23;
	seed[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
	return seed[1] + y;
}
#else // xoroshiro128+
static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

uint64_t rand128(void)
{
	const uint64_t s0 = seed[0];
	uint64_t s1 = seed[1];
	const uint64_t result = s0 + s1;

	s1 ^= s0;
	seed[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14); // a, b
	seed[1] = rotl(s1, 36); // c

	return result;
}
#endif

#ifdef STANDALONE
#include <stdio.h>

int main(int argc, char *argv[])
{
	int rc = init_seed();
	uint64_t rand = rand128();
	printf("init_seed %d seeds %lu %lu rand %lu\n", rc, seed[0], seed[1], rand);
	return 0;
}
#endif

/*
 * Local Variables:
 * compile-command: "gcc -DSTANDALONE -O2 -Wall random.c -o rand128 -lsamlib"
 * End:
 */

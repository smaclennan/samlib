#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "samlib.h"

#if defined(__x86_64__) || defined(__i386__)

/* Using a timing loop has been found to be much more accurate than
 * using the clock speed. However, it does take longer... 1 second
 * with the default values. Making CAL_TIME smaller looses
 * accuracy. Since you only need to call this once, the defaults are
 * probably a good trade off.
 *
 * As of 2018, the defaults make this more accurate than
 * gettimeofday() on my laptop.
 */
int tsc_divisor(uint64_t *divisor)
{
	int rc = EINVAL;

	char name[49];
	int family, model;
	cpu_info(name, &family, &model, NULL);

	if (strcmp(name, "AuthenticAMD") == 0) {
		uint32_t regs[4];

		/*
		 * c->x86_power is 8000_0007 edx. Bit 8 is TSC runs at constant rate
		 * with P/T states and does not stop in deep C-states
		 */
		cpuid(0x80000007, regs);
		if (regs[3] & (1 << 8))
			rc = 0;
	} else {
		/* Intel */
		/* This is how Linux decides to set constant_tsc flag in v4.15 */
		if ((family == 0xf && model >= 0x03) ||
			(family == 0x6 && model >= 0x0e))
			rc = 0;
	}

	if (divisor) {
#define N_LOOPS  4
#define CAL_TIME 250000
		uint64_t total = 0;

		for (int i = 0; i < N_LOOPS; ++i) {
			uint64_t start = rdtsc();
			// nanosleep() made no difference here
			usleep(CAL_TIME);
			uint64_t end = rdtsc();
			total += end - start;
		}

		*divisor = (total + (CAL_TIME / 2)) / (CAL_TIME * N_LOOPS);
	}

	return rc;
}
#elif defined(__aarch64__)
int tsc_divisor(uint64_t *divisor)
{   /* actually generic timer frequency - which is what we want */
	if (divisor) {
		asm volatile ("isb; mrs %0, cntfrq_el0" : "=r" (*divisor));
		*divisor /= 1000000;
	}
	return 0;
}
#elif defined(WIN32)
int tsc_divisor(uint64_t *divisor)
{
	LARGE_INTEGER val;

	if (QueryPerformanceFrequency(&val)) {
		if (divisor)
			*divisor = val.QuadPart / 1000;
		return 0;
	} else {
		if (divisor)
			*divisor = 1;
		return ENOENT;
	}
}
#else
int tsc_divisor(uint64_t *divisor) { if (divisor) *divisor = 1; return -1; }
#endif

/* Returns the delta in microseconds */
uint64_t delta_tsc(uint64_t start)
{
	static uint64_t divisor = 0;
	uint64_t end = rdtsc();

	if (divisor == 0)
		tsc_divisor(&divisor);
#ifdef WIN32
	return ((end - start) / divisor) * 1000;
#else
	return (end - start) / divisor;
#endif
}

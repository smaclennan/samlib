#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "samlib.h"

#if defined(__x86_64__) || defined(__i386__) || defined(WIN32)

int tsc_divisor(uint64_t *divisor)
{
#ifdef WIN32
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
#else
	char name[49], *p;
	int rc = EINVAL;

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
		*divisor = 1;

		unsigned int *v = (unsigned int *)name;
		cpuid(0x80000002, v);
		cpuid(0x80000003, v + 4);
		cpuid(0x80000004, v + 8);
		name[48] = 0;

		if (!(p = strchr(name, '@')))
			return rc;

		double d = strtod(p + 1, &p);
		while (isspace(*p)) ++p;
		if (strncasecmp(p, "ghz", 3) == 0) {
			d *= 1000000000.0;
		} else if (strncasecmp(p, "mhz", 3) == 0) {
			d *= 1000000.0;
		} else
			return rc;

		/* Probably could compensate for this above */
		*divisor = d / 1000000; /* us */
	}

	return rc;
#endif
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

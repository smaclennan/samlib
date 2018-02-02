#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "samlib.h"

#if defined(__x86_64__) || defined(__i386__) || defined(WIN32)

/* Warning: only tested on x86_64 */

int cpuid(uint32_t id, uint32_t *regs)
{
#ifdef WIN32
	__cpuid(regs, id);
#else
	asm volatile
		("cpuid"
		 : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		 : "a" (id), "c" (0));
#endif
	return 0;
}

int cpu_info(char *vendor, int *family, int *model, int *stepping)
{
	uint32_t regs[4];
	
	cpuid(0, regs);

	if (vendor) {
		memcpy(vendor, &regs[1], sizeof(uint32_t));
		memcpy(vendor + 4, &regs[3], sizeof(uint32_t));
		memcpy(vendor + 8, &regs[2], sizeof(uint32_t));
		vendor[12] = 0;
	}

	cpuid(1, regs);

	if (family)
		*family = ((regs[0] >> 8) & 0xf) | ((regs[0] >> (20 - 4)) & 0xf0);
	if (model)
		*model = ((regs[0] >> 4) & 0xf) | ((regs[0] >> (16 - 4)) & 0xf0);
	if (stepping)
		*stepping = regs[0] & 0xf;

	return 0;
}

int cpu_frequency(uint64_t *freq)
{
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

	if (freq) {
		*freq = 0;

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

		*freq = d;
	}

	return rc;
}

/* Returns the delta in microseconds */
uint64_t delta_tsc(uint64_t start)
{
	static uint64_t divisor = 0;
	uint64_t end = rdtsc();

	if (divisor == 0) {
		cpu_frequency(&divisor);
		divisor /= 1000000; /* us */
	}

	return (end - start) / divisor;
}
#else
int cpuid(uint32_t id, uint32_t *regs) { return -1; }
int cpu_info(int *family, int *model, int *stepping) { return -1; }
int cpu_frequency(uint64_t *freq) { return -1; }
uint64_t delta_tsc(uint64_t start) { return 0; }
#endif

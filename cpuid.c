#include <ctype.h>
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

/* str should be 16 * 3 + 1 = 49 bytes */
int processor_brand_string(char *str)
{
	uint32_t regs[4];

	cpuid(0x80000000, regs);
	if (regs[0] < 0x80000004)
		return ENOTSUP;

	cpuid(0x80000002, regs);
	memcpy(str, regs, sizeof(regs));

	cpuid(0x80000003, regs);
	memcpy(str + 16, regs, sizeof(regs));

	cpuid(0x80000004, regs);
	memcpy(str + 32, regs, sizeof(regs));

	str[48] = 0;

	int i;
	for (i = 0; isspace(str[i]); ++i) ;
	if (i)
		// Remove leading spaces
		memmove(str, str + i, 48 - i);

	return 0;
}
#elif defined(__aarch64__)
int cpuid(uint32_t id, uint32_t *regs)
{       /* Needs el1 privilege */
		asm volatile ("mrs %0, midr_el1" : "=r" (regs[0]));
		return 0;
}

int cpu_info(char *vendor, int *family, int *model, int *stepping)
{
	uint32_t regs[1];

	cpuid(0, regs);
	if (vendor) { /* implementer */
		*vendor = (regs[0] >> 24) & 0xff;
		*(vendor + 1) = 0;
	}

	if (family) /* partno */
		*family = (regs[0] >> 4) & 0xfff;
	if (model) /* variant + arch */
		*model = (regs[0] >> 16) & 0xff;
	if (stepping) /* revision */
		*stepping = regs[0] & 0xf;

	return 0;
}

int processor_brand_string(char *str) { return -1; }
#else
int cpuid(uint32_t id, uint32_t *regs) { return -1; }
int cpu_info(char *vendor, int *family, int *model, int *stepping) { return -1; }
int processor_brand_string(char *str) { return -1; }
#endif

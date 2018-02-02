#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

#ifdef TESTALL
int tsc_main(void)
#else
int main(void)
#endif
{
	char name[13];
	cpu_info(name, NULL, NULL, NULL);

	uint64_t freq;
	switch (cpu_frequency(&freq)) {
	case 0:
		printf("%s %.2fGHz\n", name, (double)freq / 1000000000.0);
		break;
	case EINVAL:
		printf("%s %.2fGHz but not constant\n", name, (double)freq  / 1000000000.0);
		break;
	case ENOTNAM:
		printf("Unable to parse model name\n");
		return 1;
	case -1:
		return 0; /* not a supported arch */
	}

	uint64_t start = rdtsc();
	usleep(88000); /* 88 ms */
	uint64_t delta = delta_tsc(start);
	/* Give it +/- 1 ms */
	if (delta < (88000 - 1000) || delta > (88000 + 1000)) {
		printf("TSC: Expected 88000 got %lu\n", delta);
		return 1;
	}

	return 0;
}

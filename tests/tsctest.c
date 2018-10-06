#include <stdio.h>
#include <stdlib.h>
#include "../samlib.h"

#define SLEEPY_TIME 888000ul

#ifdef TESTALL
int tsc_main(void)
#else
int main(void)
#endif
{
	int rc = 0;

	uint64_t divisor;
	switch (tsc_divisor(&divisor)) {
	case 0:
		printf("tsc divisor %lu\n", divisor);
		break;
	case EINVAL:
		printf("tsc divisor %lu but not constant\n", divisor);
		break;
	case -1:
		printf("tsc not supported\n");
		return 0; /* not a supported arch */
	default:
		printf("tsc divisor: unexpected return\n");
		return 1;
	}

	struct timeval tv_start, tv_end;
	uint64_t start, end;

	gettimeofday(&tv_start, NULL);
	start = rdtsc();
	usleep(SLEEPY_TIME);
	end = rdtsc();
	gettimeofday(&tv_end, NULL);

	uint64_t delta = (end - start) / divisor;
	/* Give it +/- 200us */
	if (delta < (SLEEPY_TIME - 200) || delta > (SLEEPY_TIME + 200)) {
		printf("TSC: Expected %lu got %lu\n", SLEEPY_TIME, delta);
		rc = 1;
	}

	uint64_t delta1 = delta_timeval(&tv_start, &tv_end);
	/* Give it +/- 200us */
	if (delta1 < (SLEEPY_TIME - 200) || delta1 > (SLEEPY_TIME + 200)) {
		printf("TV: Expected %lu got %lu\n", SLEEPY_TIME, delta1);
		rc = 1;
	}

	printf("Deltas: TSC %ld TV %ld\n", SLEEPY_TIME - delta, SLEEPY_TIME - delta1);

	return rc;
}

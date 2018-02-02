#include <stdio.h>
#include <time.h>
#include "../samlib.h"

#ifdef TESTALL
static int time_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	time_t now;
	struct timeval start, end;
	int rc = 0;
	unsigned long delta;

	start.tv_sec = 10;
	start.tv_usec = 100;
	end.tv_sec = 10;
	end.tv_usec = 101;

	if (delta_timeval(&start, &end) != 1) {
		puts("PROBS 1");
		rc = 1;
	}

	start.tv_sec = 10;
	start.tv_usec = 999999;
	end.tv_sec = 11;
	end.tv_usec = 0;

	if (delta_timeval(&start, &end) != 1) {
		puts("PROBS 2");
		rc = 1;
	}

	start.tv_sec = 10;
	start.tv_usec = 100;
	end.tv_sec = 11;
	end.tv_usec = 101;

	if (delta_timeval(&start, &end) != 1000001) {
		puts("PROBS 3");
		rc = 1;
	}

	start.tv_sec = 10;
	start.tv_usec = 999999;
	end.tv_sec = 12;
	end.tv_usec = 0;

	if (delta_timeval(&start, &end) != 1000001) {
		puts("PROBS 4");
		rc = 1;
	}

	start.tv_sec = 10;
	start.tv_usec = 101;
	end.tv_sec = 10;
	end.tv_usec = 100;

	if (delta_timeval(&start, &end) != ~0UL) {
		puts("PROBS 5");
		rc = 1;
	}

	start.tv_sec = 10;
	start.tv_usec = 999999;
	end.tv_sec = 10;
	end.tv_usec = 0;

	if (delta_timeval(&start, &end) != ~0UL) {
		puts("PROBS 6");
		rc = 1;
	}

	/* Test the windows gettimeofday */
	gettimeofday(&start, NULL);
	time(&now);
	/* NetBSD does not allow 1000000 */
	usleep(500000);
	usleep(500000);
	gettimeofday(&end, NULL);
	delta = delta_timeval_now(&start);
	/* Delta should be within +/-5ms of 1 second */
	if (delta < 995000 || delta > 1005000)
		printf("Warning: %s should be about 1,000,000\n", nice_number(delta));
	/* Now should be within a second of start */
	if (now != start.tv_sec && now + 1 != start.tv_sec && now - 1 != start.tv_sec)
		printf("Warning: time %ld should be about %ld\n", now, start.tv_sec);

	return rc;
}

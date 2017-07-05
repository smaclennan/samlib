#include <stdio.h>
#include "../samlib.h"

#ifdef TESTALL
static int time_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	struct timeval start, end;
	int rc = 0;

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

	return rc;
}

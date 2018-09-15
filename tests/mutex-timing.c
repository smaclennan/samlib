#include <stdio.h>
#include "../samthread.h"
#include "../samlib.h"

/* Not part of testall. This is meant for checking optimizations, not for general runs.
 * This only tests the non-contested case.
 */

/* Define this to make the times comparable between Linux slim mutex and others
#define FAIR_UNLOCK
 */

#define LOOPS 10000000

int main(int argc, char *argv[])
{
	long i;
	struct timeval start, end;

	mutex_t *mutex = mutex_create();
	if (!mutex) {
		printf("Unable to create mutex\n");
		return 1;
	}

	gettimeofday(&start, NULL);
	for (i = 0; i < LOOPS; ++i) {
		mutex_lock(mutex);
#if defined(__linux__) && !defined(WANT_PTHREADS) && !defined(FAIR_UNLOCK)
		mutex->state = 0;
#else
		mutex_unlock(mutex);
#endif
	}
	gettimeofday(&end, NULL);

	printf("mutex %ld\n", delta_timeval(&start, &end));

	mutex_destroy(&mutex);

	return 0;
}

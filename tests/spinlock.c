#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samthread.h"

#define N_CHILDREN  4

static DEFINE_SPINLOCK(lock);
static int count;

static int spin_fn(void *arg)
{
	spin_lock(&lock);
	++count;
	spin_unlock(&lock);
	return 0;
}

#ifdef TESTALL
int spinlock_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	long i;
	int rc = 0;
	samthread_t tid[N_CHILDREN];

	spin_lock(&lock);

	for (i = 0; i < N_CHILDREN; ++i) {
		tid[i] = samthread_create(spin_fn, (void *)i);
		if (tid[i] == (samthread_t)-1) {
			perror("create");
			return 1;
		}
	}

	usleep(10000); /* let the threads run */

	/* Make sure they locked */
	if (count) {
		printf("Count %d\n", count);
		rc = 1;
	}

	spin_unlock(&lock);

	for (i = 0; i < N_CHILDREN; ++i)
		rc = samthread_join(tid[i]);

	if (count != N_CHILDREN) {
		printf("Expected %d got %d\n", N_CHILDREN, count);
		rc = 1;
	}

	return rc;
}
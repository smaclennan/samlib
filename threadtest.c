#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "samthread.h"

#define CHILDREN  10

#ifdef WIN32
static mutex_t biglock;
#else
static DEFINE_MUTEX(biglock);
#endif

int fn(void *arg)
{
	long id = (long)arg;

	mutex_lock(&biglock);
	// usleep(500000);
	mutex_unlock(&biglock);
	return id;
}

int main()
{
	long i;
	int rc;
	samthread_t tid[CHILDREN];
#ifdef WIN32
	biglock = mutex_create();
#endif

	/* test a mutex_create */
	mutex_t *mutex = mutex_create();
	mutex_lock(mutex);
	mutex_unlock(mutex);
	mutex_destroy(mutex);

	mutex_lock(&biglock);

	for (i = 0; i < CHILDREN; ++i) {
		tid[i] = samthread_create(fn, (void *)i);
		if (tid[i] == (samthread_t)-1) {
			perror("create");
			exit(1);
		}
	}

	printf("Go!\n");
	mutex_unlock(&biglock);

	for (i = 0; i < CHILDREN; ++i) {
		rc = samthread_join(tid[i]);
		printf("joined %ld rc %d\n", i, rc);
	}

#ifdef WIN32
	getchar();
#endif

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -g -Wall threadtest.c samthread.c -o threadtest -lpthread"
 * End:
 */

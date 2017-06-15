#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "samthread.h"

#define CHILDREN  10

static DEFINE_MUTEX(biglock);
static int lock_count;

int fn(void *arg)
{
	long id = (long)arg;
	int r;

	mutex_lock(&biglock);
	r = __sync_add_and_fetch(&lock_count, 1);
	if (r != 1)
		printf("Child %ld lock prob %d\n", id, r);
	// usleep(500000);
	r = __sync_sub_and_fetch(&lock_count, 1);
	if (r)
		printf("Child %ld unlock prob %d\n", id, r);
	mutex_unlock(&biglock);
	return id;
}

int main()
{
	long i;
	int rc;
	samthread_t tid[CHILDREN];

	mutex_lock(&biglock);

	for (i = 0; i < CHILDREN; ++i) {
		tid[i] = samthread_create(fn, (void *)i);
		if (tid[i] == -1) {
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

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -g -Wall threadtest.c samthread.c -o threadtest"
 * End:
 */

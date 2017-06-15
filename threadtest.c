#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "samthread.h"


static DEFINE_MUTEX(biglock);

int fn(void *arg)
{
	long id = (long)arg;
	printf("child %ld\n", id);
	mutex_lock(&biglock);
	printf("child %ld sleeping...\n", id);
	usleep(500000);
	printf("child %ld unlock\n", id);
	mutex_unlock(&biglock);
	return 6 + id;
}

int main()
{
#if 0
	/* simple case - no contention */
	mutex_lock(&biglock);
	mutex_unlock(&biglock);
#endif

	signal(SIGCHLD, SIG_IGN);

	mutex_lock(&biglock);

	samthread_t tid = samthread_create(fn, (void *)1);
	if (tid == -1) {
		perror("create");
		exit(1);
	}
	printf("create returned %lx\n", tid);

	samthread_t tid2 = samthread_create(fn, (void *)2);
	if (tid2 == -1) {
		perror("create");
		exit(1);
	}
	printf("create returned %lx\n", tid2);

	samthread_t tid3 = samthread_create(fn, (void *)3);
	if (tid3 == -1) {
		perror("create");
		exit(1);
	}
	printf("create returned %lx\n", tid3);

	usleep(500000);

	printf("Parent unlocks tid %d\n", *(int *)tid);
	mutex_unlock(&biglock);

	printf("Parent waiting\n");
	int rc = samthread_join(tid);
	printf("joined 1 rc %d\n", rc);
	rc = samthread_join(tid2);
	printf("joined 2 rc %d\n", rc);
	rc = samthread_join(tid3);
	printf("joined 3 rc %d\n", rc);

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -g -Wall threadtest.c -o threadtest ./libsamthread.a"
 * End:
 */

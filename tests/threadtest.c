#include <stdio.h>
#include "../samthread.h"
#include <sys/resource.h>

#define CHILDREN  10

#ifdef WIN32
static mutex_t biglock;
#else
static DEFINE_MUTEX(biglock);
#endif

/* Shared with parent */
static int child_was, child_is;
static int done;
/* */

static int lower(void *arg)
{
	child_was = getpriority(PRIO_PROCESS, 0);

	if (setpriority(PRIO_PROCESS, 0, child_was + 10)) {
		printf("Unable to set priority\n");
		done = 1;
		return 1;
	}

	child_is = getpriority(PRIO_PROCESS, 0);

	done = 1;

	while (done == 1)
		usleep(1000);

	return 0;
}

/* This may be Linux specific */
static int test_priority(void)
{
	samthread_t tid;
	int parent_was, parent_is, parent_now;

	parent_was = getpriority(PRIO_PROCESS, 0);

	tid = samthread_create(lower, NULL);
	if (tid == (samthread_t)-1) {
		printf("samthread_create failed\n");
		return 1;
	}

	while (done == 0)
		usleep(1000);

	parent_is = getpriority(PRIO_PROCESS, 0);

	done = 2;

	samthread_join(tid);

	parent_now = getpriority(PRIO_PROCESS, 0);

#ifdef __linux__
	/* We expect 0 for everybody except child_is */
	if (parent_was || parent_is || parent_now || child_was || child_is != 10) {
		printf("Parent was %d is %d now %d Child was %d is %d\n",
			   parent_was, parent_is, parent_now, child_was, child_is);
		return 1; /* test failed */
	}
#else
	/* For BSD at least, setting the thread priority affects the
	 * parent. This is not what we want but we have to live with it.
	 */
	if (parent_was || parent_is != 10 || parent_now != 10 || child_was || child_is != 10) {
		printf("Parent was %d is %d now %d Child was %d is %d\n",
			   parent_was, parent_is, parent_now, child_was, child_is);
		return 1; /* test failed */
	}
#endif

	return 0;
}

static int fn(void *arg)
{
	long id = (long)arg;

	mutex_lock(&biglock);
	// usleep(500000);
	mutex_unlock(&biglock);
	return id;
}

#ifdef TESTALL
static int thread_main(void)
#else
	int main(int argc, char *argv[])
#endif
{
	long i, j;
	int rc = 0;
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
			return 1;
		}
	}

	mutex_unlock(&biglock);

	for (i = 0; i < CHILDREN; ++i) {
		j = samthread_join(tid[i]);
		if (j != i) {
			printf("joined %ld rc %ld\n", i, j);
			rc = 1;
		}
	}

	rc |= test_priority();

	return rc;
}

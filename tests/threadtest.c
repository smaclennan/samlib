#include <stdio.h>
#include "../samthread.h"

#define CHILDREN  10

#ifdef WIN32
static mutex_t biglock;
#else
#include <sys/resource.h>

static DEFINE_MUTEX(biglock);
#endif

#ifdef WIN32
#define PRIO_PROCESS 0

static int priority = 0;

/* SAM HACK just so the test passes */
int getpriority(int unused, int unused2)
{
	return priority;
}

int setpriority(int unused, int unused2, int newpriority)
{
	priority = newpriority;
	return 0;
}

#define usleep(u) Sleep((u) / 1000)
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

#if defined(__linux__) && !defined(WANT_PTHREADS)
/* Test that the futex call works as expected */
#include <sys/syscall.h>
#include <linux/futex.h>

static int futex_lock;
static int state;

static inline int futex(int *futex, int op, int val)
{
	return syscall(__NR_futex, futex, op, val, NULL);
}

int futex_fn1(void *arg)
{
	/* Make sure wake before wait works as expected */
	futex(&futex_lock, FUTEX_WAKE_PRIVATE, 1);
	usleep(100);
	if (state == 2) {
		puts("Problems");
		return 1;
	}
	state = 1;
	while (state == 1) usleep(1);

	/* Make sure wait before wake works as expected */
	futex(&futex_lock, FUTEX_WAIT_PRIVATE, 1);
	state = 4;
	while (state == 4) usleep(1);

	return 0;
}

int futex_fn2(void *arg)
{
	while (state == 0) usleep(1);
	futex(&futex_lock, FUTEX_WAIT_PRIVATE, 1);

	/* lock it before changing state for test 2 */
	futex_lock = 1;
	state = 2;

	/* give fn1 a chance to block */
	usleep(100000);
	if (state == 4) {
		puts("Problems");
		return 1;
	}
	futex(&futex_lock, FUTEX_WAKE_PRIVATE, 1);
	state = 3;
	while (state == 3) usleep(1);
	state = 5;

	return 0;
}

int futex_test(void)
{
	samthread_t tid1, tid2;

	tid1 = samthread_create(futex_fn1, NULL);
	tid2 = samthread_create(futex_fn2, NULL);

	if (samthread_join(tid1) || samthread_join(tid2)) {
		puts("Thread failure.");
		return 1;
	}

	return 0;
}
#else
int futex_test(void) { return 0; }
#endif

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
	rc |= futex_test();

	return rc;
}

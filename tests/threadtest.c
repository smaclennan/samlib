#include <stdio.h>
#include "../samthread.h"

#define CHILDREN  10

#ifdef WIN32
static mutex_t biglock;
#else
#include <sys/resource.h>

static DEFINE_MUTEX(biglock);

int default_priority = -1;

int get_priority(void)
{
#ifdef __linux__
	return getpriority(PRIO_PROCESS, 0);
#else
	struct sched_param param;
	param.sched_priority = -1;
	if (sched_getparam(0, &param))
		return -1;
	return param.sched_priority;
#endif
}

int set_priority(int newpriority)
{
#ifdef __linux__
	return setpriority(PRIO_PROCESS, 0, newpriority);
#else
	struct sched_param param;
	param.sched_priority = newpriority;
	return sched_setparam(0, &param);
#endif
}

/* Shared with parent */
static int child_was, child_is;
static int done;
/* */

static int lower(void *arg)
{
	child_was = get_priority();

	if (set_priority(child_was + 10)) {
		printf("Unable to set priority\n");
		done = 1;
		return 1;
	}

	child_is = get_priority();

	done = 1;

	while (done == 1)
		usleep(1000);

	return 0;
}

static int test_priority(void)
{
	samthread_t tid;
	int parent_was, parent_is, parent_now;

	parent_was = get_priority();
	default_priority = parent_was;

	tid = samthread_create(lower, NULL);
	if (tid == (samthread_t)-1) {
		printf("samthread_create failed\n");
		return 1;
	}

	while (done == 0)
		usleep(1000);

	parent_is = get_priority();

	done = 2;

	samthread_join(tid);

	parent_now = get_priority();

	/* We expect default for everybody except child_is */
	if (-1         == default_priority ||
		parent_was != default_priority ||
		parent_is  != default_priority ||
		parent_now != default_priority ||
		child_was  != default_priority ||
		child_is   != (default_priority + 10)) {
		printf("Parent was %d is %d now %d Child was %d is %d (default %d)\n",
			   parent_was, parent_is, parent_now, child_was, child_is, default_priority);
		return 1; /* test failed */
	}

	return 0;
}
#endif /* !WIN32 */

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
{   /* Test that the futex works as expected by the mutex code. */
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

#ifndef WIN32
	rc |= test_priority();
#endif
	rc |= futex_test();

	return rc;
}

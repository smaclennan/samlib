#define _GNU_SOURCE /* for clone */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <syscall.h>
#include <linux/futex.h>

#include "samthread.h"

/* The clone flags. */
#define CLONE_FLAGS (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | \
					 CLONE_THREAD | CLONE_CHILD_CLEARTID | CLONE_SYSVSEM)

static inline int futex(int *futex, int op, int val)
{
	return syscall(__NR_futex, futex, op, val, NULL);
}

struct athread {
	int tid;
	int stack_size;
	int rc;
	int (*fn)(void *arg);
	void *arg;
};

static int samthread_func(void *arg)
{
	struct athread *f = arg;

	f->rc = f->fn(f->arg);

	f->tid = 0;
	futex(&f->tid, FUTEX_WAKE_PRIVATE, 1);

	while (1)
		syscall(__NR_exit, 0);

	return 0; /* unreached */
}

samthread_t samthread_create(int (*fn)(void *arg), void *arg)
{
	struct rlimit rlim = { 0 };
	void *stack, *child_stack;
	int pid;
	struct athread *f;

	if (!fn) {
		errno = EINVAL;
		return -1;
	}

#ifdef CHILD_STACK_SIZE
	rlim.rlim_cur = CHILD_STACK_SIZE;
#else
	if (getrlimit(RLIMIT_STACK, &rlim))
		return -1;
#endif

	stack = mmap(NULL, rlim.rlim_cur, PROT_READ | PROT_WRITE,
				 MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack == MAP_FAILED)
		return -1;

	/* x86 64 grows stack down */
	child_stack = stack + rlim.rlim_cur;

	f = stack; /* steal some memory off the top */
	f->tid = 1;
	f->stack_size = rlim.rlim_cur;
	f->rc = -1;
	f->fn = fn;
	f->arg = arg;

	pid = clone(samthread_func, child_stack, CLONE_FLAGS, f, NULL, NULL, stack);
	if (pid == -1) {
		int saved = errno;
		munmap(stack, rlim.rlim_cur);
		errno = saved;
		return -1;
	}

	return (samthread_t)stack;
}

int samthread_join(samthread_t tid)
{
	struct athread *f = (struct athread *)tid;
	int rc;

	futex((int *)tid, FUTEX_WAIT, 1);

	rc = f->rc;

	munmap(f, f->stack_size);

	return rc;
}

/* Mutex has three states:
 * 0 = unlocked
 * 1 = locked
 * 2 = contended
 */

void mutex_lock(mutex_t *lock)
{
	while (1) {
		int r = __sync_val_compare_and_swap(lock, 0, 1);
		if (r == 0)
			return;

		if (r == 2 || __sync_val_compare_and_swap(lock, 1, 2))
			futex(lock, FUTEX_WAIT_PRIVATE, 2);
	}
}

void mutex_unlock(mutex_t *lock)
{
	int r = __sync_val_compare_and_swap(lock, 1, 0);
	if (r == 1)
		return;

	*lock = 0;

	/* We need to wake two threads to potentially get contention set. */
	futex(lock, FUTEX_WAKE_PRIVATE, 2);
}

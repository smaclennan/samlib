#include "samthread.h"

#if defined(__linux__) && !defined(WANT_PTHREADS)
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>

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
	struct athread *f;
	int rc;

	if (tid == (samthread_t)-1) {
		errno = EINVAL;
		return -1;
	}

	f = (struct athread *)tid;

	futex((int *)tid, FUTEX_WAIT, 1);

	rc = f->rc;

	munmap(f, f->stack_size);

	return rc;
}

mutex_t *mutex_create(void)
{
	return calloc(1, sizeof(struct mutex));
}

void mutex_destroy(mutex_t *mutex)
{
	if (mutex)
		free(mutex);
}

void mutex_lock(mutex_t *mutex)
{
	/* Optimize for the non-contended state */
	if (__sync_val_compare_and_swap(&mutex->state, 0, 1) == 0)
		return;

	__sync_add_and_fetch(&mutex->count, 1);
	while (1) {
		int r = __sync_val_compare_and_swap(&mutex->state, 0, 1);
		if (r == 0) {
			__sync_sub_and_fetch(&mutex->count, 1);
			return;
		}

		futex(&mutex->state, FUTEX_WAIT_PRIVATE, 1);
	}
}

void mutex_unlock(mutex_t *mutex)
{
	int r = __sync_val_compare_and_swap(&mutex->state, 1, 0);
	if (r == 1)
		if (mutex->count == 0)
			return;

	mutex->state = 0;

	futex(&mutex->state, FUTEX_WAKE_PRIVATE, 1);
}

#elif defined(WIN32)

struct thread_wrapper_arg {
	int(*fn)(void *arg);
	void *arg;
	int rc;
};

static DWORD __stdcall thread_wrapper(LPVOID lpThreadParameter)
{
	struct thread_wrapper_arg *wa = lpThreadParameter;
	int(*fn)(void *) = wa->fn;
	void *arg = wa->arg;

	free(wa);

	return fn(arg);
}

samthread_t samthread_create(int (*fn)(void *arg), void *arg)
{
	HANDLE thread;
	struct thread_wrapper_arg *wa = calloc(1, sizeof(struct thread_wrapper_arg));
	if (!wa)
		return (samthread_t)-1;

	wa->fn = fn;
	wa->arg = arg;

	thread = CreateThread(NULL,
#ifdef CHILD_STACK_SIZE
						CHILD_STACK_SIZE,
#else
						0, /* default */
#endif
						thread_wrapper, wa, 0, NULL);

	if (!thread)
		free(wa);

	return thread;
}

int samthread_join(samthread_t tid)
{
	DWORD rc = (DWORD)-1;

	WaitForSingleObject(tid, INFINITE);
	GetExitCodeThread(tid, &rc);

	return rc;
}

mutex_t *mutex_create(void)
{
	return (mutex_t *)CreateMutex(NULL, FALSE, NULL);
}

void mutex_destroy(mutex_t *mutex)
{
	CloseHandle((HANDLE)mutex);
}

void mutex_lock(mutex_t *mutex)
{
	WaitForSingleObject((HANDLE)mutex, INFINITE);
}

void mutex_unlock(mutex_t *mutex)
{
	ReleaseMutex((HANDLE)mutex);
}

#else
#include <stdlib.h>

/* Default to pthreads */

struct pthread_wrapper_arg {
	int (*fn)(void *arg);
	void *arg;
};

static void *pthread_wrapper(void *arg)
{
	struct pthread_wrapper_arg *wa = arg;
	int (*fn)(void *) = wa->fn;
	void *fn_arg = wa->arg;
	long rc;

	free(wa);

	rc = fn(fn_arg);

	return (void *)rc;
}

samthread_t samthread_create(int (*fn)(void *arg), void *arg)
{
	samthread_t tid;
	struct pthread_wrapper_arg *wa = calloc(1, sizeof(struct pthread_wrapper_arg));
	if (!wa)
		return (samthread_t)-1;

	wa->fn = fn;
	wa->arg = arg;

	if (pthread_create(&tid, NULL, pthread_wrapper, wa)) {
		free(wa);
		return (samthread_t)-1;
	}

	return tid;
}

int samthread_join(samthread_t tid)
{
	void *rc;

	if (pthread_join(tid, &rc))
		return -1;

	return (long)rc;
}

mutex_t *mutex_create(void)
{
	pthread_mutex_t *mutex = calloc(1, sizeof(pthread_mutex_t));

	pthread_mutex_init(mutex, NULL);
	return mutex;
}

void mutex_destroy(mutex_t *mutex)
{
	if (mutex) {
		pthread_mutex_destroy(mutex);
		free(mutex);
	}
}

void mutex_lock(mutex_t *mutex)
{
	pthread_mutex_lock(mutex);
}

void mutex_unlock(mutex_t *mutex)
{
	pthread_mutex_unlock(mutex);
}

#endif

#ifdef __linux__
#include <sys/syscall.h>

pid_t samthread_tid(void)
{
	return syscall(__NR_gettid);
}
#elif defined(__FreeBSD__)
#include <sys/thr.h>

pid_t samthread_tid(void)
{
	long tid;
	thr_self(&tid);
	return tid;
}
#elif defined(__OpenBSD__)
pid_t samthread_tid(void)
{
	return getthrid();
}
#elif defined(WIN32)
pid_t samthread_tid(void)
{
	return GetCurrentThreadId();
}
#elif defined(__QNXNTO__)
pid_t samthread_tid(void)
{
	return gettid();
}
#else
#warning No gettid

pid_t samthread_tid(void)
{
	return getpid();
}
#endif

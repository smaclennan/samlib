#ifndef __SAMTHREAD_H__
#define __SAMTHREAD_H__

/* If defined, this is the stack size for all threads.
 * If not defined, then RLIMIT_STACK is used in Linux and default in Windows.
 */
/* #define CHILD_STACK_SIZE */


/* Define this for pthreads rather than light weight threads under
 * Linux. Mainly for debugging.
#define WANT_PTHREADS
 */

#ifdef WIN32
#define sched_yield() Sleep(0)
#else
#define _GNU_SOURCE
#include <unistd.h>
#include <sched.h>
#endif

#if defined(__linux__) && !defined(WANT_PTHREADS)
typedef unsigned long samthread_t;

typedef struct mutex {
	int state;
	int count;
} mutex_t;

#define DEFINE_MUTEX(name) mutex_t name = { 0 }

#define unlikely(x)     __builtin_expect((x),0)

#elif defined(WIN32)

#include <Windows.h>

typedef DWORD pid_t;
typedef HANDLE samthread_t;

/* No DEFINE_MUTEX for windows.. you must use mutex_create() */
typedef HANDLE mutex_t;

#else

/* Default to pthreads */
#include <pthread.h>

typedef pthread_t samthread_t;
typedef pthread_mutex_t mutex_t;

#define DEFINE_MUTEX(name) mutex_t name = PTHREAD_MUTEX_INITIALIZER

#endif /* pthreads */

samthread_t samthread_create(int (*fn)(void *arg), void *arg);
int samthread_join(samthread_t tid);
/* This is not the samthread_t, it is a unique thread id */
pid_t samthread_tid(void);

mutex_t *mutex_create(void);
void mutex_destroy(mutex_t **mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#ifdef WIN32
typedef LONG spinlock_t;
#define inline _inline
#else
typedef unsigned spinlock_t;
#endif

#define DEFINE_SPINLOCK(name) spinlock_t name = 0

static inline void spinlock_init(spinlock_t *lock) { *lock = 0; }

static inline void spin_lock(spinlock_t *lock)
{
#ifdef WIN32
	while (InterlockedCompareExchange(lock, 1, 0))
		sched_yield();
#else
	while (!__sync_bool_compare_and_swap(lock, 0, 1))
		sched_yield();
#endif
}

static inline void spin_unlock(spinlock_t *lock) { *lock = 0; }

#endif

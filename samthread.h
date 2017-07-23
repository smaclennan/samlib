#ifndef __SAMTHREAD_H__
#define __SAMTHREAD_H__

/* If defined, this is the stack size for all threads.
 * If not defined, then RLIMIT_STACK is used in Linux and default in Windows.
 */
/* #define CHILD_STACK_SIZE */


/* Define this for pthreads rather than light weight threads under
 * Linux. Mainly for debugging.
 */
#define WANT_PTHREADS

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

#elif defined(WIN32)

#include <Windows.h>

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
pid_t gettid(void);

mutex_t *mutex_create(void);
void mutex_destroy(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

typedef unsigned spinlock_t;

#define DEFINE_SPINLOCK(name) spinlock_t name

static inline void spinlock_init(spinlock_t *lock) { *lock = 0; }

static inline void spin_lock(spinlock_t *lock)
{
	while (!__sync_bool_compare_and_swap(lock, 0, 1))
		sched_yield();
}

static inline void spin_unlock(spinlock_t *lock) { *lock = 0; }

#endif

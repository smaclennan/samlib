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

#ifndef WIN32
#define _GNU_SOURCE
#include <unistd.h>
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

#endif

samthread_t samthread_create(int (*fn)(void *arg), void *arg);
int samthread_join(samthread_t tid);

mutex_t *mutex_create(void);
void mutex_destroy(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif

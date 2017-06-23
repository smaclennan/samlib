#ifndef __SAMTHREAD_H__
#define __SAMTHREAD_H__

#include <signal.h>

/* If defined, this is the stack size for all threads.
 * If not defined, then RLIMIT_STACK is used.
 */
/* #define CHILD_STACK_SIZE */

typedef unsigned long samthread_t;

samthread_t samthread_create(int (*fn)(void *arg), void *arg);
int samthread_join(samthread_t tid);

typedef struct mutex {
	int state;
	int count;
} mutex_t;

#define DEFINE_MUTEX(name) mutex_t name = { 0 }

void mutex_lock(mutex_t *lock);
void mutex_unlock(mutex_t *lock);

#endif

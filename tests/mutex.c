#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include "../samthread.h"

static int verbose = 0;

static int test_state;
#define STATE_LOCKED 1
#define STATE_UNLOCK 2
#define STATE_DO_LOCK 3
#define STATE_DESTROY 4

static int ready;

static mutex_t *global_mutex;

static inline int futex(int *futex, int op, int val)
{
	return syscall(__NR_futex, futex, op, val, NULL);
}

static inline void SET_STATE(int state)
{
	if (verbose)
		printf("Set state %d\n", state);

	test_state = state;
}

static inline void WAIT_FOR_STATE(int state)
{
	while (test_state != state)
		usleep(1000);
}

void my_mutex_destroy(mutex_t **mutex)
{
	if (!*mutex)
		return;

	mutex_lock(*mutex);
	// SAM DBG - let the std threads try to lock
	SET_STATE(STATE_DO_LOCK);
	while ((*mutex)->count < 2)
		usleep(1000);
	SET_STATE(STATE_DESTROY);
	// SAM DBG

	mutex_t *save = *mutex;
	*mutex = NULL;

	while (save->count > 0) {
		save->state = 0;
		futex(&save->state, FUTEX_WAKE_PRIVATE, 1);
	}

	free(save);
}

int thread_lock(void *arg)
{
	mutex_lock(global_mutex);

	SET_STATE(STATE_LOCKED);
	WAIT_FOR_STATE(STATE_UNLOCK);

	mutex_unlock(global_mutex);
	return 0;
}

int thread_destroy(void *arg)
{
	++ready;
	my_mutex_destroy(&global_mutex);
	assert(global_mutex == NULL);
	return 0;
}

int thread_std(void *arg)
{
	++ready;
	WAIT_FOR_STATE(STATE_DO_LOCK);
	mutex_lock(global_mutex);
	mutex_unlock(global_mutex);
	assert(global_mutex == NULL);
	return 0;
}

int main(int argc, char *argv[])
{
	samthread_t t1, t2, t3, t4;

	global_mutex = mutex_create();
	assert(global_mutex);

	t1 = samthread_create(thread_lock, NULL);

	t2 = samthread_create(thread_destroy, NULL);

	t3 = samthread_create(thread_std, NULL);
	t4 = samthread_create(thread_std, NULL);

	WAIT_FOR_STATE(STATE_LOCKED);

	while (ready < 3)
		usleep(1000);

	SET_STATE(STATE_UNLOCK);

	samthread_join(t1);
	samthread_join(t2);
	samthread_join(t3);
	samthread_join(t4);

	assert(global_mutex == NULL);

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -O2 -Wall mutex.c -o mutex -lsamthread"
 * End:
 */

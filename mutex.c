#include "samthread.h"

#if defined(__linux__) && !defined(WANT_PTHREADS)
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#define unlikely(x)     __builtin_expect((x), 0)
#define likely(x)       __builtin_expect((x), 1)

static inline int futex(int *futex, int op, int val)
{
	return syscall(__NR_futex, futex, op, val, NULL);
}

mutex_t *mutex_create(void)
{
	return calloc(1, sizeof(mutex_t));
}

/* Returns with mutex set to NULL. */
void mutex_destroy(mutex_t **mutex)
{
	if (!mutex || !*mutex)
		return;

	mutex_t *save = *mutex;
	mutex_lock(save);
	*mutex = NULL;

	while (save->count > 0) {
		save->state = 0;
		futex(&save->state, FUTEX_WAKE_PRIVATE, 1);
	}

	free(save);
}

void mutex_lock(mutex_t *mutex)
{
	if (unlikely(!mutex))
		return;

	/* Optimize for the non-contended state */
	if (likely(__sync_val_compare_and_swap(&mutex->state, 0, 1) == 0))
		return;

	__sync_add_and_fetch(&mutex->count, 1);
	while (1) {
		int r = __sync_val_compare_and_swap(&mutex->state, 0, 1);
		if (r == 0) {
			__sync_sub_and_fetch(&mutex->count, 1);
			return;
		}

		futex(&mutex->state, FUTEX_WAIT_PRIVATE, 1);
		if (unlikely(!mutex))
			return;
	}
}

void mutex_unlock(mutex_t *mutex)
{
	if (unlikely(!mutex))
		return;

	int r = __sync_val_compare_and_swap(&mutex->state, 1, 0);
	if (unlikely(r != 1 || mutex->count != 0)) {
		mutex->state = 0;
		futex(&mutex->state, FUTEX_WAKE_PRIVATE, 1);
	}
}

#elif defined(WIN32)

mutex_t *mutex_create(void)
{
	return (mutex_t *)CreateMutex(NULL, FALSE, NULL);
}

void mutex_destroy(mutex_t **mutex)
{
	CloseHandle((HANDLE)*mutex);
	*mutex = INVALID_HANDLE_VALUE;
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

mutex_t *mutex_create(void)
{
	pthread_mutex_t *mutex = calloc(1, sizeof(pthread_mutex_t));

	pthread_mutex_init(mutex, NULL);
	return mutex;
}

void mutex_destroy(mutex_t **mutex)
{
	if (mutex && *mutex) {
		pthread_mutex_destroy(*mutex);
		free(*mutex);
		*mutex = NULL;
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

#include <stdio.h>
#include "../samlib.h"
#include "../samthread.h"

#ifdef HAVE_VALGRIND_H
#include <valgrind/valgrind.h>
#else
#define RUNNING_ON_VALGRIND 0
#endif

#define N_CHILDREN 8
#define COUNT (0x200000000UL / N_CHILDREN)

static unsigned long count = COUNT;

int verbose = 1;

/* Pascal's Triangle */
unsigned long expected[65] = {
	1,
	64,
	2016,
	41664,
	635376,
	7624512,
	74974368,
	621216192,
	4426165368,
	27540584512,
	151473214816,
	743595781824,
	3284214703056,
	13136858812224,
	47855699958816,
	159518999862720,
	488526937079580,
	1379370175283520,
	3601688791018080,
	8719878125622720,
	19619725782651120,
	41107996877935680,
	80347448443237920,
	146721427591999680,
	250649105469666120,
	401038568751465792,
	601557853127198688,
	846636978475316672,
	1118770292985239888,
	1388818294740297792,
	1620288010530347424,
	1777090076065542336,
	1832624140942590534,
	1777090076065542336,
	1620288010530347424,
	1388818294740297792,
	1118770292985239888,
	846636978475316672,
	601557853127198688,
	401038568751465792,
	250649105469666120,
	146721427591999680,
	80347448443237920,
	41107996877935680,
	19619725782651120,
	8719878125622720,
	3601688791018080,
	1379370175283520,
	488526937079580,
	159518999862720,
	47855699958816,
	13136858812224,
	3284214703056,
	743595781824,
	151473214816,
	27540584512,
	4426165368,
	621216192,
	74974368,
	7624512,
	635376,
	41664,
	2016,
	64,
	1
};


static unsigned long child_bins[N_CHILDREN][65];

static int generator(void *arg)
{
	unsigned long *bins = arg;
	xorshift_seed_t seed;
	unsigned long i;
	uint64_t r;
	int bin;

	xorshift_seed(&seed);

	for (i = 0; i < count; ++i) {
		r = xorshift128plus_r(&seed);
		bin = __builtin_popcountl(r);
		bins[bin]++;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	samthread_t threads[N_CHILDREN];
	int i, j;
	double chi;

	if (RUNNING_ON_VALGRIND) {
		puts("random running under valgrind... limiting loops");
		count /= 100;
	}

	for (i = 0; i < 65; ++i)
		expected[i] >>= 31;

	for (i = 0; i < N_CHILDREN; ++i) {
		threads[i] = samthread_create(generator, &child_bins[i]);
		if (threads[i] == (samthread_t)-1) {
			perror("pthread_create");
			exit(1);
		}
	}

	for (i = 0; i < N_CHILDREN; ++i)
		samthread_join(threads[i]);

	for (i = 1; i < N_CHILDREN; ++i)
		for (j = 0; j < 65; ++j)
			child_bins[0][j] += child_bins[i][j];

	chi = 0.0;
	for (i = 0; i < 65; ++i) {
		double diff = (double)child_bins[0][i] - (double)expected[i];
		if (expected[i])
			chi += diff * diff / (double)expected[i];
		if (verbose)
			printf("%2d: %20lu  %20lu     %f\n", i, child_bins[0][i], expected[i], diff);
	}

	printf("chi squared %f\n", chi);

	return 0;
}

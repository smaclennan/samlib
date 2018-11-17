#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

#define LOOPS 3000000

int main(int argc, char *argv[])
{
	uint8_t buf[64], key[TEA_KEY_SIZE];
	unsigned long delta;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	for (int i = 0; i < LOOPS; ++i)
		tea_encrypt(key, buf, sizeof(buf));
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	printf("TEA Encrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / LOOPS);

	gettimeofday(&start, NULL);
	for (int i = 0; i < LOOPS; ++i)
		tea_decrypt(key, buf, sizeof(buf));
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	printf("TEA Decrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / LOOPS);

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -O2 -Wall tea-time.c -o tea-time ../x86_64/libsamlib.a"
 * End:
 */

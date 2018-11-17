#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include "../samlib.h"

#define LOOPS 2000000
#define ENCRYPT_LEN 66 // non multiple of TEA_BAG_SIZE
#define DECRYPT_LEN 72 // tea_bag_size(ENCRYPT_LEN)

int main(int argc, char *argv[])
{
	uint8_t buf[DECRYPT_LEN + 4], expect[DECRYPT_LEN + 4];
	uint8_t key[TEA_KEY_SIZE];
	unsigned long delta;
	struct timeval start, end;

	assert(tea_bag_size(ENCRYPT_LEN) == DECRYPT_LEN);

	// Sanity check
	memset(buf, 'X', sizeof(buf)); // "garbage"
	memset(buf, 'a', ENCRYPT_LEN); // plain-text
	memset(expect, 'X', sizeof(expect)); // "garbage"
	memset(expect, 0, DECRYPT_LEN); // zero pad
	memset(expect, 'a', ENCRYPT_LEN); // plain-text

	tea_encrypt(key, buf, ENCRYPT_LEN);
	tea_decrypt(key, buf, DECRYPT_LEN);
	tea_encrypt(key, buf, ENCRYPT_LEN);
	tea_decrypt(key, buf, DECRYPT_LEN);

	assert(memcmp(buf, expect, sizeof(buf)) == 0);

	gettimeofday(&start, NULL);
	for (int i = 0; i < LOOPS; ++i)
		tea_encrypt(key, buf, ENCRYPT_LEN);
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	printf("TEA Encrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / LOOPS);

	gettimeofday(&start, NULL);
	for (int i = 0; i < LOOPS; ++i)
		tea_decrypt(key, buf, DECRYPT_LEN);
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

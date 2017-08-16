#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

#include "../samlib.h"

#if 1
/* This one is good for testing changes to aes-hw.c routines */
#define LOOPS 100000000UL
#else
/* This one is better for testing software solution */
#define LOOPS 2000000UL
#endif

int main(int argc, char *argv[])
{
#if 1
	char *type = "ECB Encrypt";
	unsigned long i, delta;
	uint8_t buf[64], output[64];
	uint8_t key[AES128_KEYLEN] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	struct timeval start, end;
	aes128_ctx ctx;

	AES128_init_ctx(&ctx, key, 1);

	gettimeofday(&start, NULL);
	for (i = 0; i < LOOPS; ++i) {
		AES128_ECB_encrypt(&ctx, buf +  0, output +  0);
		AES128_ECB_encrypt(&ctx, buf + 16, output + 16);
		AES128_ECB_encrypt(&ctx, buf + 32, output + 32);
		AES128_ECB_encrypt(&ctx, buf + 48, output + 48);
	}
	gettimeofday(&end, NULL);
#else
	char *type = "ECB Decrypt";
	unsigned long i, delta;
	uint8_t buf[64], output[64];
	uint8_t key[AES128_KEYLEN] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	struct timeval start, end;
	aes128_ctx ctx;

	AES128_init_ctx(&ctx, key, 0);

	gettimeofday(&start, NULL);
	for (i = 0; i < LOOPS; ++i) {
		AES128_ECB_decrypt(&ctx, buf +  0, output +  0);
		AES128_ECB_decrypt(&ctx, buf + 16, output + 16);
		AES128_ECB_decrypt(&ctx, buf + 32, output + 32);
		AES128_ECB_decrypt(&ctx, buf + 48, output + 48);
	}
	gettimeofday(&end, NULL);
#endif

	delta = delta_timeval(&start, &end);
	if (ctx.have_hw)
		printf("HW ");
	printf("%s %luus  %.0fns\n", type, delta, (double)delta * 1000.0 / LOOPS);

	return 0;
}

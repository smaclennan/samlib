#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

#ifdef HAVE_VALGRIND_H
#include <valgrind/valgrind.h>
#else
#define RUNNING_ON_VALGRIND 0
#endif

#if defined(WIN32) && defined(_DEBUG)
/* The tests are at least an order of magnitude slower under debug */
#define LOOPS_HW 1000000UL
#define LOOPS_SW 20000UL
#else
/* This one is good for testing changes to aes-hw.c routines */
#define LOOPS_HW 100000000UL
/* This one is better for testing software solution */
#define LOOPS_SW 2000000UL
#endif

#ifdef TESTALL
int aes_stress(void)
#else
int main(int argc, char *argv[])
#endif
{
	unsigned long i, delta, loops, software_only = 0;
	uint8_t buf[64], output[64];
	uint8_t key[AES128_KEYLEN] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	uint8_t iv[AES128_KEYLEN] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
								  0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0 };
	struct timeval start, end;
	aes128_ctx ctx;
	aes_cbc_ctx cbc;

#ifndef TESTALL
	if (argc > 1 && strcmp(argv[1], "-s") == 0)
		software_only = 1;
#endif

	AES128_init_ctx(&ctx, key, 1);
	if (software_only) ctx.have_hw = 0;
	loops = ctx.have_hw ? LOOPS_HW : LOOPS_SW;
	if (RUNNING_ON_VALGRIND) {
		puts("aes-stress running under valgrind... limiting loops");
		loops /= 1000; // the timings are useless
	}

#if 1
	gettimeofday(&start, NULL);
	for (i = 0; i < loops; ++i) {
		AES128_ECB_encrypt(&ctx, buf +  0, output +  0);
		AES128_ECB_encrypt(&ctx, buf + 16, output + 16);
		AES128_ECB_encrypt(&ctx, buf + 32, output + 32);
		AES128_ECB_encrypt(&ctx, buf + 48, output + 48);
	}
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	if (ctx.have_hw)
		printf("HW ");
	printf("ECB 128 Encrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / loops);
#endif

#if 1
	AES128_init_ctx(&ctx, key, 0);
	if (software_only) ctx.have_hw = 0;

	gettimeofday(&start, NULL);
	for (i = 0; i < loops; ++i) {
		AES128_ECB_decrypt(&ctx, buf +  0, output +  0);
		AES128_ECB_decrypt(&ctx, buf + 16, output + 16);
		AES128_ECB_decrypt(&ctx, buf + 32, output + 32);
		AES128_ECB_decrypt(&ctx, buf + 48, output + 48);
	}
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	if (ctx.have_hw)
		printf("HW ");
	printf("ECB 128 Decrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / loops);
#endif

#if 1
	AES_CBC_init_ctx(&cbc, key, iv, 128, 1);
	if (software_only) cbc.have_hw = 0;

	gettimeofday(&start, NULL);
	for (i = 0; i < loops; ++i)
		AES_CBC_encrypt(&cbc, buf, sizeof(buf), output);
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	if (cbc.have_hw)
		printf("HW ");
	printf("CBC 128 Encrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / loops);
#endif

#if 1
	AES_CBC_init_ctx(&cbc, key, iv, 128, 0);
	if (software_only) cbc.have_hw = 0;

	gettimeofday(&start, NULL);
	for (i = 0; i < loops; ++i)
		AES_CBC_decrypt(&cbc, buf, sizeof(buf), output);
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	if (cbc.have_hw)
		printf("HW ");
	printf("CBC 128 Decrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / loops);
#endif

#if 1
	AES_CBC_init_ctx(&cbc, key, iv, 256, 1);
	if (software_only) cbc.have_hw = 0;

	gettimeofday(&start, NULL);
	for (i = 0; i < loops; ++i)
		AES_CBC_encrypt(&cbc, buf, sizeof(buf), output);
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	if (cbc.have_hw)
		printf("HW ");
	printf("CBC 256 Encrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / loops);
#endif

#if 1
	AES_CBC_init_ctx(&cbc, key, iv, 256, 0);
	if (software_only) cbc.have_hw = 0;

	gettimeofday(&start, NULL);
	for (i = 0; i < loops; ++i)
		AES_CBC_decrypt(&cbc, buf, sizeof(buf), output);
	gettimeofday(&end, NULL);

	delta = delta_timeval(&start, &end);
	if (cbc.have_hw)
		printf("HW ");
	printf("CBC 256 Decrypt %luus  %.0fns\n", delta, (double)delta * 1000.0 / loops);
#endif

	return 0;
}

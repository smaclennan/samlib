#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../samlib.h"

/* GCOV CFLAGS=-coverage make D=1
 * 100% Fri Feb 16, 2018
 * Note: LARGE_BUFFERS must be defined for 100% coverage.
 */

static struct ref {
	uint16_t msg[20];
	uint16_t chksum;
} refs[] = {
	{ { 0x4500, 0x003c, 0x1c46, 0x4000, 0x4006,
		0x0000, 0xac10, 0x0a63, 0xac10, 0x0a0c },
	  0xb1e6 },
	{ { 0x4500, 0x003c, 0x1c46, 0x4000, 0x4006,
		0xb1e6, 0xac10, 0x0a63, 0xac10, 0x0a0c },
	  0 },

	{ { 0x4500, 0x0514, 0x42a2, 0x2140, 0x8001,
		0x0000, 0xc0a8, 0x0003, 0xc0a8, 0x0001 },
	  0x50b2 },
	{ { 0x4500, 0x0514, 0x42a2, 0x2140, 0x8001,
		0x50b2, 0xc0a8, 0x0003, 0xc0a8, 0x0001 },
	  0 },

	{ { 0x4500, 0x003c, 0xeb3a, 0x4000, 0x4006,
		0x0000, 0xc0a8, 0x0195, 0xc0a8, 0x0065 },
	  0xcc36 },
	{ { 0x4500, 0x003c, 0xeb3a, 0x4000, 0x4006,
		0xcc36, 0xc0a8, 0x0195, 0xc0a8, 0x0065 },
	  0 },
};
#define N_REFS (sizeof(refs) / sizeof(struct ref))

#ifndef TESTALL
// #define LARGE_BUFFERS
#include "../crc16.c"

int main(int argc, char *argv[])
#else
static int crc16_main(void)
#endif
{
	int i, rc = 0;
	uint16_t chksum;

	for (i = 0; i < N_REFS; ++i) {
		chksum = chksum16(refs[i].msg, 20);
		if (chksum != refs[i].chksum) {
			printf("PROBLEMS: %d: %x != %x\n", i, chksum, refs[i].chksum);
			rc = 1;
		}
	}

	/* Test an odd number of bytes */
	chksum = chksum16("123456789", 9);
	if (chksum != 0x2af6) {
		printf("PROBLEMS: %x != 2af6\n", chksum);
		rc = 1;
	}

#ifdef LARGE_BUFFERS
	/* Needs to be > 64K */
#define LARGE_SIZE ((64 << 10) + 1)
	void *buf = malloc(LARGE_SIZE);
	assert(buf);
	memset(buf, 0xff, LARGE_SIZE);
	chksum = chksum16(buf, LARGE_SIZE);
	if (chksum != 0xff00) {
		printf("PROBLEMS: %x != ff00\n", chksum);
		rc = 1;
	}
#endif

	return rc;
}

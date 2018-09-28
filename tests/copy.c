#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

static char dst[24], dst2[24];

static struct atest {
	char *src;
	char *expect;
	int dstlen;
} tests[] = {
	{ "hello world", "hello world", sizeof(dst) }, // fits
	{ "hello world", "hello world", 12 }, // fits - just
	{ "hello world", "hello worl", 11 }, // truncated - just
	{ "hello world", "hello", 6 }, // truncated
	{ "hello world", "", 1 }, // one
	{ "hello world", NULL, 0 }, // zero
};
#define NTESTS (sizeof(tests) / sizeof(struct atest))

static void setup(int i)
{
	memset(dst, 0xea, sizeof(dst));
	memset(dst2, 0xea, sizeof(dst2));

	if (tests[i].expect == NULL) {
		tests[i].expect = dst2;
		/* Make them real strings so we can strcmp() */
		dst[sizeof(dst) - 1] = 0;
		dst2[sizeof(dst2) - 1] = 0;
	}
}

#ifndef MIN
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#endif

#ifdef TESTALL
int copy_main(void)
#else
int main(void)
#endif
{
	int s1, s2, rc = 0;

	for (int i = 0; i < NTESTS; ++i) {
		int len = strlen(tests[i].src);
		int dlen = MIN(len, MAX(0, tests[i].dstlen - 1));

		setup(i);

		// safecpy
		s1 = safecpy(dst, tests[i].src, tests[i].dstlen);
		s2 = snprintf(dst2, tests[i].dstlen, "%s", tests[i].src);
		if (s1 != dlen) {
			printf("safecpy %d failed len\n", i);
			rc = 1;
		} else if (strcmp(dst, tests[i].expect)) {
			printf("safecpy %d failed cmp\n", i);
			rc = 1;
		} else if (dst[len + 1] != (char)0xea) {
			printf("safecpy %d failed +1\n", i);
			rc = 1;
		}
		if (s1 != MIN(s2, dlen)) {
			printf("safecpy %d len mismatch\n", i);
			rc = 1;
		} else if (memcmp(dst, dst2, sizeof(dst))) {
			printf("safecpy %d cmp mismatch\n", i);
			rc = 1;
		}

		setup(i);

		// strcpy
		s1 = strlcpy(dst, tests[i].src, tests[i].dstlen);
		s2 = snprintf(dst2, tests[i].dstlen, "%s", tests[i].src);
		if (s1 != len) {
			printf("strlcpy %d failed len\n", i);
			rc = 1;
		} else if (tests[i].expect && strcmp(dst, tests[i].expect)) {
			printf("strlcpy %d failed cmp\n", i);
			rc = 1;
		} else if (dst[len + 1] != (char)0xea) {
			printf("strlcpy %d failed +1\n", i);
			rc = 1;
		}
		if (s1 != s2) {
			printf("strlcpy %d len mismatch\n", i);
			rc = 1;
		} else if (memcmp(dst, dst2, sizeof(dst))) {
			printf("strlcpy %d cmp mismatch\n", i);
			rc = 1;
		}
	}

	return rc;
}

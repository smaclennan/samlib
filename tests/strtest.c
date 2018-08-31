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

#define STR "hello world"

static int check_copy(const char *str, int len, int line)
{
	char dst[100], expected[100];
	int n1, n2;

	if (len == 0) return 0; // not worth testing

	memset(dst, '4', sizeof(dst));
	memset(expected, '6', sizeof(expected));

	n1 = snprintf(expected, len, "%s", str);
	n2 = safecpy(dst, str, len);

	/* This is the difference between snprintf and safecpy */
	if (n1 >= len)
		n1 = len - 1;

	if (n1 != n2) {
		fprintf(stderr, "%d: size %d vs %d\n", line, n1, n2);
		return 1;
	}

	if (strcmp(dst, expected)) {
		fprintf(stderr, "%d: str %s vs %s\n", line, expected, dst);
		return 1;
	}

	if (dst[len] != '4') {
		fprintf(stderr, "%d: problems with guard %c\n", line, dst[len]);
		return 1;
	}

	return 0;
}

static void test_strconcat(void)
{
	char str[64], str2[64];
	int n;

	n = strconcat(str, sizeof(str), "hello", " ", "world", NULL);
	assert(n == 11);
	assert(strcmp(str, "hello world") == 0);

	n = strconcat(NULL, 0, "hello", " ", "world", NULL);
	assert(n == 0);

	n = strconcat(str, 10, "hello", " ", "world", NULL);
	assert(n == 9);
	assert(strcmp(str, "hello wor") == 0);

	n = strconcat(str, 5, "hello", " ", "world", NULL);
	assert(n == 4);
	assert(strcmp(str, "hell") == 0);

	strcpy(str2, " and ");
	n = strconcat(str, sizeof(str), "hello", str2, "world", NULL);
	assert(n == 15);
	assert(strcmp(str, "hello and world") == 0);
}

int main(int argc, char *argv[])
{
	int rc = 0;

#if 1
	int len = strlen(STR);

	for (int i = 0; i < len + 2; ++i)
		rc |= check_copy(STR, i, i);

	test_strconcat();
#endif

#if 1
	char dst[256], src[256];
	struct timeval start;
	gettimeofday(&start, NULL);
	for (int i = 0; i < 200000000; ++i)
		safecpy(dst, src, 64);
	unsigned long delta = delta_timeval_now(&start);
	printf("%luus (%fns)\n", delta, (double)delta / 200000000.0 * 1000.0);
#endif

	return rc;
}


/*
 * Local Variables:
 * compile-command: "gcc -O2 -Wall strtest.c -o strtest ../libsamlib.a"
 * End:
 */

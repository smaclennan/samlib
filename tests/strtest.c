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
	char dst1[100], dst2[100], expected[100];
	int n1, n2, n3;

	if (len == 0) return 0; // not worth testing

	memset(dst1, '4', sizeof(dst1));
	memset(dst2, '4', sizeof(dst2));
	memset(expected, '6', sizeof(expected));

	n1 = snprintf(expected, len, "%s", str);
	n2 = safecpy(dst1, str, len);
	n3 = strlcpy(dst2, str, len);

	if (n1 != n3) {
		fprintf(stderr, "lcpy %d: size %d vs %d\n", line, n1, n3);
		return 1;
	}

	/* This is the difference between snprintf and safecpy */
	if (n1 >= len)
		n1 = len - 1;

	if (n1 != n2) {
		fprintf(stderr, "cpy %d: size %d vs %d\n", line, n1, n2);
		return 1;
	}

	if (strcmp(dst1, expected)) {
		fprintf(stderr, "cpy %d: str %s vs %s\n", line, expected, dst1);
		return 1;
	}

	if (strcmp(dst2, expected)) {
		fprintf(stderr, "lcpy %d: str %s vs %s\n", line, expected, dst2);
		return 1;
	}

	if (dst1[len] != '4') {
		fprintf(stderr, "cpy %d: problems with guard %c\n", line, dst1[len]);
		return 1;
	}

	if (dst2[len] != '4') {
		fprintf(stderr, "lcpy %d: problems with guard %c\n", line, dst2[len]);
		return 1;
	}

	return 0;
}

static int check_cat(const char *str, int len, int line)
{
	char dst1[100], dst2[100], expected[100];
	int n1, n2, n3;

	if (len == 0) return 0; // not worth testing

	memset(dst1, '4', sizeof(dst1));
	memset(dst2, '4', sizeof(dst2));
	memset(expected, '6', sizeof(expected));

	strcpy(dst1, "INIT");
	strcpy(dst2, "INIT");

	n1 = snprintf(expected, len, "INIT%s", str);
	n2 = safecat(dst1, str, len);
	n3 = strlcat(dst2, str, len);

	if (n1 != n3) {
		fprintf(stderr, "lcat %d: size %d vs %d\n", line, n1, n3);
		return 1;
	}

	/* This is the difference between snprintf and safecat */
	n1 = len - 5;
	if (n1 > n2) n1 = n2;

	if (n1 != n2) {
		fprintf(stderr, "cat %d: size %d vs %d\n", line, n1, n2);
		return 1;
	}

	if (strcmp(dst1, expected)) {
		fprintf(stderr, "cat %d: str %s vs %s\n", line, expected, dst1);
		return 1;
	}

	if (dst1[len] != '4') {
		fprintf(stderr, "cat %d: problems with guard %c\n", line, dst1[len]);
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

	for (int i = 0; i < len + 2; ++i) {
		rc |= check_copy(STR, i, i);
		rc |= check_cat(STR, i + 5, i);
	}

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

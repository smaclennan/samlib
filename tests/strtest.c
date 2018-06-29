#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

#define STR "hello world"
#define PRE_EXISTING "not "

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

int main(int argc, char *argv[])
{
	int rc = 0;
	int len = strlen(STR);

	for (int i = 0; i < len + 2; ++i)
		rc |= check_copy(STR, i, i);

	char dst[256], src[256];
	struct timeval start;
	gettimeofday(&start, NULL);
	for (int i = 0; i < 100000000; ++i)
		safecpy(dst, src, 256);
	printf("%lu\n", delta_timeval_now(&start));

	return rc;
}


/*
 * Local Variables:
 * compile-command: "gcc -O2 -Wall strtest.c -o strtest ../libsamlib.a"
 * End:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "samlib.h"

void (*must_helper)(const char *what, int size);

static void must_fail(const char *what, int size)
{
	if (must_helper)
		must_helper(what, size);
	else {
		puts("Out of memory.");
		exit(1);
	}
}

char *must_strdup(const char *s)
{
	char *dup = strdup(s);
	if (!dup)
		must_fail("strdup", strlen(s));
	return dup;
}

void *must_alloc(size_t size)
{
	char *mem = malloc(size);
	if (!mem)
		must_fail("malloc", size);
	return mem;
}

void *must_calloc(int nmemb, int size)
{
	char *mem = calloc(nmemb, size);
	if (!mem)
		must_fail("calloc", size * nmemb);
	return mem;
}

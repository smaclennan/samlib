#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "samlib.h"

void (*must_helper)(const char *what, int size);

#define MUST_FAIL(what, size) do {				\
		if (must_helper)						\
			must_helper(what, size);			\
		else {									\
			puts("Out of memory.");				\
			exit(1);							\
		}										\
	} while (0)

char *must_strdup(const char *s)
{
	char *dup = strdup(s);
	if (!dup)
		MUST_FAIL("strdup", strlen(s));
	return dup;
}

void *must_alloc(size_t size)
{
	char *mem = malloc(size);
	if (!mem)
		MUST_FAIL("malloc", size);
	return mem;
}

void *must_calloc(int nmemb, int size)
{
	char *mem = calloc(nmemb, size);
	if (!mem)
		MUST_FAIL("calloc", size * nmemb);
	return mem;
}

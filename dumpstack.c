#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <execinfo.h>

#define MAX_DEPTH 128 /* 128 = 1k */

/* Requires -rdynamic to be useful */
void dump_stack(void)
{
	int i, size;
	char **syms;
	void *buffer[MAX_DEPTH];

	size = backtrace(buffer, MAX_DEPTH);
	syms = backtrace_symbols(buffer, size);

	for (i = 0; i < size; ++i)
		puts(syms[i]);

	free(syms);
}

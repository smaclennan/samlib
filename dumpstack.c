#include <stdio.h>
#include "samlib.h"

#define MAX_DEPTH 128 /* 128 = 1k */

#ifdef __QNXNTO__
void dump_stack(void) { puts("dump_stack not supported"); }
#else
#ifndef WIN32
#ifndef __OpenBSD__
#include <execinfo.h>

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
#endif
#else
#include <DbgHelp.h>

/* Requires DbgHelp.lib */
void dump_stack(void)
{
	void *BackTrace[MAX_DEPTH];
	USHORT n, i;
	uint8_t buf[sizeof(SYMBOL_INFO) + 256  * sizeof(TCHAR)];
	SYMBOL_INFO  *symbol = (SYMBOL_INFO *)buf;

	HANDLE process = GetCurrentProcess();

	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG);

	if (!SymInitialize(process, NULL, TRUE)) {
		puts("SymInitialize failed");
		return;
	}

	n = CaptureStackBackTrace(0, MAX_DEPTH, BackTrace, NULL);

	symbol->MaxNameLen = 256;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	for (i = 0; i < n; ++i) {
		DWORD64 offset = 0;
		if (SymFromAddr(process, (DWORD64)BackTrace[i], &offset, symbol))
			printf("%d: %llx+%llx: %s\n",
				   i, symbol->Address, offset, symbol->Name);
		else
			printf("%d: %llx SymFromAddr returned error : %d\n",
				   i, BackTrace[i], GetLastError());
	}

	SymCleanup(process);
}
#endif
#endif

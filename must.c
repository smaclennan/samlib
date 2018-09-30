#include <stdio.h>
#ifndef WIN32
#include <sys/mman.h>
#endif

#include "samlib.h"

void (*must_helper)(const char *what, int size);

static void must_fail(const char *what, int size)
{
	if (must_helper)
		must_helper(what, size);
	else {
		fputs("Out of memory.\n", stderr);
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
	void *mem = malloc(size);
	if (!mem)
		must_fail("malloc", size);
	return mem;
}

void *must_calloc(int nmemb, int size)
{
	void *mem = calloc(nmemb, size);
	if (!mem)
		must_fail("calloc", size * nmemb);
	return mem;
}

void *must_realloc(void *ptr, int size)
{
	void *mem = realloc(ptr, size);
	if (!mem)
		must_fail("realloc", size);
	return mem;
}

void *must_mmap(int size, int prot, int flags)
{
#ifndef WIN32
	if (prot == 0)
		prot = PROT_READ | PROT_WRITE;
	if (flags == 0)
		flags = MAP_PRIVATE | MAP_ANONYMOUS;

	void *mem = mmap(NULL, size, prot, flags, -1, 0);
	if (!mem)
		must_fail("mmap", size);
	return mem;
#else
	/* for windows it is just a malloc */
	return must_alloc(size);
#endif
}

void *must_mmap_file(int size, int prot, int flags, int fd)
{
#ifndef WIN32
	if (prot == 0)
		prot = PROT_READ;
	if (flags == 0)
		flags = MAP_SHARED;

	void *mem = mmap(NULL, size, prot, flags, fd, 0);
	if (!mem)
		must_fail("mmap file", size);
	return mem;
#else
	int prot2;
	void *mem;
	HANDLE h;

	if (prot == 0 || prot == PROT_READ) {
		prot = PAGE_READONLY;
		prot2 = FILE_MAP_READ;
	} else {
		prot = PAGE_READWRITE;
		prot2 = FILE_MAP_WRITE;
	}

	h = (HANDLE)_get_osfhandle(fd);

	h = CreateFileMapping(h, NULL, prot, 0, 0, NULL);
	if (h) {
		mem = MapViewOfFile(h, prot2, 0, 0, size);
		if (!mem) {
			CloseHandle(h);
			must_fail("mmap file", size);
		}
	} else
		must_fail("mmap file", size);
	return mem;
#endif
}

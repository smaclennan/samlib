#include <stdio.h> /* for snprintf */
#include "samwin32.h"

int optind = 1;
char *optarg;

int getopt(int argc, char *argv[], const char *optstring)
{
	static char *argptr;
	char arg, *p;

	optarg = NULL;

	if (argptr && !*argptr) {
		++optind;
		argptr = NULL;
	}

	if (!argptr) {
		if (optind >= argc)
			return -1;

		argptr = argv[optind];
		if (*argptr++ != '-')
			return -1;
		if (strcmp(argptr, "-") == 0) {
			++optind;
			return -1;
		}
		if (*argptr == '\0')
			return -1;
	}

	arg = *argptr++;
	p = strchr(optstring, arg);
	if (p == NULL)
		return '?';

	if (*(p + 1) == ':') {
		if (*argptr) {
			optarg = argptr;
			argptr = NULL;
			++optind;
		} else if (++optind >= argc)
			return '?';
		else
			optarg = argv[optind];
	}

	return arg;
}

DIR *opendir(const char *dirname)
{
	DIR *dir = calloc(1, sizeof(struct DIR));
	if (!dir)
		return NULL;

	char path[MAX_PATH];
	snprintf(path, sizeof(path), "%s/*", dirname);
	dir->handle = FindFirstFileA(path, &dir->data);
	if (dir->handle == INVALID_HANDLE_VALUE) {
		free(dir);
		return NULL;
	}

	return dir;
}

struct dirent *readdir(DIR *dir)
{
	if (dir->ent.d_name == NULL) {
		/* we filled in data in FindFirstFile above */
		dir->ent.d_name = dir->data.cFileName;
		return &dir->ent;
	}

	if (FindNextFileA(dir->handle, &dir->data))
		return &dir->ent;

	return NULL;
}

void closedir(DIR *dir)
{
	FindClose(dir->handle);
	free(dir);
}

/* Windows uses Jan 1 1601, Unix uses Jan 1 1970 */
#define FILETIME2UNIXTIME 11644473600LL

void gettimeofday(struct timeval *tv, void *ignored)
{
	SYSTEMTIME sys;
	FILETIME file;
	ULARGE_INTEGER now;

	GetSystemTime(&sys);
	SystemTimeToFileTime(&sys, &file);
	now.HighPart = file.dwHighDateTime;
	now.LowPart = file.dwLowDateTime;
	now.QuadPart /= 10; /* convert from 100 nanoseconds to microseconds */

	tv->tv_sec = (long)((now.QuadPart / 1000000) - FILETIME2UNIXTIME);
	tv->tv_usec = now.QuadPart % 1000000;
}

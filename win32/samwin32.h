#ifndef __SAMWIN32_H__
#define __SAMWIN32_H__

#include <windows.h>
#include <io.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

#include "fnmatch.h"
#include "regex.h"

/* WIN32 sure likes underscores */
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup
#define strlwr _strlwr
#define unlink _unlink
#define getcwd _getcwd

#define creat(a, b) _creat(a, _S_IWRITE)
#define open _open
#define read _read
#define write _write
#define lseek _lseek
#define ftruncate _chsize
#define close _close
#define fsync(h) FlushFileBuffers((HANDLE)h)

#define lstat stat

#define S_ISDIR(m) ((m) & _S_IFDIR)
#define S_ISREG(m) ((m) & _S_IFREG)

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

#define access _access
#define umask _umask
#define chdir _chdir
#define putenv _putenv
#define popen _popen
#define pclose _pclose

#define mkdir(a, b) _mkdir(a)

#define inline _inline

struct dirent {
	char *d_name;
};

typedef struct DIR {
	HANDLE handle;
	WIN32_FIND_DATAA data;
	struct dirent ent;
} DIR;

DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dir);
void closedir(DIR *dir);

extern int optind;
extern char *optarg;
int getopt(int argc, char *argv[], const char *optstring);

#define F_OK 0
#define R_OK 4
#define W_OK 2

int getline(char **line, int *len, FILE *fp);

void gettimeofday(struct timeval *tv, void *ignored);

typedef DWORD pid_t;
typedef int mode_t;

#define MAXPATHLEN MAX_PATH

#endif

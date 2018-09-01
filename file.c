#include <fcntl.h>
#include "samlib.h"

/* Create a file of set length and mode. If mode is 0, a reasonable default is chosen. */
int create_file(const char *fname, off_t length, mode_t mode)
{
	if (access(fname, F_OK) == 0)
		return EEXIST;

	if (mode == 0)
		mode = 0644;

	int fd = creat(fname, mode);
	if (fd < 0)
		return errno;

	if (ftruncate(fd, length)) {
		close(fd);
		return errno;
	}

	close(fd);
	return 0;
}

int mktempfile(char *fname, int len)
{
#ifndef WIN32
	char *envtmp;

	if (len < 16)
		return -EINVAL;

	envtmp = getenv("TMPDIR");
	if (!envtmp || strlen(envtmp) >= len -  12)
		envtmp = "/tmp";

	strconcat(fname, len, envtmp, "/tmp.XXXXXX", NULL);

	return mkstemp(fname);
#else
	char path[MAX_PATH - 14];
	char out[MAX_PATH];

	DWORD n = GetTempPath(sizeof(path), path);
	if (n == 0 || n > sizeof(path))
		strcpy(path, "c:/tmp/");

	if (!GetTempFileName(path, "tmp.", 0, out))
		return -EINVAL;

	if (strlen(out) > len)
		return -EINVAL;

	safecpy(fname, out, len);

	return _open(fname, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
#endif
}

/* Create a file name in the temp directory. If fname is NULL, just
 * returns the tmp directory. The returned name is malloced.
 */
char *tmpfilename(const char *fname)
{
#ifdef WIN32
	char tmp[MAX_PATH];

	if (GetTempPath(sizeof(tmp), tmp) == 0)
		strcpy(tmp, "c:/tmp");
#else
	char *tmp = getenv("TMPDIR");
	if (!tmp || !*tmp)
		tmp = "/tmp";
#endif
	int len = strlen(tmp);
	int n = len + 2;
	if (fname)
		n += strlen(fname);

	char *out = malloc(n);
	if (!out)
		return NULL;

	strcpy(out, tmp);
	if (*(tmp + n - 1) == '/')
		*(tmp + n - 1) = 0;

	if (fname) {
		strcat(out, "/");
		while (*fname == '/') ++fname;
		if (*fname)
			strcat(out, fname);
	}

	return out;
}

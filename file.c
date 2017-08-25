#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
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

	snprintf(fname, len, "%s/tmp.XXXXXX", envtmp);

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

	_snprintf(fname, len, "%s", out);

	return _open(fname, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
#endif
}

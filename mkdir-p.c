#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

int mkdir_p(const char *dir)
{
	mode_t mode = umask(0) ^ 0777;

	if (mkdir(dir, mode) == 0 || errno == EEXIST)
		return 0; /* done */

	if (errno != ENOENT)
		return -1;

	int rc = -1;
	char *p, *dirtmp = strdup(dir);
	if (!dirtmp) {
		errno = ENOMEM;
		return -1;
	}

	p = dirtmp;
	if (*p == '/')
		++p;

	while ((p = strchr(p, '/'))) {
		*p = 0;
		if (mkdir(dirtmp, mode) && errno != EEXIST)
			goto failed;
		*p++ = '/';
	}

	if (mkdir(dir, mode) == 0)
		rc = 0;

failed:
	free(dirtmp);
	return rc;
}

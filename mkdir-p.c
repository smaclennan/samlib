#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "samlib.h"

int mkdir_p(const char *dir)
{
	mode_t mode = umask(0) ^ 0777;

	if (mkdir(dir, mode) == 0 || errno == EEXIST)
		return 0; /* done */

	if (errno != ENOENT)
		return -1;

	int rc = -1;
	char *dirtmp = must_strdup(dir);
	char *p = dirtmp;
	if (*p == '/')
		++p;

	while ((p = strchr(p, '/'))) {
		*p = 0;
		if (mkdir(dirtmp, mode) && errno != EEXIST)
			goto failed;
		*p++ = '/';
	}

	/* We could get EEXIST for mkdir_p("a/b/") */
	if (mkdir(dir, mode) == 0 || errno == EEXIST)
		rc = 0;

failed:
	free(dirtmp);
	return rc;
}

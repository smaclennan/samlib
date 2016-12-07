#define _WITH_GETLINE /* for FreeBSD */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include "samlib.h"

/* Default line function, mainly for debugging. */
static int out_line(char *line, void *data)
{
	puts(line);
	return 0;
}

/* Read a pipe command a line at a time calling line_func() for each
 * line. Removes the NL from the line. If line_func() returns
 * non-zero, it will stop reading the file and return 1.
 *
 * The command must be less than 1k.
 *
 * Returns pclose exit status on success, -1 on file or memory
 * error. If the line_func() returns non-zero, the return is -1 and
 * the errno is set to EINTR.
 */
int readcmd(int (*line_func)(char *line, void *data), void *data, const char *fmt, ...)
{
	char cmd[1024], *p, *line = NULL;
	size_t len;
	int rc = 0;

	if (!line_func)
		line_func = out_line;

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);

	FILE *pfp = popen(cmd, "r");
	if (!pfp)
		return -1;

	while (getline(&line, &len, pfp) != EOF) {
		if ((p = strrchr(line, '\n')))
			*p = 0;
		if (line_func(line, data)) {
			rc = 1;
			break;
		}
	}

	int status = pclose(pfp);
	if (rc == 0) {
		if (WIFEXITED(status))
			rc = WEXITSTATUS(status);
		else
			rc = status;
	} else {
		rc = -1;
		errno = EINTR;
	}

	return rc;
}

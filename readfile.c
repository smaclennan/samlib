#define _WITH_GETLINE /* for FreeBSD */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "samlib.h"

#ifdef __QNXNTO__
#define GETLINE_INCREMENT 128 /* real getline 120 */

/* Simple getline implementation */
ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
	char *p;

	if (!*lineptr) {
		*lineptr = malloc(GETLINE_INCREMENT);
		if (!*lineptr) {
			errno = ENOMEM;
			return -1;
		}
		*n = GETLINE_INCREMENT;
	}

	if (!fgets(*lineptr, *n, stream)) {
		errno = feof(stream) ? 0 : EIO;
		return -1;
	}

	while (!(p = strchr(*lineptr, '\n')) && !feof(stream)) {
		char *new = realloc(*lineptr, *n + GETLINE_INCREMENT);
		if (!new) {
			errno = ENOMEM;
			return -1;
		}
		*lineptr = new;
		if (!fgets(*lineptr + *n - 1, GETLINE_INCREMENT + 1, stream)) {
			errno = feof(stream) ? 0 : EIO;
			return -1;
		}
		*n += GETLINE_INCREMENT;
	}

	return strlen(*lineptr);
}
#endif

/* Read a file line at a time calling line_func() for each
 * line. Removes the NL from the line. If line_func() returns
 * non-zero, it will stop reading the file and return 1.
 *
 * If path is NULL, reads input from stdin.
 *
 * Returns 0 on success, -1 on file or memory error, or 1 if
 * line_func() returns non-zero.
 */
int readfile(int (*line_func)(char *line, void *data), void *data,
			 const char *path, unsigned flags)
{
	FILE *fp = path ? fopen(path, "rb") : stdin;
	if (!fp)
		return -1;

	char *p, *line = NULL;
	size_t len = 0;
	int rc = 0;

	if (!line_func) {
		errno = EINVAL;
		return -1;
	}

	while (getline(&line, &len, fp) != EOF) {
		if ((p = strrchr(line, '\n')))
			*p = 0;
		if (flags & RF_IGNORE_EMPTY) {
			for (p = line; isspace(*p); ++p) ;
			if (*p == 0)
				continue;
		}
		if (flags & RF_IGNORE_COMMENTS) {
			if (*line == '#')
				continue;
		}
		if (line_func(line, data)) {
			rc = 1;
			goto closeit;
		}
	}

	if (!feof(fp))
		rc = -1;

closeit:
	free(line);

	if (path && fclose(fp))
		return -1;

	return rc;
}

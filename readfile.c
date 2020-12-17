#define _WITH_GETLINE /* for FreeBSD */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "samlib.h"

#if defined(WIN32)
#define GETLINE_INCREMENT 128

/* Simple getline implementation */
int getline(char **line, int *n, FILE *fp)
{
	int ch, i = 0;

	while (1) {
		if (i >= *n - 1) {
			char *p = realloc(*line, *n + GETLINE_INCREMENT);
			if (!p)
				return EOF;
			*line = p;
			*n += GETLINE_INCREMENT;
		}

		ch = fgetc(fp);
		if (ch == EOF) {
			if (i == 0)
				return EOF;
			*(*line + i) = 0;
			return i;
		}

		*(*line + i++) = ch;

		if (ch == '\n') {
			*(*line + i) = 0;
			return 1;
		}
	}
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

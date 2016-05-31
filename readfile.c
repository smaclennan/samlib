#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "samlib.h"

/* Default line function, mainly for debugging. */
static int out_line(char *line, void *data)
{
	puts(line);
	return 0;
}

/* Read a file line at a time calling line_func() for each
 * line. Removes the NL from the line. If line_func() returns
 * non-zero, it will stop reading the file and return 1.
 *
 * If path is NULL, reads input from stdin.
 *
 * Returns 0 on success, -1 on file or memory error, or 1 if
 * line_func() returns non-zero.
 */
int readfile(int (*line_func)(char *line, void *data), void *data, const char *path)
{
	FILE *fp = path ? fopen(path, "r") : stdin;
	if (!fp)
		return -1;

	char *p, *line = NULL;
	size_t len = 0;
	int rc = 0;

	if (!line_func)
		line_func = out_line;

	while (getline(&line, &len, fp) != EOF) {
		if ((p = strrchr(line, '\n')))
			*p = 0;
		if (line_func(line, data)) {
			rc = 1;
			goto closeit;
		}
	}

	if (!feof(fp))
		rc = -1;

closeit:
	if (line)
		free(line);

	if (path && fclose(fp))
		return -1;

	return rc;
}

#ifndef __SAMLIB_H__
#define __SAMLIB_H__

/* Read a file line at a time calling line_func() for each
 * line. Removes the NL from the line. If line_func() returns
 * non-zero, it will stop reading the file and return 1.
 *
 * Returns 0 on success, -1 on file or memory error, or 1 if
 * line_func() returns non-zero.
 */
int readfile(const char *path, int (*line_func)(char *line, void *data), void *data);

#endif

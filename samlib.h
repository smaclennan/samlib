#ifndef __SAMLIB_H__
#define __SAMLIB_H__

#define SAMLIB_MAJOR 1
#define SAMLIB_MINOR 0

#include <sys/stat.h>

/* Read a file line at a time calling line_func() for each
 * line. Removes the NL from the line. If line_func() returns
 * non-zero, it will stop reading the file and return 1.
 *
 * Returns 0 on success, -1 on file or memory error, or 1 if
 * line_func() returns non-zero.
 */
int readfile(int (*line_func)(char *line, void *data), void *data, const char *path);

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
int readcmd(int (*line_func)(char *line, void *data), void *data, const char *fmt, ...);

void samlib_version(int *major, int *minor);
const char *samlib_versionstr(void);


/* Main function. Path can be a directory or a file. The file_func()
 * will be called on every file except . (dot) files. Honors both
 * ignores and filter if set.
 */
int walkfiles(const char *path,
			  int (*file_func)(const char *path, struct stat *sbuf),
			  int flags);

/* By default links are ignored. Set this flag to process links as
 * files.  NOTE: This flag is ignored for dirs/files specified as the
 * path argument to walkfiles().
 */
#define WALK_LINKS 1

/* Print out what walkfiles is doing for debugging. */
#define WALK_VERBOSE 2

/* Adds a regular expression of paths to ignore. */
void add_ignore(const char *str);

/* Check if a path matches one of the ignores. */
int check_ignores(const char *path);

/* Sets a filter to match on every file. This uses file globbing, not
 * regular expressions.
 */
void add_filter(const char *pat);

/* Check if the file name matches a filter */
int check_filters(const char *fname);

/* Example of ignores vs filtering. If you only want to process text
 * files, you might use a filter of "*.txt". If you want to ignore all
 * text files, you could use an ignore of "\.txt$".
 */

#endif

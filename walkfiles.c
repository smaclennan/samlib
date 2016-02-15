#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <regex.h>
#include <fnmatch.h>

#include "samlib.h"

struct ignore {
	regex_t regex;
	struct ignore *next;
};

struct filter {
	char *pat;
	struct filter *next;
};

static struct walkfile_struct global_walkfiles;

#define walk_verbose (walk->flags & WALK_VERBOSE)

#define walk_ignores							\
	struct ignore *ignores;						\
	if (walk)									\
		ignores = walk->ignores;				\
	else										\
		ignores = global_walkfiles.ignores

#define walk_filters							\
	struct filter *filters;						\
	if (walk)									\
		filters = walk->filters;				\
	else										\
		filters = global_walkfiles.filters

void add_ignore(struct walkfile_struct *walk, const char *str)
{
	walk_ignores;

	struct ignore *ignore = calloc(1, sizeof(struct ignore));
	if (!ignore) {
		puts("out of memory");
		exit(1);
	}

	if (regcomp(&ignore->regex, str, REG_EXTENDED)) {
		printf("Invalid regex '%s'\n", str);
		exit(1);
	}

	ignore->next = ignores;
	ignores = ignore;
}

int check_ignores(struct walkfile_struct *walk, const char *path)
{
	walk_ignores;
	if (!ignores) return 0;

	struct ignore *ignore;
	regmatch_t pmatch[1];
	for (ignore = ignores; ignore; ignore = ignore->next)
		if (regexec(&ignore->regex, path, 1, pmatch, 0) == 0)
			return 1;

	return 0;
}

void add_filter(struct walkfile_struct *walk, const char *pat)
{
	struct filter *f;
	walk_filters;

	for (f = filters; f; f = f->next)
		if (strcmp(f->pat, pat) == 0)
			return;

	f = calloc(1, sizeof(struct filter));
	if (f) f->pat = strdup(pat);
	if (!f || !f->pat) {
		puts("out of memory");
		exit(1);
	}

	f->next = filters;
	filters = f;
}

int check_filters(struct walkfile_struct *walk, const char *fname)
{
	walk_filters;
	if (!filters) return 1;

	struct filter *f;
	for (f = filters; f; f = f->next)
		if (fnmatch(f->pat, fname, 0) == 0)
			return 1;

	return 0;
}

static int do_dir(struct walkfile_struct *walk, const char *dname)
{
	/* 2015/9/8 Largest path on zonker is 239 */
	char path[1024];
	int error = 0;

	DIR *dir = opendir(dname);
	if (!dir) {
		perror(dname);
		return 1;
	}

	struct dirent *ent;
	while ((ent = readdir(dir))) {
		if (*ent->d_name == '.') continue;

		int n = snprintf(path, sizeof(path), "%s/%s", dname, ent->d_name);
		if (n >= sizeof(path)) {
			printf("PATH TRUNCATED: %s/%s\n", dname, ent->d_name);
			error = 1;
			continue;
		}

		if (check_ignores(walk, path)) {
			if (walk_verbose) printf("Ignoring: %s\n", path);
			continue;
		}

		struct stat sbuf;
		if (lstat(path, &sbuf)) {
			perror(path);
			error = 1;
			continue;
		}

		/* Deal with links */

		if (S_ISDIR(sbuf.st_mode))
			error |= do_dir(walk, path);
		else if (S_ISLNK(sbuf.st_mode) && (walk->flags & WALK_LINKS) == 0) {
			if (walk_verbose) printf("Link %s\n", path);
		} else if (check_filters(walk, ent->d_name))
			error |= walk->file_func(path, &sbuf);
		else if (walk_verbose)
			printf("Skipping %s\n", ent->d_name);
	}

	closedir(dir);

	return error;
}

/* Path can be a directory or a file.
 * Note: directories ignore .(dot) files!
 */
int walkfiles(struct walkfile_struct *walk, const char *path,
			  int (*file_func)(const char *path, struct stat *sbuf),
			  int flags)
{
	struct stat sbuf;
	if (lstat(path, &sbuf)) {
		perror(path);
		return -1;
	}

	if (walk == NULL)
		walk = &global_walkfiles;

	if (file_func)
		walk->file_func = file_func;
	else if (!walk->file_func) {
		errno = EINVAL;
		return -1;
	}
	walk->flags |= flags;

	if (S_ISDIR(sbuf.st_mode))
		return do_dir(walk, path);
	else
		return walk->file_func(path, &sbuf);
}

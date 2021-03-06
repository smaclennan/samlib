#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <dirent.h>
#include <regex.h>
#include <fnmatch.h>
#endif

#include "samlib.h"

struct ignore {
	regex_t regex;
	struct ignore *next;
};

struct filter {
	char *pat;
	regex_t regex;
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

	struct ignore *ignore = must_calloc(1, sizeof(struct ignore));

	if (regcomp(&ignore->regex, str, REG_EXTENDED)) {
		fprintf(stderr, "Invalid regex '%s'\n", str);
		exit(1);
	}

	ignore->next = ignores;
	if (walk)
		walk->ignores = ignore;
	else
		global_walkfiles.ignores = ignore;
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

	f = must_calloc(1, sizeof(struct filter));
	f->pat = must_strdup(pat);

	f->next = filters;
	if (walk)
		walk->filters = f;
	else
		global_walkfiles.filters = f;
}

void add_filter_re(struct walkfile_struct *walk, const char *regex)
{
	struct filter *f;
	walk_filters;

	f = must_calloc(1, sizeof(struct filter));

	if (regcomp(&f->regex, regex, REG_EXTENDED)) {
		fprintf(stderr, "Invalid regex '%s'\n", regex);
		exit(1);
	}

	f->next = filters;
	if (walk)
		walk->filters = f;
	else
		global_walkfiles.filters = f;
}

int check_filters(struct walkfile_struct *walk, const char *fname)
{
	walk_filters;
	if (!filters) return 1;

	regmatch_t pmatch[1];
	for (struct filter *f = filters; f; f = f->next)
		if (f->pat) {
			if (fnmatch(f->pat, fname, 0) == 0)
				return 1;
		} else {
			if (regexec(&f->regex, fname, 1, pmatch, 0) == 0)
				return 1;
		}

	return 0;
}

static int do_dir(struct walkfile_struct *walk, const char *dname, struct stat *dbuf)
{
	/* 2016/5/30 Largest path on zonker is 260 */
	char path[1024];
	int n, error = 0;
	mode_t match;

	DIR *dir = opendir(dname);
	if (!dir) {
		perror(dname);
		return 1;
	}

	struct dirent *ent;
	while ((ent = readdir(dir))) {
		if (*ent->d_name == '.') {
			if (walk->flags & WALK_DOTFILES) {
				if (ent->d_name[1] == 0 || strcmp(ent->d_name, "..") == 0)
					continue;
			} else
				continue;
		}

		if (strcmp(dname, "/"))
			n = strconcat(path, sizeof(path), dname, "/", ent->d_name, NULL);
		else
			n = strconcat(path, sizeof(path), "/", ent->d_name, NULL);
		if (n >= sizeof(path)) {
			fprintf(stderr, "PATH TRUNCATED: %s/%s\n", dname, ent->d_name);
			error = 1;
			continue;
		}

		if (check_ignores(walk, path)) {
			if (walk_verbose) fprintf(stderr, "Ignoring: %s\n", path);
			continue;
		}

		struct stat sbuf;
		if (lstat(path, &sbuf)) {
			perror(path);
			error = 1;
			continue;
		}

		match = sbuf.st_mode & S_IFMT;
		if (walk->flags & WALK_ONE_DIR) {
			if (match == S_IFDIR)
				// treat directories as normal files
				match = S_IFREG;
		}
		switch (match) {
		case S_IFDIR:
			if (!(walk->flags & WALK_NO_SUBDIRS)) {
				if (walk->flags & WALK_XDEV)
					if (dbuf->st_dev != sbuf.st_dev) {
						if (walk_verbose)
							fprintf(stderr, "Skipping dir %s\n", path);
						break;
					}
				error |= do_dir(walk, path, &sbuf);
			}
			break;
		default:
			if ((match & walk->flags & S_IFMT) != match) {
				if (walk_verbose)
					fprintf(stderr, "Special %s (0%o)\n", path, match);
				break;
			}
			/* fall thru */
		case S_IFREG:
			if (check_filters(walk, ent->d_name))
				error |= walk->file_func(path, &sbuf);
			else if (walk_verbose)
				fprintf(stderr, "Skipping %s\n", ent->d_name);
		}
	}

	closedir(dir);

	return error;
}

/* The default file_func. Mainly for debugging. */
static int out_files(const char *path, struct stat *sbuf)
{
	puts(path);
	return 0;
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
	else if (!walk->file_func)
		walk->file_func = out_files;

	walk->flags |= flags; /* add in user flags */
	if (walk_verbose)
		fprintf(stderr, "walk flags 0%o\n", walk->flags);

	if (S_ISDIR(sbuf.st_mode))
		return do_dir(walk, path, &sbuf);
	else
		return walk->file_func(path, &sbuf);
}

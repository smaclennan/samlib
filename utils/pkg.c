#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>


#define PKG_DIR "/var/log/packages"

static int quiet;
static int all;
static int find_file;
static int full_name;
static int non_standard;
static int standard;
static int flags;
#define DUMP_INFO 1
#define DUMP_LIST 2
static char version[20];

static char *isolate(char *dname)
{
	static char *name;
	int i;

	if (name)
		free(name);
	name = strdup(dname);
	if (!name) {
		printf("Out of memory\n");
		exit(1);
	}

	for (i = 0; i < 3; ++i) {
		char *p = strrchr(name, '-');
		if (!p || p == name) {
			printf("Error %d %s\n", i, dname);
			return NULL;
		}
		*p = '\0';
	}

	return name;
}

static int do_find_file(char *dname, char *fname)
{
	char line[256];
	int mlen = strlen(fname);
	FILE *fp = fopen(dname, "r");
	if (!fp) {
		perror(dname);
		return 0;
	}

	while (fgets(line, sizeof(line), fp)) {
		char *p = strchr(line, '\n');
		if (p)
			*p = '\0';
		if (all && strncmp(line, fname, mlen) == 0) {
			printf("Match %s\n", line);
			fclose(fp);
			return 1;
		} else if (strcmp(line, fname) == 0) {
			fclose(fp);
			return 1;
		}
	}

	fclose(fp);
	return 0;
}

static void do_match(char *dname)
{
	FILE *fp;
	char line[256];
	int print = flags & DUMP_INFO;
	int empty = 0;

	if (quiet)
		return;

	if (flags == 0) {
		puts(dname);
		return;
	}

	fp = fopen(dname, "r");
	if (!fp) {
		perror(dname);
		return;
	}

	while (fgets(line, sizeof(line), fp)) {
		if (strncmp(line, "FILE LIST:", 10) == 0)
			print = !!(flags & DUMP_LIST);
		if (print) {
			if (print > 1) {
				char *p = strchr(line, ':');
				if (p) {
					for (++p; *p == ' '; ++p) ;
					if (*p == '\n')
						empty = 1;
					else {
						if (empty) {
							putchar('\n');
							empty = 0;
						}
						fputs(p, stdout);
					}
				} else
					fputs(line, stdout);
			} else
				fputs(line, stdout);
			if (strncmp(line, "PACKAGE DESCRIPTION:", 20) == 0)
				++print;
		}
	}

	fclose(fp);
}

static void get_version(void)
{
	int n = 0, major, minor;
	FILE *fp = fopen("/etc/slackware-version", "r");
	if (fp) {
		n = fscanf(fp, "Slackware %d.%d", &major, &minor);
		fclose(fp);
	}
	if (n != 2) {
		puts("You are not running Slackware.");
		exit(1);
	}
	snprintf(version, sizeof(version), "slack%d.%d", major, minor);
}

static int is_nonstandard(char *pkg)
{
	char *p = strrchr(pkg, '-');
	if (!p) {
		if (*pkg != '.')
			printf("Problems: %s\n", pkg);
		return 1; /* errors non-standard */
	}

	for (++p; isdigit(*p); ++p)
		;
	if (*p == '\0')
		return 0;

	if (*p == '_')
		++p;
	if (strncmp(p, version, strlen(version)) == 0)
		return 0;

	if (non_standard > 1)
		if (strcmp(p, "SBo") == 0)
			return 0; /* not non-standard enough ;) */

	return 1;
}

static void check_pkg(char *pkg)
{
	int rc = is_nonstandard(pkg);
	if (standard) {
		if (rc == 0)
			puts(pkg);
	} else if (rc)
		puts(pkg);
}

static void usage(void)
{
	puts("usage:  pkg [-ailq] pkg\n"
	     "\tpkg [-ailq] -f file\n"
	     "\tpkg [-ailq] -p pkgfile\n"
		 "\tpkg -n|-s\n"
		 "where:  -a  show all matching pkgs\n"
		 "\t-i  display info about pkg\n"
		 "\t-l  list files in pkg\n"
		 "\t-n  lists non-standard packages\n"
		 "\t-nn exclude SBo packages\n"
		 "\t-s  lists standard packages only\n"
		 "\t-q  quiet");
	exit(1);
}

int main(int argc, char *argv[])
{
	int c, mlen, rc = 1;
	char *match;
	DIR *dirp;
	struct dirent *dent;

	while ((c = getopt(argc, argv, "afhilnpqs")) != EOF)
		switch (c) {
		case 'a':
			all = 1;
			break;
		case 'f':
			find_file = 1;
			break;
		case 'i':
			flags |= DUMP_INFO;
			break;
		case 'h':
			usage();
			break;
		case 'l':
			flags |= DUMP_LIST;
			break;
		case 'n':
			++non_standard;
			break;
		case 'p':
			full_name = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 's':
			standard = 1;
			break;
		default:
			puts("Sorry!");
			usage();
		}

	if (non_standard || standard) {
		/* compiler shutups - we never access these */
		match = NULL;
		mlen = 0;
		get_version();
	} else {
		if (optind == argc) {
			puts("I need a name!");
			exit(1);
		}

		match = argv[optind];
		mlen = strlen(match);

		if (find_file) {
			if (*match == '/') {
				++match;
				--mlen;
			}
		} else if (full_name) {
			match = isolate(match);
			if (!match)
				exit(1);
			match = strdup(match);
			if (!match)
				exit(1);
			mlen = strlen(match);
		}
	}

	dirp = opendir(PKG_DIR);
	if (!dirp || chdir(PKG_DIR)) {
		perror(PKG_DIR);
		puts("Are you sure you are running Slackware?");
		exit(1);
	}

	while ((dent = readdir(dirp)))
		if (*dent->d_name == '.') {
			/* nop */
		} else if (non_standard || standard)
			check_pkg(dent->d_name);
		else if (find_file) {
			if (do_find_file(dent->d_name, match)) {
				do_match(dent->d_name);
				rc = 0;
			}
		} else if (strncmp(dent->d_name, match, mlen) == 0) {
			char *name = isolate(dent->d_name);
			if (!all && strcmp(name, match))
				continue;
			do_match(dent->d_name);
			rc = 0;
		}

	closedir(dirp);

	return rc;
}

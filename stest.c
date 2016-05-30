#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "samlib.h"

int outit(char *line, void *data)
{
	puts(line);
	return 0;
}

int outdir(const char *path, struct stat *sbuf)
{
	static char lastdir[1024];
	char *p;

	if (!(p = strrchr(path, '/')))
		return 0;

	*p = 0;
	if (strcmp(path, lastdir)) {
		puts(path);
		strcpy(lastdir, path);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int c, major, minor;

	while ((c = getopt(argc, argv, "-V")) != EOF)
		if (c == 'V') {
			samlib_version(&major, &minor);
			printf("Version '%d.%d'  '%s'\n",
				   major, minor, samlib_versionstr());
			exit(0);
		}

	if (argc == 1) {
		fputs("I need a filename!\n", stderr);
		exit(1);
	}

#if 0
	return readfile(outit, NULL, argv[1]) != 0;
#elif 0
	return readcmd(outit, NULL, "ls %s", argv[1]);
#elif 0
	return do_system("ls %s", argv[1]);
#else
	return walkfiles(NULL, argv[1], outdir, WALK_XDEV);
#endif
}

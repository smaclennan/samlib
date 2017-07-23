#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "samlib.h"

int main(int argc, char *argv[])
{
#if 1
	int c, flags = 0;

	while ((c = getopt(argc, argv, "f:vV")) != EOF)
		switch (c) {
		case 'v':
			flags |= WALK_VERBOSE;
			break;
		case 'f':
			flags |= strtol(optarg, NULL, 0);
			break;
		case 'V':
			printf("Version  '%s'\n", samlib_version);
			exit(0);
		}

	if (argc == optind) {
		fputs("I need a directory!\n", stderr);
		exit(1);
	}

	return walkfiles(NULL, argv[optind], NULL, 0);
#endif
}

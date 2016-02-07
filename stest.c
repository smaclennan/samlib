#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "samlib.h"

static int outit(char *line, void *data)
{
	puts(line);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		fputs("I need a filename!\n", stderr);
		exit(1);
	}

#if 0
	return readfile(outit, NULL, argv[1]) != 0;
#else
	return readcmd(outit, NULL, "ls %s", argv[1]);
#endif
}

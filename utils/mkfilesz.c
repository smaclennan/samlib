#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include <samlib.h>

static int verbose;

static int must_write(int fd, const void *buf, unsigned size, unsigned max)
{
	if (size > max)
		size = max;

	int n = write(fd, buf, size);
	if (n <= 0) {
		perror("write failed");
		close(fd);
		exit(1);
	}

	return n;
}

static void usage(char *prog, int rc)
{
	char *p = strrchr(prog, '/');
	if (p) prog = p;

	printf("usage: %s [-vz] [-s <size>] file\n"
		   "where:\t-s <size>\tis the file size (defaults to 0)\n"
		   "\t-v\t\tmore verbose\n"
		   "\t-z\t\tzero fill the file (defaults to an 80 char line)\n"
		   "Note: size is in bytes, but can end in k for kilobytes or m for megabytes.\n"
		   , prog);

	exit(rc);
}

int main(int argc, char *argv[])
{
	char *p;
	int c, fd, zero = 0;
	unsigned size = 0;

	while ((c = getopt(argc, argv, "hs:vz")) != EOF)
		switch (c) {
		case 'h':
			usage(argv[0], 0);
		case 's':
			size = strtol(optarg, &p, 0);
			switch (*p) {
			case 'm':
			case 'M':
				size *= 1024;
				/* fall thru */
			case 'k':
			case 'K':
				size *= 1024;
				break;
			case 'b':
			case 'B':
			case '\0':
				break;
			default:
				printf("Unexpected unit '%s'\n", p);
				exit(1);
			}
			break;

		case 'v':
			++verbose;
			break;
		case 'z':
			zero = 1;
			break;
		default:
			usage(argv[0], 1);
		}

	if (optind == argc)
		usage(argv[0], 1);

	if (size == 0 || zero) {
		if (create_file(argv[optind], size, 0)) {
			perror(argv[optind]);
			exit(1);
		}
		return 0;
	}

	if ((fd = creat(argv[optind], 0644)) < 0) {
		perror(argv[optind]);
		exit(1);
	}

	/* 80 char line */
	char line[80];

	for (p = line, c = '0'; c <= '~'; ++c)
		*p++ = c;
	*p = '\n';

	while (size > 0)
		size -= must_write(fd, line, size, 80);

	close(fd);
	return 0;
}

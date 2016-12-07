#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <db.h>

#include "samlib.h"

int walk_puts(char *key, void *data, int len)
{
	printf("%s: %.*s\n", key, len, (char *)data);
	return 0;
}

int main(int argc, char *argv[])
{
#if 1
	int c, major, minor, flags = 0;

	while ((c = getopt(argc, argv, "f:vV")) != EOF)
		switch (c) {
		case 'v':
			flags |= WALK_VERBOSE;
			break;
		case 'f':
			flags |= strtol(optarg, NULL, 0);
			break;
		case 'V':
			samlib_version(&major, &minor);
			printf("Version  '%d.%d'  '%s'\n",
				   major, minor, samlib_versionstr());
			exit(0);
		}

	if (argc == optind) {
		fputs("I need a directory!\n", stderr);
		exit(1);
	}

	return walkfiles(NULL, argv[optind], NULL, 0);
#elif 0
	char str[8];

	if (db_open("/tmp/dbtest", DB_CREATE, NULL)) {
		puts("Unable to open DB");
		exit(1);
	}

	if (db_put_str(NULL, "c", "3") ||
		db_put_str(NULL, "b", "2") ||
		db_put_str(NULL, "a", "1"))
		printf("Puts failed\n");

	db_get_str(NULL, "b", str, sizeof(str));
	if (strcmp(str, "2"))
		printf("Problem: expected 2 got %s\n", str);

	db_walk(NULL, walk_puts);
#else
	struct timeval start, end;

	start.tv_sec = 10;
	start.tv_usec = 100;
	end.tv_sec = 10;
	end.tv_usec = 101;

	if (delta_timeval(&start, &end) != 1)
		puts("PROBS 1");

	start.tv_sec = 10;
	start.tv_usec = 999999;
	end.tv_sec = 11;
	end.tv_usec = 0;

	if (delta_timeval(&start, &end) != 1)
		puts("PROBS 2");

	start.tv_sec = 10;
	start.tv_usec = 100;
	end.tv_sec = 11;
	end.tv_usec = 101;

	if (delta_timeval(&start, &end) != 1000001)
		puts("PROBS 3");

	start.tv_sec = 10;
	start.tv_usec = 999999;
	end.tv_sec = 12;
	end.tv_usec = 0;

	if (delta_timeval(&start, &end) != 1000001)
		puts("PROBS 4");

	start.tv_sec = 10;
	start.tv_usec = 101;
	end.tv_sec = 10;
	end.tv_usec = 100;

	if (delta_timeval(&start, &end) != ~0UL)
		puts("PROBS 5");

	start.tv_sec = 10;
	start.tv_usec = 999999;
	end.tv_sec = 10;
	end.tv_usec = 0;

	if (delta_timeval(&start, &end) != ~0UL)
		puts("PROBS 6");

	return 0;
#endif
}

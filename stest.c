#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_DB_H
#include <db.h>
#endif

#include "samlib.h"

int main(int argc, char *argv[])
{
#if 0
	pid_t pid, ppid;
	int rc = 0;

	if (argc == 1) {
		pid = getpid();
		ppid = getppid();
	} else {
		pid = strtol(argv[1], NULL, 0);
		ppid = -1;
	}

	struct procstat_min stat;
	if (readprocstat(pid, &stat)) {
		printf("Failed %d\n", pid);
		exit(1);
	}

	printf("%d %s %c ppid %d pgrp %d session %d\n",
		   stat.pid, stat.comm, stat.state,
		   stat.ppid, stat.pgrp, stat.session);

	if (stat.pid != pid) {
		printf("PROBS pid %d != %d\n", stat.pid, pid);
		rc = 1;
	}

	if (ppid != -1 && stat.ppid != ppid) {
		printf("PROBS ppid %d != %d\n", stat.pid, pid);
		rc = 1;
	}

	return rc;
#elif 1
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
#elif 1 && defined(HAVE_DB_H)
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

	db_walk(NULL, db_walk_puts);
#elif 1 && defined(HAVE_DB_H)
	if (db_open("/tmp/dbtest", DB_CREATE, NULL)) {
		puts("Unable to open DB");
		exit(1);
	}

	if (db_update_long(NULL, "c", 3) ||
		db_update_long(NULL, "b", 2) ||
		db_update_long(NULL, "a", 1))
		printf("Puts failed\n");

	db_walk(NULL, db_walk_long);
#endif
}

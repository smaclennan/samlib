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
#endif
}

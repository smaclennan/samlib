#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"
#include "../samthread.h"


static int thread(void *arg)
{
	struct procstat_min *stat = arg;

	if (readprocstat(gettid(), stat)) {
		printf("Child failed readprocstat\n");
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	pid_t pid, ppid;
	samthread_t tid;
	struct procstat_min stat, childstat;
	int rc = 0;

	pid = getpid();
	ppid = getppid();

	if (pid != gettid()) {
		printf("Hmm, pid %d tid %d\n", pid, gettid());
		rc = 1;
	}

	if (readprocstat(pid, &stat)) {
		printf("Failed %d\n", pid);
		return 1;
	}

	if (stat.pid != pid) {
		printf("PROBS pid %d != %d\n", stat.pid, pid);
		rc = 1;
	}

	if (stat.ppid != ppid) {
		printf("PROBS ppid %d != %d\n", stat.pid, pid);
		rc = 1;
	}

	memset(&childstat, 0, sizeof(childstat));
	tid = samthread_create(thread, &childstat);
	if (tid == (samthread_t)-1) {
		printf("Unable to create child thread\n");
		rc = 1;
	} else {
		samthread_join(tid);

		if (childstat.pid == pid) {
			printf("PROBS pid %d = %d\n", childstat.pid, pid);
			rc = 1;
		}

		if (childstat.ppid != ppid) {
			printf("PROBS ppid %d != %d\n", childstat.ppid, ppid);
			rc = 1;
		}
	}

	return rc;
}

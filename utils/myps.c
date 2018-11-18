#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>
#include "samlib.h"

static struct aproc {
	const char *cmd;
	int count;
	pid_t pid;
	unsigned long long time;
} *procs;
static int curproc;

static pid_t me;

static int add_proc(const char *path, struct stat *sbuf)
{
	static int maxproc;

	struct aproc *p;
	pid_t pid = strtol(path + 6, NULL, 10);
	if (pid == me) return 0;

	char buf[0x1001];
	int n = readproccmdline(pid, buf, sizeof(buf));
	if (n <= 0) {
		if (n < 0)
			fprintf(stderr, "%d: readproc failed\n", pid);
		return 0;
	}

	struct procstat_min stat;
	if (readprocstat(pid, &stat)) {
		fprintf(stderr, "%d: readprocstat failed\n", pid);
		return 0;
	}

	/* /bin/sh is a special case */
	char *cmd = buf;
	if (strncmp(cmd, "/bin/sh", 7) == 0) {
		if (*(cmd + 7)) {
#if 1
			// This drops the /bin/sh
			cmd += 8;
#else
			// This keeps the /bin/sh
			char *ptr = strchr(cmd + 8, ' ');
			if (ptr) *ptr = 0;
#endif
		}
	} else {
		char *ptr = strchr(cmd, ' ');
		if (ptr) *ptr = 0;
	}

	p = procs;
	for (int i = 0; i < curproc; ++i, ++p)
		if (strcmp(cmd, p->cmd) == 0) {
			++p->count;
			if (pid < p->pid)
				p->pid = pid;
			if (stat.starttime < p->time)
				p->time = stat.starttime;
			return 0;
		}

	if (curproc >= maxproc) {
		maxproc += 10;
		procs = must_realloc(procs, maxproc * sizeof(struct aproc));
	}
	p = &procs[curproc++];
	p->cmd = must_strdup(cmd);
	p->count = 1;
	p->pid = pid;
	p->time = stat.starttime;
	return 0;
}

static int proc_cmp(const void *a, const void *b)
{
	const struct aproc *a1 = a;
	const struct aproc *b1 = b;

	if (a1->time == b1->time)
		return a1->pid < b1->pid ? -1 : 1;
	return ((struct aproc *)a)->time < ((struct aproc *)b)->time ? -1 : 1;
}

int main(int argc, char *argv[])
{
	me = getpid();

	add_filter_re(NULL, "^[0-9]+$");
	walkfiles(NULL, "/proc", add_proc, WALK_ONE_DIR);

	qsort(procs, curproc, sizeof(struct aproc), proc_cmp);

	struct aproc *p;
	int i;
	for (p = procs, i = 0; i < curproc; ++i, ++p) {
		if (p->count > 1)
			printf("%5d %s (%d)\n", p->pid, p->cmd, p->count);
		else
			printf("%5d %s\n", p->pid, p->cmd);
	}

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -O2 -Wall myps.c -o myps -lsamlib"
 * End:
 */

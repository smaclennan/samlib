#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include "samlib.h"

static int readproc(pid_t pid, char *buf, int len)
{
	char fname[32];
	int fd, n;

	*buf = 0;

	snprintf(fname, sizeof(fname), "/proc/%u/cmdline", pid);
	fd = open(fname, O_RDONLY);
	if (fd < 0)
		return -1;
	n = read(fd, buf, len - 1);
	close(fd);
	if (n <= 0)
		return -1;

	buf[n] = 0;
	return n;
}

/* Note: /proc/pid/cmdline is limited to 4k */
int readproccmdline(pid_t pid, char *buf, int len)
{
	int i, n = readproc(pid, buf, len);

	if (n > 0)
		for (i = 0; i < n; ++i)
			if (buf[i] == 0)
				buf[i] = ' ';

	return n;
}

int readproccmd(pid_t pid, char *buf, int len)
{
	int n = readproc(pid, buf, len);
	if (n > 0)
		n = strlen(buf);
	return n;
}

pid_t _findpid(const char *cmd, pid_t start_pid, char *buf, int len)
{
	pid_t pid;
	char *e;
	struct dirent *ent;
	DIR *dir = opendir("/proc");
	if (!dir)
		return 0;

	while ((ent = readdir(dir))) {
		pid = strtol(ent->d_name, &e, 10);
		if (*e || pid <= start_pid)
			continue;
		readproccmd(pid, buf, len);
		if (strstr(buf, cmd)) {
			closedir(dir);
			return pid;
		}
	}

	closedir(dir);
	return 0;
}

pid_t findpid(const char *cmd, pid_t start_pid)
{
	char buf[4097];
	return _findpid(cmd, start_pid, buf, sizeof(buf));
}

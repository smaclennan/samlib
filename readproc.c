#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "samlib.h"

#ifdef __linux__
#include <dirent.h>

static int readproc(pid_t pid, const char *file, char *buf, int len)
{
	char fname[32];
	int fd, n;

	*buf = 0;

	strfmt(fname, sizeof(fname), "/proc/%u/%s", pid, file);
	fd = open(fname, O_RDONLY);
	if (fd < 0)
		return -1;
	n = read(fd, buf, len - 1);
	close(fd);

	/* Zero length is not an error. Some files, like a kernel thread
	 * cmdline, are zero length.
	 */
	if (n < 0)
		return -1;

	buf[n] = 0;
	return n;
}

/* Note: /proc/pid/cmdline is limited to 4k */
int readproccmdline(pid_t pid, char *buf, int len)
{
	int i, n = readproc(pid, "cmdline", buf, len);

	if (n > 0)
		for (i = 0; i < n - 1; ++i)
			if (buf[i] == 0)
				buf[i] = ' ';

	return n;
}

int readproccmd(pid_t pid, char *buf, int len)
{
	int n = readproc(pid, "cmdline", buf, len);
	if (n > 0)
		n = strlen(buf);
	return n;
}

int readprocstat(pid_t pid, struct procstat_min *stat)
{
	char buf[128];

	int n = readproc(pid, "stat", buf, sizeof(buf));
	if (n <= 0)
		return n;

	if (sscanf(buf, "%d %s %c %d %d %d %*d %*d %*u "
			   "%*u %*u %*u %*u %*u %*u %*u "
			   "%*u %*u %*u %*u %*u %llu",
			   &stat->pid, stat->comm, &stat->state,
			   &stat->ppid, &stat->pgrp, &stat->session,
			   &stat->starttime) != 7)
		return 1; /* can't happen */

	return 0;
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
		readproccmdline(pid, buf, len);
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
#elif defined(WIN32)
#include <psapi.h>

int readproccmd(pid_t pid, char *buf, int len)
{
	HMODULE hMod;
	DWORD cbNeeded;
	int rc = -1;
	HANDLE nProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
							   FALSE, pid);

	if (nProc)
		if (EnumProcessModules(nProc, &hMod, sizeof(hMod), &cbNeeded))
			rc = GetModuleBaseNameA(nProc, hMod, buf, len);

	CloseHandle(nProc);

	return rc;
}

pid_t _findpid(const char *cmd, pid_t start_pid, char *buf, int len)
{
	DWORD procs[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(procs, sizeof(procs), &cbNeeded))
		return 0;

	cProcesses = cbNeeded / sizeof(DWORD);
	for (i = 0; i < cProcesses; i++)
		if (procs[i] != 0 && procs[i] > start_pid)
			if (readproccmd(procs[i], buf, len) > 0)
				if (strstr(buf, cmd))
					return procs[i];

	return 0;
}

pid_t findpid(const char *cmd, pid_t start_pid)
{
	char buf[MAX_PATH];
	return _findpid(cmd, start_pid, buf, sizeof(buf));
}
#endif

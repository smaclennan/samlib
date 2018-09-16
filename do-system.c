#include <stdio.h>
#include <stdarg.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
#include "samlib.h"

/* Do a system() call with var args. The cmd is limited to 1k. Returns
 * either < 0 on error or the system status.
 */
int do_system(const char *fmt, ...)
{
	char cmd[1024];
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vsnprintf(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);
	
	if (n >= sizeof(cmd))
		return -EINVAL;

	int rc = system(cmd);
#ifndef WIN32
	if (WIFEXITED(rc))
		rc = WEXITSTATUS(rc);
#endif

	return rc;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include "samlib.h"

/* Do a system() call with var args. The cmd is limited to 1k. Returns
 * the system() status.
 */
int do_system(const char *fmt, ...)
{
	char cmd[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, ap);
	va_end(ap);

	int rc = system(cmd);
	if (WIFEXITED(rc))
		rc = WEXITSTATUS(rc);

	return rc;
}

#include <stdio.h>
#include "samlib.h"

void samlib_version(int *major, int *minor)
{
	if (major)
		*major = SAMLIB_MAJOR;
	if (minor)
		*minor = SAMLIB_MINOR;
}

const char *samlib_versionstr(void)
{
	static char version[8];
	snprintf(version, sizeof(version), "%d.%d", SAMLIB_MAJOR, SAMLIB_MINOR);
	return version;
}

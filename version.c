#include <stdio.h>
#include "samlib.h"

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */
#define __stringify_1(x)	#x
#define __stringify(x)	__stringify_1(x)

/* The marker is for `strings libsamlib.a | fgrep Version' */
const char *marker = "Version-" __stringify(SAMLIB_MAJOR) "." __stringify(SAMLIB_MINOR);

const char *samlib_version = __stringify(SAMLIB_MAJOR) "." __stringify(SAMLIB_MINOR);

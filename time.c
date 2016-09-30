#include <errno.h>

#include "samlib.h"


int timeval_delta_valid(struct timeval *start,
						struct timeval *end)
{
	if (end->tv_sec > start->tv_sec)
		return 1;
	if (end->tv_sec == start->tv_sec)
		if (end->tv_usec >= start->tv_usec)
			return 1;
	return 0; /* invalid */
}

/* Returns 0 if a valid delta was calculated, an errno if not.
 * Note that delta can point to start or end.
 */
int timeval_delta(struct timeval *start,
				  struct timeval *end,
				  struct timeval *delta)
{
	if (!start || !end || !delta || !timeval_delta_valid(start, end))
		return EINVAL;

	if (start->tv_usec > end->tv_usec) {
		delta->tv_sec = end->tv_sec - 1 - start->tv_sec;
		delta->tv_usec = end->tv_usec + 1000000 - start->tv_usec;
	} else {
		delta->tv_sec = end->tv_sec - start->tv_sec;
		delta->tv_usec = end->tv_usec - start->tv_usec;
	}

	return 0;
}

int timeval_delta2(struct timeval *t1,
				   struct timeval *t2,
				   struct timeval *delta)
{
	int rc = timeval_delta(t1, t2, delta);
	if (rc)
		rc = timeval_delta(t2, t1, delta);
	return rc;
}

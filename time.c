#ifndef WIN32
#define HAVE_TIMERSUB
#endif

#include <errno.h>
#include <math.h>
#ifdef HAVE_TIMERSUB
#include <sys/time.h>
#endif

#include "samlib.h"


/* If set, then get_month() assumes months are 1 based. Default is 0
 * based.
 */
int months_one_based;

#define ONE_SECOND 1000000

const char *get_month(unsigned month, int long_months)
{
	if (months_one_based)
		--month;
	if (month > 11)
		return "???";

	return long_months ? long_month[month] : short_month[month];
}

/* Can handle a difference of about 66 hours on 32 bit systems and
 * about 150 years on 64 bit systems. Optimized for delta < 1 second.
 */
unsigned long delta_timeval(const struct timeval *start,
							const struct timeval *end)
{
	if (start->tv_usec > end->tv_usec) {
		if (start->tv_sec == end->tv_sec - 1)
			return end->tv_usec + ONE_SECOND - start->tv_usec;
		if (start->tv_sec > end->tv_sec - 1)
			return ~0;
		return ((end->tv_sec - 1 - start->tv_sec) * ONE_SECOND) +
			(end->tv_usec + ONE_SECOND - start->tv_usec);
	}
	if (start->tv_sec == end->tv_sec)
		return end->tv_usec - start->tv_usec;
	if (start->tv_sec > end->tv_sec)
		return ~0;
	return ((end->tv_sec - start->tv_sec) * ONE_SECOND) +
		(end->tv_usec - start->tv_usec);
}

int timeval_delta_valid(const struct timeval *start, const struct timeval *end)
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
int timeval_delta(const struct timeval *start,
				  const struct timeval *end,
				  struct timeval *delta)
{
	if (!start || !end || !delta || !timeval_delta_valid(start, end))
		return EINVAL;

#ifdef HAVE_TIMERSUB
	timersub(end, start, delta);
#else
	if (start->tv_usec > end->tv_usec) {
		delta->tv_sec = end->tv_sec - 1 - start->tv_sec;
		delta->tv_usec = end->tv_usec + ONE_SECOND - start->tv_usec;
	} else {
		delta->tv_sec = end->tv_sec - start->tv_sec;
		delta->tv_usec = end->tv_usec - start->tv_usec;
	}
#endif

	return 0;
}

double timeval_delta_d(const struct timeval *start,
					   const struct timeval *end)
{
	struct timeval delta;

	if (timeval_delta(start, end, &delta))
		return (double)INFINITY; /* windows needs the cast */

	return (double)delta.tv_usec / 1000000 + (double)delta.tv_sec;
}

int timeval_delta2(const struct timeval *t1,
				   const struct timeval *t2,
				   struct timeval *delta)
{
	int rc = timeval_delta(t1, t2, delta);
	if (rc)
		rc = timeval_delta(t2, t1, delta);
	return rc;
}

unsigned mbs(unsigned long bytes, unsigned long useconds)
{
	double mb = (double)bytes / 1048576.0;
	double sec = (double)useconds / 1000000.0;
	return (unsigned)(mb / sec + 0.5);
}

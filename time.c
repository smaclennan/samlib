#include <errno.h>

#include "samlib.h"


const char *short_month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *long_month[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};

/* If set, then get_month() assumes months are 1 based. Default is 0
 * based.
 */
int months_one_based;

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
ulong delta_timeval(struct timeval *start, struct timeval *end)
{
	if (start->tv_usec > end->tv_usec) {
		--end->tv_sec;
		end->tv_usec += 1000000;
	}
	if (start->tv_sec == end->tv_sec)
		return end->tv_usec - start->tv_usec;
	if (start->tv_sec > end->tv_sec)
		return ~0;
	return ((end->tv_sec - start->tv_sec) * 1000000) +
		(end->tv_usec - start->tv_usec);
}

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

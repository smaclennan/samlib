#include <stdio.h>
#include "samlib.h"

/* Returns duration in seconds. Supports fractional values. */
unsigned long get_duration(const char *str)
{
	char *e;
	double t = strtod(str, &e);

	switch (*e) {
	case 'd':
	case 'D':
		t *= 24.0;
		/* fall thru */
	case 'h':
	case 'H':
		t *= 60.0;
		/* fall thru */
	case 'm':
	case 'M':
		t *= 60.0;
		/* fall thru */
	case 's':
	case 'S':
	case 0:
		return (unsigned long)t;
	default:
		fprintf(stderr, "Invalid size %s\n", e);
		exit(1);
	}
}

char *nice_duration(unsigned long duration, char *str, int len)
{
	static const char fmt[3] = { 'd', 'h', 'm' };
	static const unsigned long check[3] = { ONE_DAY, ONE_HOUR, ONE_MINUTE };
	int d, i, n = 0;

	if (str == NULL) {
		if (len == 0)
			len = 32;
		str = malloc(len);
		if (!str)
			return NULL;
	}

	for (i = 0; i < 3; ++i)
		if (duration >= check[i]) {
			d = duration / check[i];
			duration -= d * check[i];
			n += strfmt(str + n, len - n, "%d%c", d, fmt[i]);
		}
	if (duration || n == 0)
		n += strfmt(str + n, len - n, "%lus", duration);

	return str;
}

/* Returns memory length in bytes. */
uint64_t get_mem_len(const char *str)
{
	char *e;
	uint64_t len = strtoul(str, &e, 0);

	switch (*e) {
	case 't':
	case 'T':
		len *= 1024;
		/* fall thru */
	case 'g':
	case 'G':
		len *= 1024;
		/* fall thru */
	case 'm':
	case 'M':
		len *= 1024;
		/* fall thru */
	case 'k':
	case 'K':
		len *= 1024;
		/* fall thru */
	case 'b':
	case 'B':
	case 0:
		return len;
	default:
		fprintf(stderr, "Invalid size %s\n", e);
		exit(1);
	}
}

unsigned nice_mem_len(uint64_t size, char *ch)
{
	if (size) {
		if ((size & ((1ULL << 40) - 1)) == 0) {
			*ch = 'T';
			return size >> 40;
		} else if ((size & ((1UL << 30) - 1)) == 0) {
			*ch = 'G';
			return size >> 30;
		} else if ((size & ((1UL << 20) - 1)) == 0) {
			*ch = 'M';
			return size >> 20;
		} else if ((size & ((1UL << 10) - 1)) == 0) {
			*ch = 'K';
			return size >> 10;
		}
	}

	*ch = ' ';
	return size;
}

char *nicer_mem_len(uint64_t size, char *str, int len)
{
	if (size >= (1ul << 30)) {
		if ((size & ((1UL << 30) - 1)) == 0)
			snprintf(str, len, "%luG", size >> 30);
		else
			snprintf(str, len, "%.1fG", (double)size / (double)(1ul << 30));
	} else if (size >= (1ul << 20)) {
		if ((size & ((1UL << 20) - 1)) == 0)
			snprintf(str, len, "%luM", size >> 20);
		else
			snprintf(str, len, "%.1fM", (double)size / (double)(1ul << 20));
	} else if (size >= (1ul << 10)) {
		if ((size & ((1UL << 10) - 1)) == 0)
			snprintf(str, len, "%luK", size >> 10);
		else
			snprintf(str, len, "%.1fK", (double)size / (double)(1ul << 10));
	} else
		snprintf(str, len, "%lu", size);
	return str;
}

/* Adds commas. Not thread safe */
char *nice_number(long number)
{   /* largest 64 bit decimal, with commas, is 26 + 1 */
	static char str[32], *p;
	int i, neg;

	if (number == 0)
		return strcpy(str, "0");

	neg = number < 0;
	if (neg)
		number = -number;

	p = str + 31;
	*p = 0;

	for (i = 0; number; i++) {
		if ((i & 3) == 3) {
			*(--p) = ',';
			++i;
		}
		*(--p) = (number % 10) + '0';
		number /= 10;
	}

	if (neg)
		*(--p) = '-';

	return p;
}

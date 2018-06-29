#include "samlib.h"
#include <string.h>
#include <stdarg.h>


static inline int __copy(char *dst, const char *src, int dstsize)
{
	int i;

	if (dstsize <= 0) return 0;

	--dstsize;
	for (i = 0; i < dstsize && *src; ++i)
		*dst++ = *src++;
	*dst = 0;

	return i;
}

int safecpy(char *dst, const char *src, int dstsize)
{
	return __copy(dst, src, dstsize);
}

int safecat(char *dst, const char *src, int dstsize)
{
	if (dstsize <= 0) return 0;

	int len = strlen(dst);
	dst += len;
	dstsize -= len;

	return __copy(dst, src, dstsize);
}

int safe_snprintf(char *dst, int dstsize, const char *fmt, ...)
{
	if (dstsize <= 0) return 0;

	va_list ap;
	va_start(ap, fmt);
	int n = vsnprintf(dst, dstsize, fmt, ap);
	va_end(ap);

	if (n > dstsize)
		n = dstsize;

	return n;
}

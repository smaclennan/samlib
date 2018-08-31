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

#if defined(__linux__) || defined(WIN32)
size_t strlcpy(char *dst, const char *src, size_t dstsize)
{
	size_t srcsize = strlen(src);

	if (dstsize) {
		if (dstsize > srcsize)
			strcpy(dst, src);
		else {
			--dstsize;
			strncpy(dst, src, dstsize);
			dst[dstsize] = 0;
		}
	}

	return srcsize;
}

/* A simple strlcat implementation for Linux */
size_t strlcat(char *dst, const char *src, size_t dstsize)
{
	size_t i = 0;

	while (dstsize > 0 && *dst) {
		--dstsize;
		++dst;
		++i;
	}

	if (dstsize > 0) {
		--dstsize;
		while (*src && i < dstsize) {
			*dst++ = *src++;
			++i;
		}
		*dst = 0;
	}

	/* strlcat returns the size of the src + initial dst */
	while (*src++) ++i;

	return i;
}
#endif

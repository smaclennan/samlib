#include "samlib.h"
#include <string.h>
#include <stdarg.h>


static inline int __copy(char *dst, const char *src, int dstsize)
{
	int i = 0;

	if (dstsize > 0) {
		--dstsize;
		while (i < dstsize && *src) {
			*dst++ = *src++;
			++i;
		}
		*dst = 0;
	}

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
	__copy(dst, src, dstsize);
	return strlen(src);
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

	__copy(dst, src, dstsize);

	/* strlcat returns the size of the src + initial dst */
	return i + strlen(src);
}
#endif

/* Concatenates any number of string. The last string must be NULL.
 * Returns length actually copied
 */
int strconcat(char *str, int len, ...)
{
	char *arg;
	int total = 0;

	va_list ap;
	va_start(ap, len);
	while ((arg = va_arg(ap, char *)) && len > 0) {
		int n = __copy(str, arg, len);
		str += n;
		len -= n;
		total += n;
	}
	va_end(ap);

	return total;
}

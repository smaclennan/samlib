#include "samlib.h"
#include <string.h>
#include <stdarg.h>


#if defined(__linux__) || defined(WIN32)
/* Returns the size of src */
size_t strlcpy(char *dst, const char *src, size_t dstlen)
{
	int srclen = strlen(src);

	if (dstlen > 0) {
		if (dstlen > srclen)
			strcpy(dst, src);
		else {
			strncpy(dst, src, dstlen - 1);
			dst[dstlen - 1] = 0;
		}
	}

	return srclen;
}

/* Returns the size of dst + size of src */
size_t strlcat(char *dst, const char *src, size_t dstsize)
{
	int dstlen = strlen(dst);
	int srclen = strlen(src);
	int left   = dstsize - dstlen;

	if (left > 0) {
		if (left > srclen)
			strcpy(dst + dstlen, src);
		else {
			strncpy(dst + dstlen, src, left - 1);
			dst[dstsize - 1] = 0;
		}
	}

	return dstlen + srclen;
}
#endif

/* Concatenates any number of strings. The last string must be NULL.
 * Returns length actually copied
 */
int strconcat(char *str, int len, ...)
{
	char *arg;
	int total = 0;

	if (len == 0)
		return 0;

	va_list ap;
	va_start(ap, len);
	while ((arg = va_arg(ap, char *)) && len > 0) {
		int n = strlcpy(str, arg, len);
		str += n;
		len -= n;
		total += n;
	}
	va_end(ap);

	if (len <= 0)
		total += len - 1;

	return total;
}

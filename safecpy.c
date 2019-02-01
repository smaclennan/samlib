#include "samlib.h"
#include <string.h>


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

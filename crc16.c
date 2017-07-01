#include "samlib.h"

/* RFC 1071. */
uint16_t chksum16(const void *buf, int count)
{
	int32_t sum = 0, shift;
	const uint16_t *p = buf;

	while (count > 1) {
		sum += *p++;
		count -= 2;
	}

	if (count > 0)
		sum += *p;

	/*  Fold 32-bit sum to 16 bits */
	while ((shift = sum >> 16))
		sum = (sum & 0xffff) + shift;

	return ~sum;
}

#include "samlib.h"

/* This checksum is meant for small buffers. Set the LARGE_BUFFERS
 * define if you want to deal with buffers greater than 64k. With
 * pedantic off, you need 64k + 1 of 0xFF bytes to overflow.
#define LARGE_BUFFERS
 */

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
	if ((shift = sum >> 16)) {
		sum = (sum & 0xffff) + shift;
#ifdef LARGE_BUFFERS
		if ((shift = sum >> 16))
			sum = (sum & 0xffff) + shift;
#endif
	}

	return ~sum;
}

#include "samlib.h"

/* RFC 1071 */
uint16_t crc16(const void *buf, int count)
{
	int32_t sum = 0, shift;

	while (count > 1) {
		sum += *(unsigned short *)buf++;
		count -= 2;
	}

	if (count > 0)
		sum += *(unsigned char *)buf;

	/*  Fold 32-bit sum to 16 bits */
	while ((shift = sum >> 16))
		sum = (sum & 0xffff) + shift;

	return ~sum;
}

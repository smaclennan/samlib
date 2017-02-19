#include "samlib.h"

/* RFC 1071. Warning, while this is known as crc16, it is actually a
 * checksum and not a proper CRC.
 */
uint16_t crc16(const void *buf, int count)
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

#ifdef STANDALONE
#include <stdio.h>

static struct ref {
	uint16_t msg[20];
	int len;
	uint16_t crc;
} refs[] = {
	{ { 0x4500, 0x003c, 0x1c46, 0x4000, 0x4006,
		0x0000, 0xac10, 0x0a63, 0xac10, 0x0a0c },
	  20, 0xb1e6 },
	{ { 0x4500, 0x003c, 0x1c46, 0x4000, 0x4006,
		0xb1e6, 0xac10, 0x0a63, 0xac10, 0x0a0c },
	  20, 0 },

	{ { 0x4500, 0x0514, 0x42a2, 0x2140, 0x8001,
		0x0000, 0xc0a8, 0x0003, 0xc0a8, 0x0001 },
	  20, 0x50b2 },
	{ { 0x4500, 0x0514, 0x42a2, 0x2140, 0x8001,
		0x50b2, 0xc0a8, 0x0003, 0xc0a8, 0x0001 },
	  20, 0 },

	{ { 0x4500, 0x003c, 0xeb3a, 0x4000, 0x4006,
		0x0000, 0xc0a8, 0x0195, 0xc0a8, 0x0065 },
	  20, 0xcc36 },
	{ { 0x4500, 0x003c, 0xeb3a, 0x4000, 0x4006,
		0xcc36, 0xc0a8, 0x0195, 0xc0a8, 0x0065 },
	  20, 0 },
};
#define N_REFS (sizeof(refs) / sizeof(struct ref))

int main(int argc, char *argv[])
{
	int i, rc = 0;
	uint16_t crc;

	for (i = 0; i < N_REFS; ++i) {
		crc = crc16(refs[i].msg, refs[i].len);
		if (crc != refs[i].crc) {
			printf("PROBLEMS: %d: %x != %x\n", i, crc, refs[i].crc);
			rc = 1;
		}
	}

	return rc;
}
#endif

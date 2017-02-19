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

#ifdef STANDALONE
#include <stdio.h>

static struct ref {
	uint16_t msg[20];
	uint16_t chksum;
} refs[] = {
	{ { 0x4500, 0x003c, 0x1c46, 0x4000, 0x4006,
		0x0000, 0xac10, 0x0a63, 0xac10, 0x0a0c },
	  0xb1e6 },
	{ { 0x4500, 0x003c, 0x1c46, 0x4000, 0x4006,
		0xb1e6, 0xac10, 0x0a63, 0xac10, 0x0a0c },
	  0 },

	{ { 0x4500, 0x0514, 0x42a2, 0x2140, 0x8001,
		0x0000, 0xc0a8, 0x0003, 0xc0a8, 0x0001 },
	  0x50b2 },
	{ { 0x4500, 0x0514, 0x42a2, 0x2140, 0x8001,
		0x50b2, 0xc0a8, 0x0003, 0xc0a8, 0x0001 },
	  0 },

	{ { 0x4500, 0x003c, 0xeb3a, 0x4000, 0x4006,
		0x0000, 0xc0a8, 0x0195, 0xc0a8, 0x0065 },
	  0xcc36 },
	{ { 0x4500, 0x003c, 0xeb3a, 0x4000, 0x4006,
		0xcc36, 0xc0a8, 0x0195, 0xc0a8, 0x0065 },
	  0 },
};
#define N_REFS (sizeof(refs) / sizeof(struct ref))

int main(int argc, char *argv[])
{
	int i, rc = 0;
	uint16_t chksum;

	for (i = 0; i < N_REFS; ++i) {
		chksum = chksum16(refs[i].msg, 20);
		if (chksum != refs[i].chksum) {
			printf("PROBLEMS: %d: %x != %x\n", i, chksum, refs[i].chksum);
			rc = 1;
		}
	}

	return rc;
}
#endif

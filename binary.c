#include <stdio.h>
#include <ctype.h>
#include "samlib.h"

static inline void bin_char(uint8_t c)
{
	if (isprint(c))
		putchar(c);
	else
		putchar('.');
}

void binary_dump(const uint8_t *buf, int len)
{
	int i, format, addr = 0;

	format = len <= 0x10000 ? 4 : 8;

	while (len > 16) {
		printf("%*x: ", format, addr);
		for (i = 0; i < 16; ++i)
			printf("%02x ", buf[i]);
		putchar(' ');
		for (i = 0; i < 16; ++i)
			bin_char(buf[i]);
		putchar('\n');

		buf += 16;
		len -= 16;
		addr += 16;
	}

	if (len == 0)
		return; /* yeah! */

	printf("%*x: ", format, addr);
	for (i = 0; i < len; ++i)
		printf("%02x ", buf[i]);

	for( ; i < 16; ++i)
		fputs("   ", stdout);
	putchar(' ');
	for (i = 0; i < len; ++i)
			bin_char(buf[i]);
	putchar('\n');
}

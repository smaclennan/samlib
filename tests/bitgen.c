#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <samlib.h>

/* For diehard: bitgen -b10000000 > test.bits
 * For dieharder: bitgen | dieharder -g 200 -a
 */

static void usage(void)
{
	fputs("bitgen [-h] [-b bits]\n"
		  "where:\t-h\tis this help\n"
		  "\t-b bits\tgenerates at least the specified bits (rounded up to 64).\n"
		  "Normal case is to produce bits until EOF.\n", stderr);
}

int main(int argc, char *argv[])
{
	uint64_t r;
	int n;
	long len = 0;

	while ((n = getopt(argc, argv, "hb:")) != EOF)
		switch (n) {
		case 'h': usage(); exit(0);
		case 'b': len = strtol(optarg, NULL, 0); break;
		default: puts("Sorry!"); exit(1);
		}

	if (len)
		while (len > 0) {
			r = xorshift128plus();
			n = write(1, &r, sizeof(r));
			if (n != sizeof(r)) {
				puts("write error");
				exit(1);
			}
			len -= sizeof(r) * 8;
		}
	else
		/* write bits until EOF */
		do {
			r = xorshift128plus();
			n = write(1, &r, sizeof(r));
		} while (n == sizeof(r));

	return 0;
}

/* ipaddr.c - get various IP information
 * Copyright (C) 2012-2017 Sean MacLennan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this project; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <errno.h>
#include "../samlib.h"

#ifndef WIN32
#include <arpa/inet.h> /* inet_ntoa */
#endif

#define W_ADDRESS  (1 << 0)
#define W_MASK     (1 << 1)
#define W_SUBNET   (1 << 2)
#define W_BITS     (1 << 3)
#define W_GATEWAY  (1 << 4)
#define W_GUESSED  (1 << 5)
#define W_ALL	   (1 << 6)

#define MAX_INTERFACES 16 /* arbitrary and huge */

/* This is so fast, it is not worth optimizing. */
static int maskcnt(unsigned mask)
{
	unsigned count = 32;

	mask = ntohl(mask);
	while ((mask & 1) == 0) {
		mask >>= 1;
		--count;
	}

	return count;
}

static int check_one(const char *ifname, unsigned what)
{
	int n = 0;
	struct in_addr addr, mask, gw, *gw_ptr = NULL;

	if (what & W_GATEWAY)
		/* gateway is more expensive */
		gw_ptr = &gw;

	if (ip_addr(ifname, &addr, &mask, gw_ptr)) {
		if (errno == EADDRNOTAVAIL) {
			if (what & W_ALL) {
				/* not an error, they asked for down interfaces */
				printf("down (%s)\n", ifname);
				return 0;
			} else
				fprintf(stderr, "%s: No address\n", ifname);
		} else
			perror(ifname);
		return 1;
	}

	if (what & W_ADDRESS) {
		++n;
		printf("%s", inet_ntoa(addr));
		if (what & W_BITS)
			printf("/%d", maskcnt(mask.s_addr));
	}
	if (what & W_SUBNET) {
		addr.s_addr &= mask.s_addr;
		if (n++)
			putchar(' ');
		printf("%s", inet_ntoa(addr));
		if (what & W_BITS)
			printf("/%d", maskcnt(mask.s_addr));
	}
	if (what & W_MASK) {
		if (n++)
			putchar(' ');
		printf("%s", inet_ntoa(mask));
	}

	if (what & W_GATEWAY) {
		if (n++)
			putchar(' ');
		printf("%s", inet_ntoa(gw));
	}

	if (n) {
		if (what & W_GUESSED)
			printf(" (%s)", ifname);
		putchar('\n');
	}

	return 0;
}

static void usage(int rc)
{
	fputs("usage: ipaddr [-abgims] [interface]\n"
		  "where: -i displays IP address (default)\n"
		  "       -g displays gateway\n"
		  "       -m displays network mask\n"
		  "       -s displays subnet\n"
		  "       -b add bits as /bits to -a and/or -s\n"
		  "       -a displays all interfaces (even down)\n"
		  "Interface defaults to eth0.\n"
		  "Designed to be easily used in scripts. All error output to stderr.\n",
		  stderr);

	exit(rc);
}

int main(int argc, char *argv[])
{
	int c, rc = 0;
	unsigned what = 0;

	while ((c = getopt(argc, argv, "abgmsh")) != EOF)
		switch (c) {
		case 'i':
			what |= W_ADDRESS;
			break;
		case 'b':
			what |= W_BITS;
			break;
		case 'g':
			what |= W_GATEWAY;
			break;
		case 'm':
			what |= W_MASK;
			break;
		case 's':
			what |= W_SUBNET;
			break;
		case 'a':
			what |= W_ALL;
			break;
		case 'h':
			usage(0);
		default:
			exit(2);
		}

	if ((what & ~(W_BITS | W_ALL)) == 0)
		what |= W_ADDRESS;

	if (optind < argc) {
		while (optind < argc)
			rc |= check_one(argv[optind++], what);
	} else {
		char *ifaces[MAX_INTERFACES];
		uint64_t state;
		int i, n;

		n = get_interfaces(ifaces, MAX_INTERFACES, &state);
		if (n == 0) {
			fputs("No interfaces found\n", stderr);
			return 1;
		}

		for (i = 0; i < n; ++i, state >>= 1)
			if ((what & W_ALL) || (state & 1))
				rc |= check_one(ifaces[i], what | W_GUESSED);
	}

	return rc;
}

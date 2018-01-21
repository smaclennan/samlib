/* meminfo.c - simple /proc/meminfo output
 * Copyright (C) 2015 Sean MacLennan
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this project; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This is a nice simple output of /proc/meminfo. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>


#define U "%u %u %u %u %c\n"
#define FULL_U "Total: %8u   Free: %8u   Available: %8u  Swap: %8u  (%c)\n"

static unsigned get_value(const char *line)
{
	char *p;
	unsigned val = strtol(line, &p, 10);
	if (strncmp(p, " kB", 3)) {
		printf("Invalid line '%s'\n", line);
		exit(1);
	}
	return val;
}

/* n is in K */
#define MEG(n) (((n) + 512) / 1024)
#define GIG(n) (((double)(n)) / 1024.0 / 1024.0)
#define uGIG(n) ((MEG(n) + 512) / 1024)

static void usage(const char *prog, int rc)
{
	printf("usage: %s [-kmgps]\n"
		   "where:  -k output in kilobytes\n"
		   "\t-m output in megabytes\n"
		   "\t-g output in gigabytes (default)\n"
		   "\t-p output in percent\n"
		   "\t-s script friendly output\n"
		   "\nIt is valid to specify more than one output arg.\n"
		   "\nNote:   The -g option displays the output as a floating point number.\n"
		   "\tFor an integer specify -gg.\n", prog);
	exit(rc);
}

int main(int argc, char *argv[])
{
	unsigned total = 0, free = 0, avail = 0, buffers = 0, cached = 0, swapt = 0, swapf = 0;
	int c, do_k = 0, do_m = 0, do_g = 0, do_script = 0, do_percent = 0;

	while ((c = getopt(argc, argv, "hkmgps")) != EOF)
		switch (c) {
		case 'h': usage(argv[0], 0);
		case 'k': do_k = 1; break;
		case 'm': do_m = 1; break;
		case 'g': ++do_g; break;
		case 'p': do_percent = 1; break;
		case 's': do_script = 1; break;
		default: usage(argv[0], 1);
		}

	FILE *fp = fopen("/proc/meminfo", "r");
	if (!fp) {
		perror("/proc/meminfo");
		exit(1);
	}

	char line[80];
	while (fgets(line, sizeof(line), fp)) {
		if (strncmp(line, "MemTotal:", 9) == 0)
			total = get_value(line + 9);
		else if (strncmp(line, "MemFree:", 8) == 0)
			free = get_value(line + 8);
		else if (strncmp(line, "MemAvailable:", 13) == 0)
			avail = get_value(line + 13);
		else if (strncmp(line, "Buffers:", 8) == 0)
			buffers = get_value(line + 8);
		else if (strncmp(line, "Cached:", 7) == 0)
			cached = get_value(line + 7);
		else if (strncmp(line, "SwapTotal:", 10) == 0)
			swapt = get_value(line + 10);
		else if (strncmp(line, "SwapFree:", 9) == 0)
			swapf = get_value(line + 9);
	}

	fclose(fp);

	/* Older kernels do not have available */
	if (avail == 0)
		avail = free + buffers + cached;

	swapt -= swapf;

	if (do_k == 0 && do_m == 0 && do_percent == 0) do_g = 1; /* default to g */

	if (do_script) {
		if (do_k)
			printf(U, total, free, avail, swapt, 'K');
		if (do_m)
			printf(U, MEG(total), MEG(free), MEG(avail), MEG(swapt), 'M');
		if (do_g > 1)
			printf(U, uGIG(total), uGIG(free), uGIG(avail), uGIG(swapt), 'G');
		else if (do_g)
			printf("%.2f %.2f %.2f %.2f G\n", GIG(total), GIG(free), GIG(avail), GIG(swapt));
		if (do_percent)
			printf("%.0f %.0f %%\n",
				   (double)free * 100.0 / (double)total,
				   (double)avail * 100.0 / (double)total);
	} else {
		if (do_k)
			printf(FULL_U, total, free, avail, swapt, 'K');
		if (do_m)
			printf(FULL_U, MEG(total), MEG(free), MEG(avail), MEG(swapt), 'M');
		if (do_g > 1)
			printf(FULL_U, uGIG(total), uGIG(free), uGIG(avail), uGIG(swapt), 'G');
		else if (do_g)
			printf("Total: %8.2f   Free: %8.2f   Available: %8.2f  Swap: %8.2f  (G)\n",
				   GIG(total), GIG(free), GIG(avail), GIG(swapt));
		if (do_percent)
			printf("Free: %.0f%%  Available: %.0f%%\n",
				   (double)free * 100.0 / (double)total,
				   (double)avail * 100.0 / (double)total);
	}

	return 0;
}

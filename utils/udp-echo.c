/*
 * udp-echo - a udp echo server
 * Copyright (C) 2007 Sean MacLennan
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 9898


static int verbose;


struct port_list {
	int start, end;
	int inc;
	struct port_list *next;
};

struct port_list default_port = { PORT, PORT, 1, NULL };

struct port_list *ports;

static void get_ports(char *optarg);
static struct pollfd *create_pollfds(int *n_ufds);

static void usage(char *prog, int rc)
{
	char *p;

	p = strrchr(prog, '/');
	if (p)
		prog = p + 1;
	printf("usage: %s [-v] [-p port]\n", prog);
	exit(rc);
}


void echo(int sock)
{
	struct sockaddr_in from;
	struct sockaddr *addr = (struct sockaddr *)&from;
	unsigned fromlen = sizeof(from);
	int sent, n;
	char packet[8096];

	n = recvfrom(sock, packet, sizeof(packet), 0, addr, &fromlen);
	if (n <= 0) {
		perror("recvfrom");
		return;
	}

	if (verbose)
		printf("Got packet from %s:%d\n",
		       inet_ntoa(from.sin_addr), ntohs(from.sin_port));

	sent = sendto(sock, packet, n, 0, addr, fromlen);
	if (sent != n)
		printf("Short send %d/%d\n", sent, n);
}


int main(int argc, char *argv[])
{
	int c, i, n;
	struct pollfd *ufds = NULL;
	int n_ufds = 0;

	while ((c = getopt(argc, argv, "hp:v")) != EOF)
		switch (c) {
		case 'h':
			usage(argv[0], 0);
		case 'p':
			get_ports(optarg);
			break;
		case 'v':
			++verbose;
			break;
		default:
			usage(argv[0], 1);
		}

	ufds = create_pollfds(&n_ufds);

	while ((n = poll(ufds, n_ufds, -1)) > 0)
		for (i = 0; i < n_ufds && n; ++i)
			if (ufds[i].revents) {
				echo(ufds[i].fd);
				--n;
			}

	perror("poll");
	return 1;
}


struct pollfd *create_pollfds(int *n_ufds)
{
	struct port_list *port;
	struct pollfd *ufds = NULL;
	int n = 0;

	if (ports == NULL)
		ports = &default_port;

	for (port = ports; port; port = port->next) {
		while (port->start <= port->end) {
			int sock, rc;
			struct sockaddr_in from;

			sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (sock == -1) {
				perror("socket");
				exit(1);
			}

			memset(&from, 0, sizeof(from));
			from.sin_family = AF_INET;
			from.sin_addr.s_addr = INADDR_ANY;
			from.sin_port = htons(port->start);

			rc = bind(sock, (struct sockaddr *)&from, sizeof(from));
			if (rc == -1) {
				perror("bind");
				exit(1);
			}


			ufds = realloc(ufds, sizeof(struct pollfd) * (n + 1));
			if (ufds == NULL) {
				printf("Realloc failed\n");
				exit(1);
			}

			ufds[n].fd = sock;
			ufds[n].events = POLLIN;
			++n;

			if (verbose)
				printf("Server listening on %d\n", port->start);

			port->start += port->inc;
		}
	}

	if (n_ufds)
		*n_ufds = n;

	return ufds;
}


/* n[-m][/i] */
void get_ports(char *optarg)
{
	static struct port_list *tail;
	struct port_list *port;
	char *arg = optarg;
	int start, end, inc = 1;

	start = end = strtol(arg, &arg, 0);
	if (*arg == '-')
		end = strtol(arg + 1, &arg, 0);
	if (*arg == '/')
		inc = strtol(arg + 1, &arg, 0);
	if (*arg != '\0') {
		printf("Unexpected char %c in '%s'\n", *arg, optarg);
		exit(1);
	}
	if (start == 0 || end == 0 || inc == 0) {
		printf("Ranges cannot be zero\n");
		exit(1);
	}

	port = calloc(sizeof(struct port_list), 1);
	if (port == NULL) {
		printf("Out of memory\n");
		exit(1);
	}

	if (start <= end) {
		port->start = start;
		port->end = end;
	} else {
		port->start = end;
		port->end = start;
	}
	port->inc = inc;

	if (ports)
		tail->next = port;
	else
		ports = port;
	tail = port;
}

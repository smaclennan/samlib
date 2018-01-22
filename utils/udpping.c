/*
 * udpping - ping over udp
 * Copyright (C) 2005-2009 Sean MacLennan
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
#include <sys/signal.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 9898

static int server_side(unsigned host, int port);
static int client_side(unsigned host, int port);
static void done(int sig);
#ifndef __FreeBSD__
static int get_ifindex(const char *ifname, int fd);
#endif
static int proto_udp_writev(int sock, struct iovec *iov, size_t iovlen,
			    struct sockaddr *to, int ifindex);

struct ping_pkt {
	unsigned magic;
#define MAGIC 0xea0102ea
	unsigned seqno;
	struct timeval t1, t2, t3;
};


static int verbose;
static int quiet;


void usage(char *prog)
{
	printf("usage: %s [-sv] [-p port] [host[:port]]\n"
		   "       host required on client\n"
		   , prog);
	exit(2);
}

static int loops;
static int timeout = 1000;
static int ifindex;

/* Stats */
static int pkt_sent;
static int pkt_read;


int main(int argc, char *argv[])
{
	int server;
	char *p, *host = NULL;
	int port = PORT, c;
	in_addr_t hostip;

	p = strrchr(argv[0], '/');
	if (p)
		++p;
	else
		p = argv[0];
	server = strcmp(p, "udppingd") == 0;

	while ((c = getopt(argc, argv, "i:n:p:qst:v")) != EOF)
		switch (c) {
		case 'i':
#ifndef __FreeBSD__
			ifindex = get_ifindex(optarg, -1);
			if (ifindex == 0) {
				printf("Unable to get index for %s\n", optarg);
				exit(1);
			}
#else
			puts("-i not supported in FreeBSD");
#endif
			break;
		case 'n':
			loops = strtol(optarg, 0, 0);
			break;
		case 'p':
			port = strtol(optarg, 0, 0);
			break;
		case 'q':
			quiet = 1;
			break;
		case 's':
			server = 1;
			break;
		case 't':
			timeout = strtol(optarg, 0, 0);
			break;
		case 'v':
			++verbose;
			break;
		default:
			usage(argv[0]);
		}

	if (optind < argc)
		host = argv[optind];

	if (host) {
		struct hostent *h;

		p = strchr(host, ':');
		if (p) {
			*p++ = '\0';
			port = strtol(p, 0, 0);
		}
		h = gethostbyname(host);
		if (h == NULL) {
			perror(host);
			exit(1);
		}
		hostip = *(in_addr_t *)h->h_addr;
	} else if (server)
		hostip = INADDR_ANY;
	else
		usage(argv[0]);

	if (timeout < 100) {
		printf("Minimum timeout 100ms\n");
		timeout = 100;
	}

	if (server)
		return server_side(hostip, port);
	else
		return client_side(hostip, port);
}


static int server_side(in_addr_t host, int port)
{
	struct sockaddr_in from;
	struct sockaddr *addr;
	struct ping_pkt *pkt;
	int sock, n;
	char packet[128];

	memset(&from, 0, sizeof(from));
	from.sin_family = AF_INET;
	from.sin_addr.s_addr = host;
	from.sin_port = htons(port);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		perror("listen socket");
		return 1;
	}

	if (bind(sock, (struct sockaddr *)&from, sizeof(from)) == -1) {
		perror("bind");
		close(sock);
		return 1;
	}

	if (verbose)
		printf("Server listening on %d\n", PORT);

	pkt = (struct ping_pkt *)packet;
	addr = (struct sockaddr *)&from;

	while (1) {
		unsigned fromlen = sizeof(from);
		int sent;

		n = recvfrom(sock, packet, sizeof(packet), 0, addr, &fromlen);
		if (n <= 0) {
			perror("recvfrom");
			continue;
		}

		if (verbose > 1)
			printf("Got a request from %s:%d\n",
			       inet_ntoa(from.sin_addr), ntohs(from.sin_port));

		if (n < sizeof(struct ping_pkt)) {
			printf("Expected %ld, got %d\n",
			       sizeof(struct ping_pkt), n);
			continue;
		} else if (verbose && n != sizeof(struct ping_pkt))
			printf("Expected %ld, got %d\n",
			       sizeof(struct ping_pkt), n);

		if (pkt->magic != MAGIC) {
			printf("Bad magic %x (%x)\n", pkt->magic, MAGIC);
			continue;
		}

		gettimeofday(&pkt->t2, 0);

		sent = sendto(sock, packet, n, 0, addr, fromlen);
		if (sent != n)
			printf("Short send %d/%d\n", sent, n);
	}
}


int send_ping(int sock, struct sockaddr *to)
{
	static int seqno;
	struct ping_pkt pkt;
	struct iovec iov;
	int n;

	memset(&pkt, 0, sizeof(pkt));
	pkt.magic = MAGIC;
	pkt.seqno = ++seqno;
	gettimeofday(&pkt.t1, 0);

	iov.iov_base = &pkt;
	iov.iov_len = sizeof(pkt);

	n = proto_udp_writev(sock, &iov, 1, to, ifindex);
	if (n != sizeof(pkt)) {
		printf("Short send %d/%ld\n", n, sizeof(pkt));
		return 1;
	}

	++pkt_sent;

	return 0;
}


int get_pong(int sock)
{
	struct sockaddr_in from;
	struct sockaddr *addr;
	struct ping_pkt *pkt;
	int n;
	char packet[128];
	struct timeval rtt;

	pkt = (struct ping_pkt *)packet;
	addr = (struct sockaddr *)&from;
	n = sizeof(struct ping_pkt);

	unsigned fromlen = sizeof(from);
	n = recvfrom(sock, packet, sizeof(packet), 0, addr, &fromlen);
	gettimeofday(&pkt->t3, 0);

	if (n <= 0) {
		perror("recvfrom");
		return 1;
	}

	if (n < sizeof(struct ping_pkt)) {
		printf("Expected %ld, got %d\n", sizeof(struct ping_pkt), n);
		return 1;
	}

	if (pkt->magic != MAGIC) {
		printf("Bad magic %x (%x)\n", pkt->magic, MAGIC);
		return 1;
	}

	++pkt_read;

	if (quiet)
		return 0;

	if (pkt->t1.tv_usec > pkt->t3.tv_usec) {
		rtt.tv_sec = pkt->t3.tv_sec - 1 - pkt->t1.tv_sec;
		rtt.tv_usec = pkt->t3.tv_usec + 1000000 - pkt->t1.tv_usec;
	} else {
		rtt.tv_sec = pkt->t3.tv_sec - pkt->t1.tv_sec;
		rtt.tv_usec = pkt->t3.tv_usec - pkt->t1.tv_usec;
	}
	rtt.tv_usec = (rtt.tv_usec + 500) / 1000; /* convert to ms */
	rtt.tv_sec = rtt.tv_sec * 1000 + rtt.tv_usec;

	printf("%-4d %6lums\n", pkt->seqno, rtt.tv_sec);

	return 0;
}


/* timeout should be an option */
int client_side(in_addr_t host, int port)
{
	struct sockaddr_in to;
	int sock;
	struct pollfd ufd;
	int packets = 0;

	signal(SIGINT, done);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		return 1;
	}

	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = host;
	to.sin_port = htons(port);

	/* Send first packet */
	if (send_ping(sock, (struct sockaddr *)&to)) {
		printf("FATAL: couldn't send first packet!!!\n");
		close(sock);
		return 1;
	}
	++packets;

	ufd.fd = sock;
	ufd.events = POLLIN;

	while (1) {
		int n = poll(&ufd, 1, timeout); /* crude timeout for now.... */

		if (n == 0) {
			/* time to send */
			if (loops && packets == loops)
				done(0);
			send_ping(sock, (struct sockaddr *)&to);
			++packets;
		} else if (n > 0) {
			get_pong(sock);
		} else {
			perror("poll");
			sleep(1); /* holdoff */
		}
	}

	return 0;
}


void done(int sig)
{
	if (!quiet)
		printf("Sent %d Recv %d = %.1f%%\n", pkt_sent, pkt_read,
			   (double)pkt_read / (double)pkt_sent * 100.0);
	exit(0);
}


/****** WARNING: Roman code from here on! You have been warned! ******/


#ifndef __FreeBSD__
/* fd is a socket */
static int get_ifindex(const char *ifname, int fd)
{
	struct ifreq ifr;
	int sock;

	if (!ifname)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

	if (fd == -1)
		sock = socket(PF_INET, SOCK_DGRAM, 0);
	else
		sock = fd;

	/* we don't need to check the return code, as in the case of
	 * failure ifr_ifindex will be 0, and this is sufficient to
	 * signal the error up the stack */
	ioctl(sock, SIOCGIFINDEX, &ifr);

	if (fd == -1)
		close(sock);

	return ifr.ifr_ifindex;
}
#endif

static int proto_udp_writev(int sock, struct iovec *iov, size_t iovlen,
			    struct sockaddr *to, int ifindex)
{
	struct msghdr msg;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = to;
	msg.msg_namelen = sizeof(struct sockaddr_in);
	msg.msg_iov = iov;
	msg.msg_iovlen = iovlen;

#ifndef __FreeBSD__
	if (ifindex) {
		/* send through specific interface */
		struct cmsghdr *cmsg;
		struct in_pktinfo *pktinfo;
		char cbuf[CMSG_SPACE(sizeof(struct in_pktinfo))];

		msg.msg_control = cbuf;
		msg.msg_controllen = sizeof(cbuf);

		/* have to assign msg_controllen before using CMSG macros */
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_IP;
		cmsg->cmsg_type = IP_PKTINFO;
		cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));

		pktinfo = (struct in_pktinfo *)CMSG_DATA(cmsg);
		pktinfo->ipi_ifindex = ifindex;
		pktinfo->ipi_spec_dst.s_addr = 0;
	}
#endif

	return sendmsg(sock, &msg, 0);
}

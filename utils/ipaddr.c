/* ipaddr.c - get various IP information
 * Copyright (C) 2012-2019 Sean MacLennan
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

#define W_ADDRESS  (1 << 0)
#define W_MASK     (1 << 1)
#define W_SUBNET   (1 << 2)
#define W_BITS     (1 << 3)
#define W_GATEWAY  (1 << 4)
#define W_GUESSED  (1 << 5)
#define W_ALL	   (1 << 6)
#define W_FLAGS    (1 << 7)
#define W_SET      (1 << 8)

#if defined(__linux__)
/* Returns the size of src */
size_t strlcpy(char *dst, const char *src, size_t dstlen)
{
	int srclen = strlen(src);

	if (dstlen > 0) {
		if (dstlen > srclen)
			strcpy(dst, src);
		else {
			strncpy(dst, src, dstlen - 1);
			dst[dstlen - 1] = 0;
		}
	}

	return srclen;
}

/* Returns 0 on success, < 0 for errors, and > 0 if ifname not found.
 * The gateway arg can be NULL.
 */
static int get_gateway(const char *ifname, struct in_addr *gateway)
{
	FILE *fp = fopen("/proc/net/route", "r");
	if (!fp)
		return -1;

	char line[128], iface[8];
	uint32_t dest, gw, flags;
	while (fgets(line, sizeof(line), fp))
		if (sscanf(line, "%s %x %x %x", iface, &dest, &gw, &flags) == 4 &&
			strcmp(iface, ifname) == 0 && dest == 0 && (flags & 2)) {
			fclose(fp);
			if (gateway)
				gateway->s_addr = gw;
			return 0;
		}

	fclose(fp);
	return 1;
}

#else
#include <net/route.h>
#include <sys/poll.h>
#include <sys/sysctl.h>

#define RTM_ADDRS ((1 << RTAX_DST) | (1 << RTAX_NETMASK))
#define RTM_SEQ 42
#define RTM_FLAGS (RTF_STATIC | RTF_UP | RTF_GATEWAY)
#define	READ_TIMEOUT 10

struct rtmsg {
	struct rt_msghdr hdr;
	unsigned char data[512];
};

static int rtmsg_send(int s)
{
	struct rtmsg rtmsg;

	memset(&rtmsg, 0, sizeof(rtmsg));
	rtmsg.hdr.rtm_type = RTM_GET;
	rtmsg.hdr.rtm_flags = RTM_FLAGS;
	rtmsg.hdr.rtm_version = RTM_VERSION;
	rtmsg.hdr.rtm_seq = RTM_SEQ;
	rtmsg.hdr.rtm_addrs = RTM_ADDRS;

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_len = sizeof(sa);
	sa.sin_family = AF_INET;

	/* 0.0.0.0/0 */
	unsigned char *cp = rtmsg.data;
	memcpy(cp, &sa, sizeof(sa));
	cp += sizeof(sa);
	memcpy(cp, &sa, sizeof(sa));
	cp += sizeof(sa);
	rtmsg.hdr.rtm_msglen = cp - (unsigned char *)&rtmsg;

	if (write(s, &rtmsg, rtmsg.hdr.rtm_msglen) < 0)
		return -1;

	return 0;
}

static int rtmsg_recv(int s, struct in_addr *gateway)
{
	struct rtmsg rtmsg;
	struct pollfd ufd = { .fd = s, .events = POLLIN };

	do {
		if (poll(&ufd, 1, READ_TIMEOUT * 1000) <= 0)
			return -1;

		if (read(s, (char *)&rtmsg, sizeof(rtmsg)) <= 0)
			return -1;
	} while (rtmsg.hdr.rtm_type != RTM_GET ||
			 rtmsg.hdr.rtm_seq != RTM_SEQ ||
			 rtmsg.hdr.rtm_pid != getpid());

	if (rtmsg.hdr.rtm_version != RTM_VERSION)
		return -1;
	if (rtmsg.hdr.rtm_errno)  {
		errno = rtmsg.hdr.rtm_errno;
		return -1;
	}

	unsigned char *cp = rtmsg.data;
	for (int i = 0; i < RTAX_MAX; i++)
		if (rtmsg.hdr.rtm_addrs & (1 << i)) {
			if (i == RTAX_GATEWAY) {
				*gateway = ((struct sockaddr_in *)cp)->sin_addr;
				return 0;
			}
			cp += SA_SIZE((struct sockaddr *)cp);
		}

	return 1; /* not found */
}

/* ifname ignored */
static int get_gateway(const char *ifname, struct in_addr *gateway)
{
	int s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0)
		return -1;

	if (rtmsg_send(s)) {
		close(s);
		return -1;
	}

	int n = rtmsg_recv(s, gateway);
	if (n) {
		close(s);
		return n;
	}

	close(s);
	return 0;
}
#endif

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

/* Returns 0 on success. The args addr and/or mask and/or gw can be NULL. */
static int ip_addr(const char *ifname,
				   struct in_addr *addr, struct in_addr *mask, struct in_addr *gw)
{
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return -1;

	struct ifreq ifr;
	strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);

	if (addr) {
		if (ioctl(s, SIOCGIFADDR, &ifr) < 0)
			goto failed;
		*addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	}

	if (mask) {
		if (ioctl(s, SIOCGIFNETMASK, &ifr) < 0)
			goto failed;
		*mask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	}

	close(s);

	if (gw)
		return get_gateway(ifname, gw);

	return 0;

failed:
	close(s);
	return -1;
}

static int set_ip(const char *ifname, const char *ip, const char *mask)
{
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		return -1;

	struct ifreq req;
	memset(&req, 0, sizeof(req));

	strlcpy(req.ifr_name, ifname, IF_NAMESIZE);

	struct sockaddr_in *in = (struct sockaddr_in *)&req.ifr_addr;
#ifndef __linux__
	in->sin_len = sizeof(struct sockaddr_in);
#endif
	in->sin_family = AF_INET;
	in->sin_port = 0;
	in->sin_addr.s_addr = inet_addr(ip);

	if (ioctl(s, SIOCSIFADDR, &req))
		goto failed;

 	in->sin_addr.s_addr = inet_addr(mask);
	if (ioctl(s, SIOCSIFNETMASK, &req))
		goto failed;

	req.ifr_flags = IFF_UP | IFF_RUNNING;
	if (ioctl(s, SIOCSIFFLAGS, &req))
		goto failed;

	close(s);
	return 0;

failed:
	close(s);
	return -1;
}

static char *ip_flags(const char *ifname)
{
	static char flagstr[64];
	struct ifreq ifreq;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return "failed";

	memset(&ifreq, 0, sizeof(ifreq));
	strlcpy(ifreq.ifr_name, ifname, IF_NAMESIZE);
	if (ioctl(sock, SIOCGIFFLAGS, &ifreq)) {
		return "Failed";
	}

	strcpy(flagstr, (ifreq.ifr_flags & IFF_UP) ? "UP" : "DOWN");
	if (ifreq.ifr_flags & IFF_RUNNING)
		strcat(flagstr, ",RUNNING");

	return flagstr;
}

static int check_one(const char *ifname, int state, unsigned what)
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
				if (state)
					printf("0.0.0.0 (%s)\n", ifname);
				else
					printf("down (%s)\n", ifname);
				return 0;
			} else if ((what & W_GUESSED) == 0)
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

	if (what & W_FLAGS) {
		if (n++)
			putchar(' ');
		printf("<%s>", ip_flags(ifname));
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
		  "       ipaddr -S <interface> <ip> <mask>\n"
		  "where: -i displays IP address (default)\n"
		  "		  -f display up and running flags\n"
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

	while ((c = getopt(argc, argv, "abfgmishS")) != EOF)
		switch (c) {
		case 'i':
			what |= W_ADDRESS;
			break;
		case 'b':
			what |= W_BITS;
			break;
		case 'f':
			what |= W_FLAGS;
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
		case 'S':
			what |= W_SET;
			break;
		default:
			exit(2);
		}

	if (what & W_SET) {
		if ((what & ~W_SET) || argc - optind < 3) {
			fputs("ipaddr -S <ifname> <ip> <mask>\n", stderr);
			exit(1);
		}
		if (set_ip(argv[optind], argv[optind + 1], argv[optind + 2])) {
			perror("set_ip");
			exit(1);
		}
		return 0;
	}

	if ((what & ~(W_BITS | W_ALL)) == 0)
		what |= W_ADDRESS;

	if (optind < argc) {
		while (optind < argc)
			rc |= check_one(argv[optind++], 0, what);
	} else {
		struct ifaddrs *ifa;

		if (getifaddrs(&ifa)) {
			perror("getifaddrs");
			exit(1);
		}

		for (struct ifaddrs *p = ifa; p; p = p->ifa_next) {
			if (p->ifa_addr->sa_family != AF_INET || (p->ifa_flags & IFF_LOOPBACK))
				continue;

			unsigned up = p->ifa_flags & IFF_UP;
			if ((what & W_ALL) || up)
				rc |= check_one(p->ifa_name, up, what | W_GUESSED);
		}
	}

	return rc;
}

/*
 * Local Variables:
 * compile-command: "gcc -O2 -Wall ipaddr.c -o ipaddr"
 * End:
 */

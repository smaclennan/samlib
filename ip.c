#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

#include "samlib.h"

#ifdef WIN32
#error You want win32/win32-ip.c
#endif

/* Undefine this to use netstat */
#define HAVE_RTMSG

#ifdef __linux__
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
#elif defined(HAVE_RTMSG)
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
int get_gateway(const char *ifname, struct in_addr *gateway)
{
	int s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0)
		return -1;

	int numfibs;
	size_t len = sizeof(numfibs);
	if (sysctlbyname("net.fibs", (void *)&numfibs, &len, NULL, 0) == 0) {
		int	defaultfib;
		len = sizeof(defaultfib);
		if (sysctlbyname("net.my_fibnum", (void *)&defaultfib, &len, NULL, 0) == 0)
			if (setsockopt(s, SOL_SOCKET, SO_SETFIB, (void *)&defaultfib,
						   sizeof(defaultfib))) {
				close(s);
				return -1;
			}
	}

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
#else
#include <ctype.h> // isspace()

/* Returns 0 on success, < 0 for errors, and > 0 if ifname not found.
 * The gateway arg can be NULL.
 */
static int get_gateway(const char *ifname, struct in_addr *gateway)
{
	/* Fall back to netstat */
	FILE *pfp = popen("netstat -nr", "r");
	if (!pfp)
		return -ENOENT;

	char line[128];
	while (fgets(line, sizeof(line), pfp))
		if (strncmp(line, "default", 7) == 0 || strncmp(line, "0.0.0.0", 7) == 0) {
			if (strstr(line, ifname))
				pclose(pfp);
				if (gateway) {
					struct in_addr addr;
					char *p = line + 7;
					while (isspace(*p)) ++p;
					if (inet_aton(p, &addr)) {
						*gateway = addr;
						return 0;
					}
					return -1;
				}
				return 0;
		}

	pclose(pfp);
	return 1;

}
#endif

void free_interfaces(char **ifnames, int n)
{
	for (int i = 0; i < n; ++i)
		free(ifnames[i]);
	free(ifnames);
}

/* Skips the loopback interface */
int get_interfaces(char ***ifnames, uint64_t *state)
{
	struct ifaddrs *ifa;
	int i = 0;
	char **names = NULL;

	if (state) *state = 0;

	if (getifaddrs(&ifa))
		return -errno;

	for (struct ifaddrs *p = ifa; p; p = p->ifa_next) {
		if (p->ifa_addr->sa_family != AF_INET || (p->ifa_flags & IFF_LOOPBACK))
			continue;

		names = realloc(names, sizeof(char *) * (i + 1));
		if (!names)
			goto oom;
		names[i] = strdup(p->ifa_name);
		if (!names[i])
			goto oom;
		if (state && (p->ifa_flags & IFF_UP))
			*state |= 1 << i;
		++i;
	}

	*ifnames = names;
	freeifaddrs(ifa);
	return i;

oom:
	free_interfaces(names, i);
	freeifaddrs(ifa);
	*ifnames = NULL;
	return -ENOMEM;
}

/* Returns 0 on success. The args addr and/or mask and/or gw can be NULL. */
int ip_addr(const char *ifname,
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

/* Returns 0 on success or an errno suitable for gai_strerror().
 * The ipv4 arg should be at least INET_ADDRSTRLEN long or NULL.
 * The ipv6 arg should be at least INET6_ADDRSTRLEN long or NULL.
 */
int get_address(const char *hostname, char *ipv4, char *ipv6)
{
	struct addrinfo *result, *res;
	struct addrinfo hints;
	int err;

	if (ipv4)
		*ipv4 = 0;
	if (ipv6)
		*ipv6 = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	err = getaddrinfo(hostname, NULL, &hints, &result);
	if (err)
		return err;

	for (res = result; res; res = res->ai_next)
		switch (res->ai_family) {
		case AF_INET:
			if (ipv4) {
				struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
				inet_ntop(res->ai_family, &sin->sin_addr, ipv4, INET_ADDRSTRLEN);
			}
			break;
		case AF_INET6:
			if (ipv6) {
				struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)res->ai_addr;
				inet_ntop(res->ai_family, &sin6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
			}
			break;
		}

	freeaddrinfo(result);

	return 0;
}

uint32_t get_address4(const char *hostname)
{
	struct hostent *host = gethostbyname(hostname);
	if (host)
		return ntohl(*(uint32_t *)host->h_addr);
	else
		return 0;
}

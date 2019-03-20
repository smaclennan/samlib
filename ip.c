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

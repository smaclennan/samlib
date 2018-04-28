#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
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

static int check_state(const char *name)
{
	char buff[64];
	int fd;

	snprintf(buff, sizeof(buff), "/sys/class/net/%s/operstate", name);
	fd = open(buff, O_RDONLY);
	if (fd >= 0) {
		int n = read(fd, buff, sizeof(buff));
		close(fd);
		if (n > 0)
			return strncmp(buff, "up", 2) == 0;
	}
	return 0; /* down */
}

/* Skips the loopback interface */
int get_interfaces(char **ifnames, int n, uint64_t *state)
{
	if (state) {
		*state = 0;
		if (n > 64) n = 64;
	}

	int i = 0;
	DIR *dir = opendir("/sys/class/net");
	if (!dir)
		return -ENOENT;

	struct dirent *ent;
	while ((ent = readdir(dir)) && i < n)
		if (*ent->d_name != '.' && strcmp(ent->d_name, "lo")) {
			ifnames[i] = strdup(ent->d_name);
			if (!ifnames[i])
				goto oom;
			if (state)
				*state |= check_state(ent->d_name) << i;
			++i;
		}

	closedir(dir);
	return i;

oom:
	free_interfaces(ifnames, i);
	return -ENOMEM;
}

#else

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

/* Skips the loopback interface */
int get_interfaces(char **ifnames, int n, uint64_t *state)
{
	/* Fall back to ifconfig */
	FILE *pfp = popen("ifconfig -a", "r");
	if (!pfp)
		return -ENOENT;

	if (state) {
		*state = 0;
		if (n > 64) n = 64;
	}

	int i = 0;
	char line[128];
	while (fgets(line, sizeof(line), pfp))
		if (!isspace(*line) && strncmp(line, "lo", 2) && i < n) {
			if (state)
				*state |= (strstr(line, "UP") != NULL) << i;
			ifnames[i] = strdup(strtok(line, ":"));
			if (!ifnames[i])
				goto oom;
			++i;
		}

	pclose(pfp);
	return i;

oom:
	free_interfaces(ifnames, i);
	return -ENOMEM;
}
#endif

/* Returns 0 on success. The args addr and/or mask and/or gw can be NULL. */
int ip_addr(const char *ifname,
			struct in_addr *addr, struct in_addr *mask, struct in_addr *gw)
{
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return -1;

	struct ifreq ifr;
	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);

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

void free_interfaces(char **ifnames, int n)
{
	int i;

	for (i = 0; i < n; ++i) {
		free(ifnames[i]);
		ifnames[i] = NULL;
	}
}

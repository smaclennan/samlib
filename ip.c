#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#endif

#include "samlib.h"

/* WINDOWS interface names:
 *
 * Warning: I am not a windows network expert. I am assuming that for
 * any given machine windows will always enumerate in the same order!
 *
 * For windows I am going to assume that ethN is the Nth ethernet
 * interface and wlanN is the Nth wireless interface.
 */

#ifdef WIN32
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static IP_ADAPTER_INFO *win32_getadapterinfo(void)
{
	ULONG ulOutBufLen = 0;

	/* Initial call to GetAdaptersInfo to get size */
	GetAdaptersInfo(NULL, &ulOutBufLen);

	pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
	if (pAdapterInfo == NULL)
		return NULL;

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) {
		FREE(pAdapterInfo);
		return NULL;
	}

	return pAdapterInfo;
}

static int win32_getinfo(const char *ifname,
						 struct in_addr *addr,
						 struct in_addr *mask,
						 struct in_addr *gw)
{
	PIP_ADAPTER_INFO pAdapter;
	DWORD dwRetVal = 0;
	UINT i, ethn = 0, wlann = 0;
	UINT want_eth = 0, want_wlan = 0;

	if (strncmp(ifname, "eth", 3) == 0)
		want_eth = strtol(ifname + 3, NULL, 10);
	else if (strncmp(ifname, "wlan", 4) == 0)
		want_wlan = strtol(ifname + 4, NULL, 10);
	else
		return -ENOENT;

	PIP_ADAPTER_INFO pAdapterInfo = win32_getadapterinfo();
	if (!pAdapterInfo)
		return -1;

	for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next)
		switch (pAdapter->Type) {
		case MIB_IF_TYPE_ETHERNET:
			if (ethn == want_eth)
				goto got_it;
			++ethn;
			break;
		case IF_TYPE_IEEE80211:
			if (wlann == want_wlan)
				goto got_it;
			++wlann;
			break;
		}

failed:
	FREE(pAdapterInfo);
	return -1; /* not found */

got_it:
	if (addr)
		if (inet_ntoa(pAdapter->IpAddressList.IpAddress.String, addr))
			goto failed;
	if (mask)
		if (inet_ntoa(pAdapter->IpAddressList.IpMask.String, mask))
			goto failed;
	if (gw)
		if (inet_nota(pAdapter->GatewayList.IpAddress.String, gw))
			goto failed;

	FREE(pAdapterInfo);
	return 0;
}

static int win32_get_interfaces(char **ifnames, int n, int flags)
{
	PIP_ADAPTER_INFO pAdapter;
	DWORD dwRetVal = 0;
	UINT i = 0, ethn = 0, wlann = 0;
	char name[16];

	PIP_ADAPTER_INFO pAdapterInfo = win32_getadapterinfo();
	if (!pAdapterInfo)
		return -1;

	for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
		switch (pAdapter->Type) {
		case MIB_IF_TYPE_ETHERNET:
			snprintf(name, sizeof(name), "eth%d", ethn);
			++ethn;
			break;
		case IF_TYPE_IEEE80211:
			snprintf(name, sizeof(name), "wlan%d", wlann);
			++wlann;
			break;
		default:
			continue;
		}

		int up = strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0");
		if (flags) {
			if ((flags & IFACE_UP) && !up)
				continue;
			if ((flags & IFACE_DOWN) && up)
				continue;
		}

		if (i < n) {
			ifaces[i] = strdup(name);
			if (!ifaces[i])
				goto failed;
			++i;
		}
	}

	FREE(pAdapterInfo);
	return i;

failed:
	free_interfaces(ifaces, i);
	FREE(pAdapterInfo);
	return -1;
}
#endif

/* Returns 0 on success. The args addr and/or mask can be NULL. */
int ip_addr(const char *ifname, struct in_addr *addr, struct in_addr *mask)
{
#ifdef WIN32
	return win32_getinfo(ifname, addr, mask, NULL);
#else
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
	return 0;

failed:
	close(s);
	return -1;
#endif
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

/* Returns 0 on success, < 0 for errors, and > 0 if ifname not found.
 * The gateway arg can be NULL.
 */
int get_gateway(const char *ifname, struct in_addr *gateway)
{
#ifdef WIN32
	return win32_getinfo(ifname, NULL, NULL, gateway);
#elif defined(__linux__)
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
#else
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

#endif
}

#ifdef __linux__
static int check_flags(const char *name, int flags)
{
	char buff[64];
	int fd;

	if (flags == 0)
		return 1;

	snprintf(buff, sizeof(buff), "/sys/class/net/%s/operstate", name);
	fd = open(buff, O_RDONLY);
	if (fd >= 0) {
		int n = read(fd, buff, sizeof(buff));
		close(fd);
		if (n > 0) {
			int up = strncmp(buff, "up", 2) == 0;
			return (up && (flags & IFACE_UP)) || (!up && (flags & IFACE_DOWN));
		}
	}
	return 0; /* down */
}
#endif

/* Skips the loopback interface */
int get_interfaces(char **ifnames, int n, int flags)
{
#ifdef __linux__
	int i = 0;
	DIR *dir = opendir("/sys/class/net");
	if (!dir)
		return -ENOENT;

	struct dirent *ent;
	while ((ent = readdir(dir)) && i < n)
		if (*ent->d_name != '.' && strcmp(ent->d_name, "lo")) {
			if (check_flags(ent->d_name, flags)) {
				ifnames[i] = strdup(ent->d_name);
				if (!ifnames[i])
					goto oom;
				++i;
			}
		}

	closedir(dir);
	return i;

oom:
	free_interfaces(ifnames, i);
	return -ENOMEM;
#elif defined(WIN32)
	return win32_get_interfaces(ifnames, n, flags);
#else
	/* Fall back to ifconfig */
	FILE *pfp = popen("ifconfig -a", "r");
	if (!pfp)
		return -ENOENT;

	int i = 0;
	char line[128];
	while (fgets(line, sizeof(line), pfp))
		if (!isspace(*line) && strncmp(line, "lo", 2) && i < n) {
			if (flags) {
				char *up = strstr(line, "UP");
				if ((flags & IFACE_UP) && up == NULL)
					continue;
				if ((flags & IFACE_DOWN) && up)
					continue;
			}
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
#endif
}

void free_interfaces(char **ifnames, int n)
{
	int i;

	for (i = 0; i < n; ++i)
		if (ifnames[i]) {
			free(ifnames[i]);
			ifnames[i] = NULL;
		}
}

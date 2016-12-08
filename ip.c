#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#endif

#include "samlib.h"

/* Returns 0 on success. The args addr and/or mask can be NULL. */
int ip_addr(const char *ifname, struct in_addr *addr, struct in_addr *mask)
{
#ifdef WIN32
	return ENOSYS;
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

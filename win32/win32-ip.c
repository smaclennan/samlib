#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "../samlib.h"

/* WINDOWS interface names:
 *
 * Warning: I am not a windows network expert. I am assuming that for
 * any given machine windows will always enumerate in the same order!
 *
 * For windows I am going to assume that ethN is the Nth ethernet
 * interface and wlanN is the Nth wireless interface.
 */

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

static IP_ADAPTER_INFO *win32_getadapterinfo(void)
{
	PIP_ADAPTER_INFO pAdapterInfo;
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

/* Returns 0 on success. The args addr and/or mask and/or gw can be NULL. */
int ip_addr(const char *ifname,
			struct in_addr *addr, struct in_addr *mask, struct in_addr *gw)
{
	PIP_ADAPTER_INFO pAdapter;
	DWORD dwRetVal = 0;
	UINT ethn = 0, wlann = 0;
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
	if (addr) {
		addr->s_addr = inet_addr(pAdapter->IpAddressList.IpAddress.String);
		if (addr->s_addr == INADDR_NONE)
			goto failed;
	}
	if (mask) {
		mask->s_addr = inet_addr(pAdapter->IpAddressList.IpMask.String);
		if (mask->s_addr == INADDR_NONE)
			goto failed;
	}
	if (gw) {
		gw->s_addr = inet_addr(pAdapter->GatewayList.IpAddress.String);
		if (gw->s_addr == INADDR_NONE)
			goto failed;
	}

	FREE(pAdapterInfo);
	return 0;
}

int get_interfaces(char **ifnames, int n, uint64_t *state)
{
	PIP_ADAPTER_INFO pAdapter;
	DWORD dwRetVal = 0;
	int i = 0, ethn = 0, wlann = 0;
	char name[16];

	PIP_ADAPTER_INFO pAdapterInfo = win32_getadapterinfo();
	if (!pAdapterInfo)
		return -1;

	if (state) {
		*state = 0;
		if (n > 64) n = 64;
	}

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

		if (i >= n)
			break;

		if (state)
			*state |= (strcmp(pAdapter->IpAddressList.IpAddress.String, "0.0.0.0") != 0) << i;

		ifnames[i] = strdup(name);
		if (!ifnames[i])
			goto failed;
		++i;
	}

	FREE(pAdapterInfo);
	return i;

failed:
	free_interfaces(ifnames, i);
	FREE(pAdapterInfo);
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

	for (i = 0; i < n; ++i)
		if (ifnames[i]) {
			free(ifnames[i]);
			ifnames[i] = NULL;
		}
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

/* Returns 0 on success. The args addr and/or mask can be NULL. */
int ip_addr(const char *ifname, struct in_addr *addr, struct in_addr *mask)
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
	return 0;

failed:
	close(s);
	return -1;
}


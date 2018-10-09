#include "samlib.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BACKLOG 10

/* Warning: closes the socket on error. */
static int set_nonblocking(int sock)
{
	int optval = fcntl(sock, F_GETFL, 0);
	if (optval == -1 || fcntl(sock, F_SETFL, optval | O_NONBLOCK)) {
		close(sock);
		return -1;
	}

	return sock;
}

int socket_listen(int port, int flags)
{
	struct sockaddr_in sock_name;
	int s, optval;

	sock_name.sin_family = AF_INET;
	if (flags & SOCK_LOCAL)
		sock_name.sin_addr.s_addr = htonl(0x7f000001);
	else
		sock_name.sin_addr.s_addr = INADDR_ANY;
	sock_name.sin_port = htons(port);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
		return -1;

	optval = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
				   (char *)&optval, sizeof(optval)) == -1 ||
		bind(s, (struct sockaddr *)&sock_name, sizeof(sock_name)) == -1 ||
		listen(s, BACKLOG) == -1) {
		close(s);
		return -1;
	}

	if (flags & SOCK_NONBLOCKING)
		return set_nonblocking(s);

	return s;
}

int socket_accept(int sock, char *ip, int flags)
{
	struct sockaddr_in sock_name;
	unsigned addrlen = sizeof(sock_name);
	int new = accept(sock, (struct sockaddr *)&sock_name, &addrlen);
	if (new < 0)
		return -1;

	if (ip)
		strcpy(ip, inet_ntoa(sock_name.sin_addr));

	int opt = 1;
	setsockopt(new, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

	if (flags & SOCK_NONBLOCK)
		return set_nonblocking(new);

	return new;
}

static int try_connect(struct addrinfo *r, int *deferred, int flags)
{
	int sock = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
	if (sock < 0)
		return -1;

	if ((flags & SOCK_NONBLOCKING) == 0) {
		if (connect(sock, r->ai_addr, r->ai_addrlen) == 0) {
			*deferred = 0;
			return sock;
		}
	} else {
		if (set_nonblocking(sock) == -1)
			return -1;

		if (connect(sock, r->ai_addr, r->ai_addrlen) == 0) {
			*deferred = 0;
			return sock;
		}

		if (errno == EINPROGRESS || errno == EAGAIN) {
			*deferred = 1;
			return sock;
		}
	}

	close(sock);
	return -1;
}

/* Supports ipv6 */
int socket_connect(const char *hostname, uint16_t port, int flags)
{
	int sock = -1, deferred;
	struct addrinfo hints, *result, *r;

	/* We need this or we will get tcp and udp */
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;

	char portstr[8];
	uint2str(port, portstr);
	if (getaddrinfo(hostname, portstr, &hints, &result))
		return -1;

	for (r = result; r; r = r->ai_next) {
		sock = try_connect(r, &deferred, flags);
		if (sock >= 0)
			break;
	}

	freeaddrinfo(result);

	return sock;
}

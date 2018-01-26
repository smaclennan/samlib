#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

int md5sum(const char *fname)
{
	int fd, n;
	char buf[64 * 1024]; /* nobody needs more than 64k */
	uint8_t hash[MD5_DIGEST_LEN];
	md5ctx ctx;

	if (strcmp(fname, "-") == 0)
		fd = 0;
	else {
		fd = open(fname, O_RDONLY);
		if (fd <= 0) {
			perror(fname);
			return 1;
		}
	}

	md5_init(&ctx);
	while ((n = read(fd, buf, sizeof(buf))) > 0)
		md5_update(&ctx, buf, n);
	md5_final(&ctx, hash);

	if (fd)
		close(fd);

	if (n) {
		fprintf(stderr, "%s: read error\n", fname);
		return 1;
	}

	printf("%s  %s\n", md5str(hash, buf), fname);
	return 0;
}

int main(int argc, char *argv[])
{
	int i, rc = 0;

	if (argc == 1)
		md5sum("-");
	else
		for (i = 1; i < argc; ++i)
			if (md5sum(argv[i]))
				rc = 1;

	return rc;
}

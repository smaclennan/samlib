#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

int main(int argc, char *argv[])
{
	uint8_t hash[MD5_DIGEST_LEN];
	char str[MD5_DIGEST_LEN * 2 + 1];
	int i, rc = 0;

	if (argc == 1) {
		_md5sum(0, hash);
		puts(md5str(hash, str));
	} else
		for (i = 1; i < argc; ++i)
			if (md5sum(argv[i], hash)) {
				perror(argv[i]);
				rc = 1;
			} else
				puts(md5str(hash, str));

	return rc;
}

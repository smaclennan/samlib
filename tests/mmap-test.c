#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include "../samlib.h"

#ifdef TESTALL
int mmap_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	char fname[MAXPATHLEN];
	int i, rc = 1;

	int fd = mktempfile(fname, sizeof(fname));
	if (fd < 0) {
		perror(fname);
		return 1;
	}

	for (i = 0; i < 10; ++i)
		if (write(fd, "Hello world!\n", 13) != 13) {
			printf("write failed\n");
			goto cleanup;
			return 1;
		}

	close(fd);

	fd = open(fname, O_RDONLY | O_BINARY);
	if (fd < 0) {
		perror(fname);
		goto cleanup;
	}

	char *mem = must_mmap_file(13 * 10, 0, 0, fd);

	if (strncmp(mem + 5 * 13, "Hello world!\n", 13) == 0)
		rc = 0; /* success */
	else
		printf("read failed\n");

cleanup:
	if (fd >= 0)
		close(fd);
	unlink(fname);
	return rc;
}

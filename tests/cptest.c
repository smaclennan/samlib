#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "../samlib.h"

#define FILENAME "readfile.txt" /* reuse this */
#define TONAME "cptest.txt"

#define MAX_FILE_SIZE (128 * 1024)
static uint64_t buf[MAX_FILE_SIZE / sizeof(uint64_t)];
static uint64_t buf2[MAX_FILE_SIZE / sizeof(uint64_t)];
static int len;

static int create_random_bin_file(const char *fname)
{
	int fd, i, n, count;

	len = xorshift128plus() % MAX_FILE_SIZE;
	count = len / sizeof(uint64_t);

	for (i = 0; i < count; ++i)
		buf[i] = xorshift128plus();

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
	if (fd < 0) {
		perror(fname);
		return 1;
	}
	n = write(fd, buf, len);
	close(fd);

	if (n != len) {
		printf("create error");
		return 1;
	}

	return 0;
}

#ifdef TESTALL
static int cptest_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	int fd, n, rc;

	if (create_random_bin_file(FILENAME))
		return 1;

	rc = copy_file(FILENAME, TONAME);
	if (rc != len) {
		printf("copy_file failed: %d != %d\n", rc, len);
		return 1;
	}

	fd = open(TONAME, O_RDONLY | O_BINARY);
	if (fd < 0) {
		perror(TONAME);
		return 1;
	}
	n = read(fd, buf2, sizeof(buf2));
	close(fd);

	if (n != len) {
		printf("read error\n");
		return 1;
	}

	if (memcmp(buf, buf2, len)) {
		printf("Output did not match\n");
		return 1;
	}

	unlink(FILENAME);
	unlink(TONAME);

	return 0;
}

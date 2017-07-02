#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <samlib.h>

#define FILENAME "readfile.txt" /* reuse this */
#define TONAME "cptest.txt"

#define MAX_FILE_SIZE (128 * 1024)
static uint64_t buf[MAX_FILE_SIZE / sizeof(uint64_t)];
static uint64_t buf2[MAX_FILE_SIZE / sizeof(uint64_t)];
static int len;

static void create_random_file(const char *fname)
{
	int fd, i, n, count;

	len = xorshift128plus() % MAX_FILE_SIZE;
	count = len / sizeof(uint64_t);

	for (i = 0; i < count; ++i)
		buf[i] = xorshift128plus();

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror(fname);
		exit(1);
	}
	n = write(fd, buf, len);
	close(fd);

	if (n != len) {
		printf("create error");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int fd, n, rc;

	create_random_file(FILENAME);

	rc = copy_file(FILENAME, TONAME);
	if (rc != len) {
		printf("copy_file failed: %d != %d\n", rc, len);
		exit(1);
	}

	fd = open(TONAME, O_RDONLY);
	if (fd < 0) {
		perror(TONAME);
		exit(1);
	}
	n = read(fd, buf2, sizeof(buf2));
	close(fd);

	if (n != len) {
		printf("read error");
		exit(1);
	}

	if (memcmp(buf, buf2, len)) {
		printf("Output did not match\n");
		exit(1);
	}

	unlink(FILENAME);
	unlink(TONAME);

	return 0;
}

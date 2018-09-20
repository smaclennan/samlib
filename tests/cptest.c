#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "../samlib.h"

#define FILENAME "/tmp/readfile.txt" /* reuse this */
#define TONAME "/tmp/cptest.txt"

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

static int test_tmpfilename(void)
{
	int rc = 0;
	char path[128];

	char *tmp = tmpfilename(NULL);
	assert(tmp && *tmp);
	if (*(tmp + strlen(tmp) - 1) == '/') {
		printf("Bad tmp '%s'\n", tmp);
		rc = 1;
	}

	strfmt(path, sizeof(path), "%s/test", tmp);

	char *fname = tmpfilename("test");
	if (strcmp(fname, path)) {
		printf("'%s' != '%s'\n", fname, path);
		rc = 1;
	}
	free(fname);

	fname = tmpfilename("/test");
	if (strcmp(fname, path)) {
		printf("'%s' != '%s'\n", fname, path);
		rc = 1;
	}
	free(fname);
	free(tmp);

	return rc;
}

#ifdef TESTALL
static int cptest_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	int fd, n, rc;

	char *filename = tmpfilename("readfile.txt");
	char *toname = tmpfilename("cptest.txt");
	if (!filename || !toname) {
		puts("cptest: Out of memory!");
		return 1;
	}

	if (create_random_bin_file(filename))
		return 1;

	rc = copy_file(filename, toname);
	if (rc != len) {
		printf("copy_file failed: %d != %d\n", rc, len);
		return 1;
	}

	fd = open(toname, O_RDONLY | O_BINARY);
	if (fd < 0) {
		perror(toname);
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

	unlink(filename);
	unlink(toname);

	free(filename);
	free(toname);

	/* Since we use tmpfilename, test it here */
	return test_tmpfilename();
}

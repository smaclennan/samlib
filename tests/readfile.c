#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "../samlib.h"

#define MAX_LINES		1000
#define MAX_CHARS		128

static char readbuf[MAX_LINES * (MAX_CHARS + 1) + 1];
static int buflen, readlen;

static char next_ch(void)
{
	static uint64_t r;
	static int n = 8;
	char ch;

	while (1) {
		if (n == 8) {
			r = xorshift128plus();
			n = 0;
		}

		ch = (char)r & 0x7f;
		r >>= 8;
		++n;

		if (ch >= ' ' && ch <= '~')
			return ch;
	}
}

static int create_random_file(const char *fname)
{
	int lines = (xorshift128plus() % (MAX_LINES - 1)) + 1;
	int i, j, ch, n;
	char *p = readbuf;

	int fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
	if (fd < 0) {
		perror(fname);
		return 1;
	}

	for (i = 0; i < lines; ++i) {
		ch = xorshift128plus() % MAX_CHARS; /* zero is ok */
		for (j = 0; j < ch; ++j) {
			*p++ = next_ch();
			++buflen;
		}
		*p++ = '\n';
		++buflen;
	}
	*p = 0;

	n = write(fd, readbuf, buflen);

	close(fd);

	if (n != buflen) {
		puts("write error");
		return 1;
	}

	readlen = buflen;

	return 0;
}

static int truncate_file(const char *fname)
{
	int rc, fd = open(fname, O_WRONLY | O_BINARY, 0644);
	if (fd < 0) {
		perror(fname);
		return 1;
	}

	--readlen;
	rc = ftruncate(fd, readlen);
	close(fd);

	readbuf[readlen] = 0;

	return rc;
}

static char *p = readbuf;
static int lineno = 0;

int check_line(char *line, void *arg)
{
	int len = strlen(line);

	if (buflen < len) {
		puts("Out of buffer");
		return 1;
	}

	++lineno;
	if (strncmp(line, p, len)) {
		printf("%d: %s\n", lineno, line);
		return 1;
	}

	p += len + 1; /* also skip LF */
	buflen -= len + 1;

	return 0;
}

#ifdef TESTALL
static int readfile_main(void)
#else
int main(int argc, char *argv[])
#endif
{
	char *filename = tmpfilename("readfile.txt");
	assert(filename);

	if (create_random_file(filename))
		return 1;

	if (readfile(check_line, NULL, filename, 0)) {
		perror(filename);
		return 1;
	}

	if (buflen) {
		printf("buflen %d\n", buflen);
		return 1;
	}

	/* Test no NL at EOF */
	if (truncate_file(filename)) {
		perror("truncate");
		return 1;
	}

	p = readbuf;
	lineno = 0;
	buflen = readlen;
	if (readfile(check_line, NULL, filename, 0)) {
		perror(filename);
		return 1;
	}

	/* It will be -1 since check_line assumes a final NL */
	if (buflen != -1) {
		printf("buflen %d\n", buflen);
		return 1;
	}

	unlink(filename);
	free(filename);

	return 0;
}

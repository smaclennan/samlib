#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#if 1
#define BUFLEN 4096
#else
#define BUFLEN 10 /* for debugging */
#endif

typedef struct strip_ctx {
	int fd; /* input */
	char buf[BUFLEN];
	int cur;
	int len;
	int pushed;
	int in_string;
} strip_ctx;

void strip_ctx_init(strip_ctx *ctx, int fd)
{
	ctx->fd = fd;
	ctx->cur = ctx->len = 0;
	ctx->pushed = -1;
}

static int getch(strip_ctx *ctx)
{
	if (ctx->cur >= ctx->len) {
		/* need to read more */
		ctx->len = read(ctx->fd, ctx->buf, sizeof(ctx->buf));
		if (ctx->len <= 0)
			return -1;
		ctx->cur = 0;
	}

	return ctx->buf[ctx->cur++];
}

static int strip_comments(strip_ctx *ctx)
{
	int c, nested = 0, state = 0;

	if (ctx->pushed != -1) {
		c = ctx->pushed;
		ctx->pushed = -1;
		return c;
	}

	while ((c = getch(ctx)) != EOF) {
		switch (ctx->in_string) {
		case 0:
			break;
		case 1:
			if (c == '\\')
				ctx->in_string = 2;
			else if (c == '"')
				ctx->in_string = 0;
			return c;
		case 2:
			ctx->in_string = 1;
			return c;
		}

		switch (state) {
		case 0:
			/* normal state */
			if (c == '"') {
				ctx->in_string = 1;
				return c;
			} else if (c == '/')
				state = 1;
			else
				return c;
			break;

		case 1:
			/* possible comment start */
			if (c == '*') {
				nested = 1;
				state = 2;
			} else if (c == '/') {
				while ((c = getch(ctx)) != EOF && c != '\n') ;
				if (c == '\n')
					return c;
				state = 0;
			} else {
				state = 0;
				ctx->pushed = c;
				return '/';
			}
			break;

		case 2:
			/* possible comment end */
			if (c == '*')
				state = 3;
			else if (c == '/')
				state = 4;
			break;

		case 3:
			/* comment end? */
			if (c == '/') {
				if (--nested == 0)
					state = 0;
			} else {
				if (c != '*')
					state = 2;
			}
			break;

		case 4:
			/* possible nested comment */
			if (c == '*') {
				++nested;
				state = 2;
			} else if (c != '/')
				state = 2;
			break;

		default:
			puts("Internal error");
		}
	}

	return EOF;
}

/* strips comments and blank lines */
char *mygets(strip_ctx *ctx, char *buf, int len)
{
	int cur, c;
	char *p;

again:
	cur = 0;
	while ((c = strip_comments(ctx)) != EOF) {
		buf[cur++] = c;
		if (cur >= len - 1) {
			buf[cur] = 0;
			goto checkit;
		}
		if (c == '\n') {
			buf[cur] = 0;
			goto checkit;
		}
	}

	if (cur) {
		buf[cur] = 0;
		goto checkit;
	}

	return NULL;

checkit:
	/* check for an empty line */
	for (p = buf; isspace(*p); ++p) ;
	if (*p)
		return buf;
	goto again;
}

#ifndef IN_PREPRO
int main(int argc, char *argv[])
{
	char *fname = "ctest.c";
	char buf[1024];
	strip_ctx ctx;

	if (argc > 1)
		fname = argv[1];

	int fd = open(fname, O_RDONLY);
	if (fd < 0) {
		perror(fname);
		exit(1);
	}

	strip_ctx_init(&ctx, fd);

	while (mygets(&ctx, buf, sizeof(buf)))
		printf("%s", buf);

	close(fd);

	return 0;
}
#endif

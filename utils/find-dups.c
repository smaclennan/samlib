#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <regex.h>
#include <fnmatch.h>
#include <sys/stat.h>

#include <samlib.h>

static int do_md5;
static int do_norm;

struct file {
	char *path;
	char *name;
	uint8_t *digest; /* if do_md5 set */
	struct file *next;
};
static struct file *files;

static void *must_memdup(const void *mem, int len)
{
	void *new = malloc(len);
	if (!new) {
		puts("Out of memory!");
		exit(1);
	}
	memcpy(new, mem, len);
	return new;
}

static char *normalize(const char *fname)
{
	static char normal[PATH_MAX + 1];

	if (do_norm) {
		const char *in = fname;
		char *out = normal;

		while (*in) {
			if (strncasecmp(in, "the", 3) == 0 &&
				!isalnum(*(in + 3)) &&
				(in == fname || !isalnum(*(in - 1))))
				in += 2; /* ++in below */
			else if (isalpha(*in))
				*out++ = tolower(*in);
			else if (isdigit(*in))
				*out++ = *in;
			++in;
		}

		*out = '\0';

		return normal;
	} else
		return (char *)fname;
}

static int md5_file(const char *path, uint8_t *digest)
{
	uint8_t buf[4096];
	int n, zero = 1;
	md5ctx ctx;

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror(path);
		return -1;
	}

	md5_init(&ctx);
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		md5_update(&ctx, buf, n);
		zero = 0;
	}
	md5_final(&ctx, digest);

	close(fd);

	if (zero) {
		/* All zero length files have the same md5sum */
		fprintf(stderr, "%s: zero length file\n", path);
		return 1;
	}

	return n;
}

static int file_func(const char *path, struct stat *sbuf)
{
	uint8_t digest[MD5_DIGEST_LEN];
	char *normalized;
	int md5_set = 0, md5_match;

	const char *fname = strrchr(path, '/');
	if (fname)
		++fname;
	else
		fname = path;

	if (do_md5) {
		if (md5_file(path, digest))
			printf("Unable to calc md5 for %s\n", path);
		else
			md5_set = 1;
	}

	normalized = normalize(fname);

	struct file *f;
	for (f = files; f; f = f->next) {
		md5_match = 1; /* assume matched */
		if (md5_set) {
			if (f->digest && memcmp(digest, f->digest, MD5_DIGEST_LEN))
				md5_match = 0;
		}

		if (strcmp(f->name, normalized) == 0) {
			printf("DUP %s\n    %s\n", f->path, path);
			if (!md5_match)
				printf("    MD5 does not match\n");
			return 0;
		}

		if (md5_set && md5_match && f->digest)
			printf("MD5 %s\n    %s\n", f->path, path);
	}

	struct file *new = calloc(1, sizeof(struct file));
	if (!new) {
		puts("Out of memory.");
		exit(1);
	}

	new->path = must_strdup(path);
	new->name = must_strdup(normalized);
	if (md5_set)
		new->digest = must_memdup(digest, MD5_DIGEST_LEN);
	new->next = files;
	files = new;
	return 0;
}

int main(int argc, char *argv[])
{
	int c, flags = 0, error = 0;

	while ((c = getopt(argc, argv, "i:f:lmnv")) != EOF)
		switch (c) {
		case 'i': add_ignore(NULL, optarg); break;
		case 'f': add_filter(NULL, optarg); break;
		case 'l': flags |= WALK_LINKS; break;
		case 'n': do_norm = 1; break;
		case 'm': do_md5 = 1; break;
		case 'v': flags |= WALK_VERBOSE; break;
		default:
			puts("Sorry!");
			exit(1);
		}

	if (optind == argc) {
		puts("I need a dir");
		exit(1);
	}

	for (c = optind; c < argc; ++c)
		error |= walkfiles(NULL, argv[c], file_func, flags);

	return error;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include "samlib.h"

struct test {
	char *in;
	char *hash;
} test_suite[] = {
/* From RFC */
{ "", "d41d8cd98f00b204e9800998ecf8427e" },
{ "a", "0cc175b9c0f1b6a831c399e269772661" },
{ "abc", "900150983cd24fb0d6963f7d28e17f72" },
{ "message digest", "f96b697d7cb7938d525a2f31aaf161d0" },
{ "abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b" },
{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
  "d174ab98d277d9f5a5611c2c9f419d9f" },
{ "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
  "57edf4a22be3c955ac49da2e2107b67a" },
/* Mine - results from md5sum */
{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123", /* 56 */
  "27eca74a76daae63f472b250b5bcff9d" },
{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234", /* 57 */
  "7b704b4e3d241d250fd327d433c27250" },
{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789ab", /* 64 */
  "a27155ae242d64584221b66416d22a61" },
};
#define N_TEST ((sizeof(test_suite) / sizeof(struct test)))

static int do_testsuite(void)
{
	uint8_t hash[MD5_DIGEST_LEN];
	char str[34];
	int i, rc = 0;

	for (i = 0; i < N_TEST; ++i) {
#if 0
		md5ctx ctx;
		md5_init(&ctx);
		md5_update(&ctx, test_suite[i].in, strlen(test_suite[i].in));
		md5_final(&ctx, hash);
#else
		md5(test_suite[i].in, strlen(test_suite[i].in), hash);
#endif

		if (strcmp(md5str(hash, str), test_suite[i].hash)) {
			printf("Mismatch: %s\n", test_suite[i].in);
			puts(str);
			puts(test_suite[i].hash);
			rc = 1;
		}
	}

	return rc;
}

static int md5sum(char *line, void *data)
{
	md5ctx *ctx = data;
	md5_update(ctx, line, strlen(line));
	md5_update(ctx, "\n", 1);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
		return do_testsuite();

	md5ctx ctx;
	md5_init(&ctx);

	if (readfile(md5sum, &ctx, argv[1])) {
		puts("Readfile failed");
		exit(1);
	}

	uint8_t hash[MD5_DIGEST_LEN];
	char str[34];
	md5_final(&ctx, hash);
	printf("%s  %s\n", md5str(hash, str), argv[1]);

	return 0;
}

/*
 * Local Variables:
 * compile-command: "gcc -O3 -Wall md5test.c -o md5test ./libsamlib.a"
 * End:
 */

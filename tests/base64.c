#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <samlib.h>

/* Test vectors from rfc4648 */

struct test_vector {
	const char *in;
	const char *out;
} test_vectors[] = {
	{ "", "" },
	{ "f", "Zg==" },
	{ "fo", "Zm8=" },
	{ "foo", "Zm9v" },
	{ "foob", "Zm9vYg==" },
	{ "fooba", "Zm9vYmE=" },
	{ "foobar", "Zm9vYmFy" },
};
#define N_TESTS (sizeof(test_vectors) / sizeof(struct test_vector))

int main(int argc, char *argv[])
{
	char buf[16]; /* tons of space */
	int i, n, len, rc = 0;

	for (i = 0; i < N_TESTS; ++i) {
		n = base64_encode(buf, sizeof(buf), (uint8_t *)test_vectors[i].in, strlen(test_vectors[i].in));
		if (n < 0) {
			printf("%d: encode failed\n", i);
			rc = 1;
		} else if (strcmp(buf, test_vectors[i].out)) {
			printf("%d: encode expected '%s' got '%s'\n", i,
				   test_vectors[i].out, buf);
			rc = 1;
		}

		n = base64_decode((uint8_t *)buf, sizeof(buf) - 1, test_vectors[i].out, strlen(test_vectors[i].out));
		if (n < 0) {
			printf("%d: decode failed\n", i);
			rc = 1;
		} else {
			buf[n] = 0;
			if (strcmp(buf, test_vectors[i].in)) {
				printf("%d: encode expected '%s' got '%s'\n", i,
					   test_vectors[i].out, buf);
				rc = 1;
			}
		}
	}

	/* test the encode length */
	len = base64_encoded_len(strlen(test_vectors[4].in));
	if (len != 8) {
		printf("Encode length problem 1 %d != 8\n", len);
		rc = 1;
	}
	n = base64_encode(buf, len + 1, (uint8_t *)test_vectors[4].in, strlen(test_vectors[4].in));
	if (n != 8) {
		printf("Encode length problem 2 %d != 8\n", n);
		rc = 1;
	}
	n = base64_encode(buf, len, (uint8_t *)test_vectors[4].in, strlen(test_vectors[4].in));
	if (n != -1) {
		printf("Encode length problem 3 %d should be -1\n", n);
		rc = 1;
	}

	/* test url safe */
	n = base64_encode(buf, sizeof(buf), (uint8_t *)"~~~~", 4);
	if (strncmp(buf, "fn5+fg==", 8)) {
		printf("Problems encoding ~~~~\n");
		rc = 1;
	}
	base64url_safe = 1;
	n = base64_encode(buf, sizeof(buf), (uint8_t *)"~~~~", 4);
	if (strncmp(buf, "fn5-fg==", 8)) {
		printf("Problems encoding ~~~~ url safe\n");
		rc = 1;
	}
	base64url_safe = 0;

	return rc;
}

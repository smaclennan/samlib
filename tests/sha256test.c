#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include "../samlib.h"

/* GCOV CFLAGS=-coverage make D=1
 * 100% Thu Feb 15, 2018
 */

/*
 *  Define patterns for testing
 */
#define TEST1    "abc"
#define TEST2_1													\
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
#define TEST3    "a"                            /* times 1000000 */
#define TEST4a   "01234567012345670123456701234567"
#define TEST4b   "01234567012345670123456701234567"
/* an exact multiple of 512 bits */
#define TEST4   TEST4a TEST4b                   /* times 10 */

#define TEST7_256													\
	"\xbe\x27\x46\xc6\xdb\x52\x76\x5f\xdb\x2f\x88\x70\x0f\x9a\x73"
#define TEST8_256														\
	"\xe3\xd7\x25\x70\xdc\xdd\x78\x7c\xe3\x88\x7a\xb2\xcd\x68\x46\x52"
#define TEST9_256														\
	"\x3e\x74\x03\x71\xc8\x10\xc2\xb9\x9f\xc0\x4e\x80\x49\x07\xef\x7c"	\
	"\xf2\x6b\xe2\x8b\x57\xcb\x58\xa3\xe2\xf3\xc0\x07\x16\x6e\x49\xc1"	\
	"\x2e\x9b\xa3\x4c\x01\x04\x06\x91\x29\xea\x76\x15\x64\x25\x45\x70"	\
	"\x3a\x2b\xd9\x01\xe1\x6e\xb0\xe0\x5d\xeb\xa0\x14\xeb\xff\x64\x06"	\
	"\xa0\x7d\x54\x36\x4e\xff\x74\x2d\xa7\x79\xb0\xb3"
#define TEST10_256														\
	"\x83\x26\x75\x4e\x22\x77\x37\x2f\x4f\xc1\x2b\x20\x52\x7a\xfe\xf0"	\
	"\x4d\x8a\x05\x69\x71\xb1\x1a\xd5\x71\x23\xa7\xc1\x37\x76\x00\x00"	\
	"\xd7\xbe\xf6\xf3\xc1\xf7\xa9\x08\x3a\xa3\x9d\x81\x0d\xb3\x10\x77"	\
	"\x7d\xab\x8b\x1e\x7f\x02\xb8\x4a\x26\xc7\x73\x32\x5f\x8b\x23\x74"	\
	"\xde\x7a\x4b\x5a\x58\xcb\x5c\x5c\xf3\x5b\xce\xe6\xfb\x94\x6e\x5b"	\
	"\xd6\x94\xfa\x59\x3a\x8b\xeb\x3f\x9d\x65\x92\xec\xed\xaa\x66\xca"	\
	"\x82\xa2\x9d\x0c\x51\xbc\xf9\x33\x62\x30\xe5\xd7\x84\xe4\xc0\xa4"	\
	"\x3f\x8d\x79\xa3\x0a\x16\x5c\xba\xbe\x45\x2b\x77\x4b\x9c\x71\x09"	\
	"\xa9\x7d\x13\x8f\x12\x92\x28\x96\x6f\x6c\x0a\xdc\x10\x6a\xad\x5a"	\
	"\x9f\xdd\x30\x82\x57\x69\xb2\xc6\x71\xaf\x67\x59\xdf\x28\xeb\x39"	\
	"\x3d\x54\xd6"

#define length(x) (sizeof(x)-1)

static struct test256 {
	uint8_t *in;
	int len;
	int repeatcount;
	uint8_t extrabits;
	int n_extrabits;
	const char *digest_str;
} test256[] = {
	/* 1 */ { (uint8_t *)TEST1, length(TEST1), 1, 0, 0, "ba7816bf8f01cfea4141"
			  "40de5dae2223b00361a396177a9cb410ff61f20015ad" },
	/* 2 */ { (uint8_t *)TEST2_1, length(TEST2_1), 1, 0, 0, "248d6a61d20638b8"
			  "e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1" },
	/* 3 */ { (uint8_t *)TEST3, length(TEST3), 1000000, 0, 0, "cdc76e5c9914fb92"
			  "81a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0" },
	/* 4 */ { (uint8_t *)TEST4, length(TEST4), 10, 0, 0, "594847328451bdfa"
			  "85056225462cc1d867d877fb388df0ce35f25ab5562bfbb5" },
	/* 5 */ { (uint8_t *)"", 0, 0, 0x68, 5, "d6d3e02a31a84a8caa9718ed6c2057be"
			  "09db45e7823eb5079ce7a573a3760f95" },
	/* 6 */ { (uint8_t *)"\x19", 1, 1, 0, 0, "68aa2e2ee5dff96e3355e6c7ee373e3d"
			  "6a4e17f75f9518d843709c0c9bc3e3d4" },
	/* 7 */ { (uint8_t *)TEST7_256, length(TEST7_256), 1, 0x60, 3, "77ec1dc8"
			  "9c821ff2a1279089fa091b35b8cd960bcaf7de01c6a7680756beb972" },
	/* 8 */ { (uint8_t *)TEST8_256, length(TEST8_256), 1, 0, 0, "175ee69b02ba"
			  "9b58e2b0a5fd13819cea573f3940a94f825128cf4209beabb4e8" },
	/* 9 */ { (uint8_t *)TEST9_256, length(TEST9_256), 1, 0xA0, 3, "3e9ad646"
			  "8bbbad2ac3c2cdc292e018ba5fd70b960cf1679777fce708fdb066e9" },
	/* 10 */ { (uint8_t *)TEST10_256, length(TEST10_256), 1, 0, 0, "97dbca7d"
			   "f46d62c8a422c941dd7e835b8ad3361763f7e9b2d95f4f0da6e1ccbc" },
};
#define N_TEST256 ((sizeof(test256) / sizeof(struct test256)))

#ifndef TESTALL
#include "../sha256.c"

int main(void)
#else
static int sha256_main(void)
#endif
{
	int i, j, len, rc = 0;
	uint8_t *in;
	uint8_t digest[SHA256_DIGEST_SIZE];
	char digeststr[65];

	for (i = 0; i < N_TEST256; ++i) {
		if (test256[i].n_extrabits)
			continue; /* we don't support final bits */

		if (test256[i].repeatcount > 1) {
			uint8_t *p;

			len = test256[i].repeatcount * test256[i].len;
			p = in = malloc(len + 1);
			if (!in) {
				puts("Out of memory!");
				return 1;
			}
			for (j = 0; j < test256[i].repeatcount; ++j) {
				memcpy(p, test256[i].in, test256[i].len);
				p += test256[i].len;
			}
			*p = 0;
		} else {
			in = test256[i].in;
			len = test256[i].len;
		}

		sha256(in, len, digest);
		sha256str(digest, digeststr);

		if (strcmp(digeststr, test256[i].digest_str)) {
			printf("%d: failed\n", i + 1);
			printf("  %s\n", digeststr);
			printf("  %s\n", test256[i].digest_str);
			rc = 1;
		}

		if (test256[i].repeatcount > 1)
			free(in);
	}

	return rc;
}

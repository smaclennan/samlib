#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

/* RFC 1321 */

typedef struct md5_ctx {
	uint32_t abcd[4];
	uint8_t buf[64]; /* 512 / 8 */
	int cur;
	uint32_t size; /* size in bytes */
} md5ctx;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* F(X,Y,Z) = XY v not(X) Z */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
/* G(X,Y,Z) = XZ v Y not(Z) */
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
/* H(X,Y,Z) = X xor Y xor Z */
#define H(x, y, z) ((x) ^ (y) ^ (z))
/* I(X,Y,Z) = Y xor (X v not(Z)) */
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

#define A (ctx->abcd[0])
#define B (ctx->abcd[1])
#define C (ctx->abcd[2])
#define D (ctx->abcd[3])

#define ROTATE(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/* Calculate one block */
static void md5_calc(md5ctx *ctx)
{
	uint32_t aa, bb, cc, dd, *x;

	x = (uint32_t *)ctx->buf;

	/* Save A as AA, B as BB, C as CC, and D as DD. */
	aa = A;
	bb = B;
	cc = C;
	dd = D;

	/* Round 1 */
#define R1(a, b, c, d, k, s, t) a = b + ROTATE((a + F(b, c, d) + x[k] + t), s)
	R1(A, B, C, D,   0,  7, 0xd76aa478);
	R1(D, A, B, C,   1, 12, 0xe8c7b756);
	R1(C, D, A, B,   2, 17, 0x242070db);
	R1(B, C, D, A,   3, 22, 0xc1bdceee);
	R1(A, B, C, D,   4,  7, 0xf57c0faf);
	R1(D, A, B, C,   5, 12, 0x4787c62a);
	R1(C, D, A, B,   6, 17, 0xa8304613);
	R1(B, C, D, A,   7, 22, 0xfd469501);
	R1(A, B, C, D,   8,  7, 0x698098d8);
	R1(D, A, B, C,   9, 12, 0x8b44f7af);
	R1(C, D, A, B,  10, 17, 0xffff5bb1);
	R1(B, C, D, A,  11, 22, 0x895cd7be);
	R1(A, B, C, D,  12,  7, 0x6b901122);
	R1(D, A, B, C,  13, 12, 0xfd987193);
	R1(C, D, A, B,  14, 17, 0xa679438e);
	R1(B, C, D, A,  15, 22, 0x49b40821);

	 /* Round 2 */
#define R2(a, b, c, d, k, s, t) a = b + ROTATE((a + G(b,c,d) + x[k] + t), s)
	R2(A, B, C, D,   1,  5, 0xf61e2562);
	R2(D, A, B, C,   6,  9, 0xc040b340);
	R2(C, D, A, B,  11, 14, 0x265e5a51);
	R2(B, C, D, A,   0, 20, 0xe9b6c7aa);
	R2(A, B, C, D,   5,  5, 0xd62f105d);
	R2(D, A, B, C,  10,  9, 0x02441453);
	R2(C, D, A, B,  15, 14, 0xd8a1e681);
	R2(B, C, D, A,   4, 20, 0xe7d3fbc8);
	R2(A, B, C, D,   9,  5, 0x21e1cde6);
	R2(D, A, B, C,  14,  9, 0xc33707d6);
	R2(C, D, A, B,   3, 14, 0xf4d50d87);
	R2(B, C, D, A,   8, 20, 0x455a14ed);
	R2(A, B, C, D,  13,  5, 0xa9e3e905);
	R2(D, A, B, C,   2,  9, 0xfcefa3f8);
	R2(C, D, A, B,   7, 14, 0x676f02d9);
	R2(B, C, D, A,  12, 20, 0x8d2a4c8a);

	/* Round 3 */
#define R3(a, b, c, d, k, s, t) a = b + ROTATE((a + H(b,c,d) + x[k] + t), s)
	R3(A, B, C, D,   5,  4, 0xfffa3942);
	R3(D, A, B, C,   8, 11, 0x8771f681);
	R3(C, D, A, B,  11, 16, 0x6d9d6122);
	R3(B, C, D, A,  14, 23, 0xfde5380c);
	R3(A, B, C, D,   1,  4, 0xa4beea44);
	R3(D, A, B, C,   4, 11, 0x4bdecfa9);
	R3(C, D, A, B,   7, 16, 0xf6bb4b60);
	R3(B, C, D, A,  10, 23, 0xbebfbc70);
	R3(A, B, C, D,  13,  4, 0x289b7ec6);
	R3(D, A, B, C,   0, 11, 0xeaa127fa);
	R3(C, D, A, B,   3, 16, 0xd4ef3085);
	R3(B, C, D, A,   6, 23, 0x04881d05);
	R3(A, B, C, D,   9,  4, 0xd9d4d039);
	R3(D, A, B, C,  12, 11, 0xe6db99e5);
	R3(C, D, A, B,  15, 16, 0x1fa27cf8);
	R3(B, C, D, A,   2, 23, 0xc4ac5665);

	/* Round 4 */
#define R4(a, b, c, d, k, s, t) a = b + ROTATE((a + I(b,c,d) + x[k] + t), s)
	R4(A, B, C, D,   0,  6, 0xf4292244);
	R4(D, A, B, C,   7, 10, 0x432aff97);
	R4(C, D, A, B,  14, 15, 0xab9423a7);
	R4(B, C, D, A,   5, 21, 0xfc93a039);
	R4(A, B, C, D,  12,  6, 0x655b59c3);
	R4(D, A, B, C,   3, 10, 0x8f0ccc92);
	R4(C, D, A, B,  10, 15, 0xffeff47d);
	R4(B, C, D, A,   1, 21, 0x85845dd1);
	R4(A, B, C, D,   8,  6, 0x6fa87e4f);
	R4(D, A, B, C,  15, 10, 0xfe2ce6e0);
	R4(C, D, A, B,   6, 15, 0xa3014314);
	R4(B, C, D, A,  13, 21, 0x4e0811a1);
	R4(A, B, C, D,   4,  6, 0xf7537e82);
	R4(D, A, B, C,  11, 10, 0xbd3af235);
	R4(C, D, A, B,   2, 15, 0x2ad7d2bb);
	R4(B, C, D, A,   9, 21, 0xeb86d391);

	/* Then perform the following additions. (That is increment each
	 * of the four registers by the value it had before this block
	 *was started.) */
	A = A + aa;
	B = B + bb;
	C = C + cc;
	D = D + dd;
}

void md5_init(md5ctx *ctx)
{
	A = 0x67452301;
	B = 0xefcdab89;
	C = 0x98badcfe;
	D = 0x10325476;

	ctx->cur = 0;
	ctx->size = 0;
}

void md5_update(md5ctx *ctx, void *data, int len)
{
	while (len > 0) {
		int left = 64 - ctx->cur;
		int n = MIN(len, left);

		memcpy(ctx->buf + ctx->cur, data, n);
		ctx->cur += n;
		ctx->size += n;
		len -= n;
		data += n;

		if (ctx->cur == 64) {
			md5_calc(ctx);
			ctx->cur = 0;
		}
	}
}

static void md5_pad(md5ctx *ctx)
{
	ctx->buf[ctx->cur++] = 0x80;
	memset(ctx->buf + ctx->cur, 0, 64 - ctx->cur);

	if (ctx->cur > 56) {
		md5_calc(ctx);
		memset(ctx->buf, 0, sizeof(ctx->buf));
		ctx->cur = 0;
	}

	/* append length */
	uint64_t size = ctx->size * 8; /* convert to bits */
	memcpy(ctx->buf + 56, &size, 8);
}

void md5_final(md5ctx *ctx, uint8_t *hash)
{
	md5_pad(ctx);
	md5_calc(ctx);
	memcpy(hash, ctx->abcd, sizeof(ctx->abcd));
	memset(ctx, 0, sizeof(md5ctx));
}

char *md5str(uint8_t *hash, char *str)
{
	int i;

	for (i = 0; i < 32; i += 2, ++hash)
		sprintf(str + i, "%02x", *hash);

	return str;
}

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

int main(int argc, char *argv[])
{
	md5ctx ctx;
	uint8_t hash[16];
	char str[34];
	int i;

	for (i = 0; i < N_TEST; ++i) {
		md5_init(&ctx);
		md5_update(&ctx, test_suite[i].in, strlen(test_suite[i].in));
		md5_final(&ctx, hash);

		if (strcmp(md5str(hash, str), test_suite[i].hash)) {
			printf("Mismatch: %s\n", test_suite[i].in);
			puts(str);
			puts(test_suite[i].hash);
		}
	}

	return 0;
}

/*
MD5 test suite:
	*/
/*
 * Local Variables:
 * compile-command: "gcc -O3 -Wall md5.c -o mymd5"
 * End:
 */

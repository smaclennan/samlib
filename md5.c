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

static uint32_t t[] = {
	0, /* RFC 1 based */
	0xd76aa478,
	0xe8c7b756,
	0x242070db,
	0xc1bdceee,
	0xf57c0faf,
	0x4787c62a,
	0xa8304613,
	0xfd469501,
	0x698098d8,
	0x8b44f7af,
	0xffff5bb1,
	0x895cd7be,
	0x6b901122,
	0xfd987193,
	0xa679438e,
	0x49b40821,
	0xf61e2562,
	0xc040b340,
	0x265e5a51,
	0xe9b6c7aa,
	0xd62f105d,
	0x2441453,
	0xd8a1e681,
	0xe7d3fbc8,
	0x21e1cde6,
	0xc33707d6,
	0xf4d50d87,
	0x455a14ed,
	0xa9e3e905,
	0xfcefa3f8,
	0x676f02d9,
	0x8d2a4c8a,
	0xfffa3942,
	0x8771f681,
	0x6d9d6122,
	0xfde5380c,
	0xa4beea44,
	0x4bdecfa9,
	0xf6bb4b60,
	0xbebfbc70,
	0x289b7ec6,
	0xeaa127fa,
	0xd4ef3085,
	0x4881d05,
	0xd9d4d039,
	0xe6db99e5,
	0x1fa27cf8,
	0xc4ac5665,
	0xf4292244,
	0x432aff97,
	0xab9423a7,
	0xfc93a039,
	0x655b59c3,
	0x8f0ccc92,
	0xffeff47d,
	0x85845dd1,
	0x6fa87e4f,
	0xfe2ce6e0,
	0xa3014314,
	0x4e0811a1,
	0xf7537e82,
	0xbd3af235,
	0x2ad7d2bb,
	0xeb86d391,
};

void Decode(
  uint32_t*      output,
  unsigned char* input,
  uint32_t       len)
{
  uint32_t i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
  {
	output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j + 1]) << 8) |
				(((uint32_t)input[j + 2]) << 16) | (((uint32_t)input[j + 3]) << 24);
  }
}

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
#define R1(a, b, c, d, k, s, i) a = b + ROTATE((a + F(b, c, d) + x[k] + t[i]), s)
	R1(A, B, C, D,   0,  7,  1);
	R1(D, A, B, C,   1, 12,  2);
	R1(C, D, A, B,   2, 17,  3);
	R1(B, C, D, A,   3, 22,  4);
	R1(A, B, C, D,   4,  7,  5);
	R1(D, A, B, C,   5, 12,  6);
	R1(C, D, A, B,   6, 17,  7);
	R1(B, C, D, A,   7, 22,  8);
	R1(A, B, C, D,   8,  7,  9);
	R1(D, A, B, C,   9, 12, 10);
	R1(C, D, A, B,  10, 17, 11);
	R1(B, C, D, A,  11, 22, 12);
	R1(A, B, C, D,  12,  7, 13);
	R1(D, A, B, C,  13, 12, 14);
	R1(C, D, A, B,  14, 17, 15);
	R1(B, C, D, A,  15, 22, 16);

	 /* Round 2 */
#define R2(a, b, c, d, k, s, i) a = b + ROTATE((a + G(b,c,d) + x[k] + t[i]), s)
	R2(A, B, C, D,   1,  5, 17);
	R2(D, A, B, C,   6,  9, 18);
	R2(C, D, A, B,  11, 14, 19);
	R2(B, C, D, A,   0, 20, 20);
	R2(A, B, C, D,   5,  5, 21);
	R2(D, A, B, C,  10,  9, 22);
	R2(C, D, A, B,  15, 14, 23);
	R2(B, C, D, A,   4, 20, 24);
	R2(A, B, C, D,   9,  5, 25);
	R2(D, A, B, C,  14,  9, 26);
	R2(C, D, A, B,   3, 14, 27);
	R2(B, C, D, A,   8, 20, 28);
	R2(A, B, C, D,  13,  5, 29);
	R2(D, A, B, C,   2,  9, 30);
	R2(C, D, A, B,   7, 14, 31);
	R2(B, C, D, A,  12, 20, 32);

	/* Round 3 */
#define R3(a, b, c, d, k, s, i) a = b + ROTATE((a + H(b,c,d) + x[k] + t[i]), s)
	R3(A, B, C, D,   5,  4, 33);
	R3(D, A, B, C,   8, 11, 34);
	R3(C, D, A, B,  11, 16, 35);
	R3(B, C, D, A,  14, 23, 36);
	R3(A, B, C, D,   1,  4, 37);
	R3(D, A, B, C,   4, 11, 38);
	R3(C, D, A, B,   7, 16, 39);
	R3(B, C, D, A,  10, 23, 40);
	R3(A, B, C, D,  13,  4, 41);
	R3(D, A, B, C,   0, 11, 42);
	R3(C, D, A, B,   3, 16, 43);
	R3(B, C, D, A,   6, 23, 44);
	R3(A, B, C, D,   9,  4, 45);
	R3(D, A, B, C,  12, 11, 46);
	R3(C, D, A, B,  15, 16, 47);
	R3(B, C, D, A,   2, 23, 48);

	/* Round 4 */
#define R4(a, b, c, d, k, s, i) a = b + ROTATE((a + I(b,c,d) + x[k] + t[i]), s)
	R4(A, B, C, D,   0,  6, 49);
	R4(D, A, B, C,   7, 10, 50);
	R4(C, D, A, B,  14, 15, 51);
	R4(B, C, D, A,   5, 21, 52);
	R4(A, B, C, D,  12,  6, 53);
	R4(D, A, B, C,   3, 10, 54);
	R4(C, D, A, B,  10, 15, 55);
	R4(B, C, D, A,   1, 21, 56);
	R4(A, B, C, D,   8,  6, 57);
	R4(D, A, B, C,  15, 10, 58);
	R4(C, D, A, B,   6, 15, 59);
	R4(B, C, D, A,  13, 21, 60);
	R4(A, B, C, D,   4,  6, 61);
	R4(D, A, B, C,  11, 10, 62);
	R4(C, D, A, B,   2, 15, 63);
	R4(B, C, D, A,   9, 21, 64);

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

void md5_update(md5ctx *ctx, uint8_t *data, int len)
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

	/* append length */
	uint64_t size = ctx->size * 8; /* convert to bits */
	memcpy(ctx->buf + 56, &size, 8);
}

void md5_final(md5ctx *ctx, uint8_t *hash)
{
	md5_pad(ctx);
	md5_calc(ctx);
	memcpy(hash, ctx->abcd, sizeof(ctx->abcd));
}

int main(int argc, char *argv[])
{
	md5ctx ctx;
	uint8_t hash[16];

	md5_init(&ctx);
	md5_final(&ctx, hash);

	int i;
	for (i = 0; i < 16; ++i)
		printf("%02x", hash[i]);
	putchar('\n');
	puts("d41d8cd98f00b204e9800998ecf8427e");
	return 0;
}

/*
MD5 test suite:
MD5 ("") = d41d8cd98f00b204e9800998ecf8427e
MD5 ("a") = 0cc175b9c0f1b6a831c399e269772661
MD5 ("abc") = 900150983cd24fb0d6963f7d28e17f72
MD5 ("message digest") = f96b697d7cb7938d525a2f31aaf161d0
MD5 ("abcdefghijklmnopqrstuvwxyz") = c3fcd3d76192e4007dfb496cca67e13b
MD5 ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") =
d174ab98d277d9f5a5611c2c9f419d9f
MD5 ("123456789012345678901234567890123456789012345678901234567890123456
78901234567890") = 57edf4a22be3c955ac49da2e2107b67a
	*/
/*
 * Local Variables:
 * compile-command: "gcc -O3 -Wall md5.c -o mymd5"
 * End:
 */

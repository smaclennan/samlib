#include "samlib.h"
#include <fcntl.h>

/* RFC 1321 */

#define A (ctx->abcd[0])
#define B (ctx->abcd[1])
#define C (ctx->abcd[2])
#define D (ctx->abcd[3])
/* Nobody knows what happened to E... */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

#if defined(WIN32) || !defined(__x86_64__)
#define ROTATE(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#else
static inline uint32_t ROTATE(uint32_t word, uint8_t bits)
{   /* We need at least -O for the asm code to work */
#ifdef __DEBUG__
	return ((word << bits) | (word >> (32 - bits)));
#else
	uint32_t res;
	asm ("mov %2, %0;\n" /* Don't clobber word */
		 "rol %1, %0;\n"
		 : "=r" (res)
		 : "n" (bits), "r" (word));
	return res;
#endif
}
#endif

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
#define R1(a, b, c, d, k, s, t) a = b + ROTATE((a + F(b,c,d) + x[k] + t), s)
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

	/* Then perform the following additions. */
	A += aa;
	B += bb;
	C += cc;
	D += dd;
}

void md5_init(md5ctx *ctx)
{
	A = 0x67452301;
	B = 0xefcdab89;
	C = 0x98badcfe;
	D = 0x10325476;

	ctx->cur  = 0;
	ctx->size = 0;
}

void md5_update(md5ctx *ctx, const void *data, int len)
{
	while (len > 0) {
		int left = 64 - ctx->cur;
		int n = len < left ? len : left;

		memcpy(ctx->buf + ctx->cur, data, n);
		ctx->cur += n;
		ctx->size += n;
		len -= n;
		WIN32_COERCE data += n;

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

/* str needs to be MD5_DIGEST_LEN * 2 + 1 */
char *md5str(uint8_t *hash, char *str)
{
	char *p = str;

	for (int i = 0; i < MD5_DIGEST_LEN * 2; i += 2, ++hash) {
		*p++ = tohex[(*hash >> 4) & 0xf];
		*p++ = tohex[*hash & 0xf];
	}
	*p++ = 0;

	return str;
}

void md5(const void *data, int len, uint8_t *hash)
{
	md5ctx ctx;

	md5_init(&ctx);
	md5_update(&ctx, data, len);
	md5_final(&ctx, hash);
}

int _md5sum(int fd, uint8_t *hash)
{
	char buf[64 * 1024];
	int n;
	md5ctx ctx;

	md5_init(&ctx);
	while ((n = read(fd, buf, sizeof(buf))) > 0)
		md5_update(&ctx, buf, n);
	md5_final(&ctx, hash);

	if (n) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int md5sum(const char *fname, uint8_t *hash)
{
	int fd = open(fname, O_RDONLY);
	if (fd < 0)
		return -1;

	int rc = _md5sum(fd, hash);

	close(fd);

	return rc;
}

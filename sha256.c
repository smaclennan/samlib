/***************************** sha256.c *****************************/
/********************* See RFC 4634 for details *********************/
/*
 * Description:
 *   This file implements the Secure Hash Signature Standard
 *   algorithms as defined in the National Institute of Standards
 *   and Technology Federal Information Processing Standards
 *   Publication (FIPS PUB) 180-1 published on April 17, 1995, 180-2
 *   published on August 1, 2002, and the FIPS PUB 180-2 Change
 *   Notice published on February 28, 2004.
 *
 *   A combined document showing all algorithms is available at
 *       http://csrc.nist.gov/publications/fips/
 *       fips180-2/fips180-2withchangenotice.pdf
 *
 *   The SHA-256 algorithm produces 256-bit message digests for a
 *   given data stream. It should take about 2**n steps to find a
 *   message with the same digest as a given message and 2**(n/2) to
 *   find any two messages with the same digest, when n is the digest
 *   size in bits. Therefore, this algorithm can serve as a means of
 *   providing a "fingerprint" for a message.
 *
 * Portability Issues:
 *   SHA-256 is defined in terms of 32-bit "words".  This code uses
 *   <stdint.h> (included via "sha.h") to define 32 and 8 bit unsigned
 *   integer types. If your C compiler does not support 32 bit
 *   unsigned integers, this code is not appropriate.
 *
 * Caveats:
 *   SHA-256 is designed to work with messages less than 2^64 bits
 *   long. This implementation uses 256Input() to hash the bits that
 *   are a multiple of the size of an 8-bit character, and then uses
 *   256FinalBits() to hash the final few bits of the input.
 */

/* SAMLIB notes:
 * SHA-224 removed
 * Exported function names changed
 * Various small cleanups
 */

#include "samlib.h"

#define SHA256_Message_Block_Size (SHA256_DIGEST_SIZE * 2)
#define PAD_BYTE 0x80

/*
 * These definitions are defined in FIPS-180-2, section 4.1.
 * Ch() and Maj() are defined identically in sections 4.1.1,
 * 4.1.2 and 4.1.3.
 *
 * The definitions used in FIPS-180-2 are as follows:
 */

#define USE_MODIFIED_MACROS
#ifndef USE_MODIFIED_MACROS
#define SHA_Ch(x,y,z)        (((x) & (y)) ^ ((~(x)) & (z)))
#define SHA_Maj(x,y,z)       (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#else /* USE_MODIFIED_MACROS */
/*
 * The following definitions are equivalent and potentially faster.
 */

#define SHA_Ch(x, y, z)      (((x) & ((y) ^ (z))) ^ (z))
#define SHA_Maj(x, y, z)     (((x) & ((y) | (z))) | ((y) & (z)))
#endif /* USE_MODIFIED_MACROS */

/* Define the SHA shift and rotate right macro */
#define SHA256_SHR(bits,word)      ((word) >> (bits))

#ifdef __clang__
/* Note: bits must be a constant */
static inline uint32_t SHA256_ROTR(uint8_t bits, uint32_t word)
{   /* We need at least -O for the asm code to work */
	uint32_t res;
	asm ("mov %2, %0;\n" /* Don't clobber word */
		 "ror %1, %0;\n"
		 : "=r" (res)
		 : "n" (bits), "r" (word));
	return res;
}
#else
/* gcc knows to convert this pattern to a ror, clang does not */
#define SHA256_ROTR(bits, word)						\
	(((word) >> (bits)) | ((word) << (32 - (bits))))
#endif

/* Define the SHA SIGMA and sigma macros */
#define SHA256_SIGMA0(word)												\
	(SHA256_ROTR( 2,word) ^ SHA256_ROTR(13,word) ^ SHA256_ROTR(22,word))
#define SHA256_SIGMA1(word)												\
	(SHA256_ROTR( 6,word) ^ SHA256_ROTR(11,word) ^ SHA256_ROTR(25,word))
#define SHA256_sigma0(word)												\
	(SHA256_ROTR( 7,word) ^ SHA256_ROTR(18,word) ^ SHA256_SHR( 3,word))
#define SHA256_sigma1(word)												\
	(SHA256_ROTR(17,word) ^ SHA256_ROTR(19,word) ^ SHA256_SHR(10,word))

/* Local Function Prototypes */
static void SHA256ProcessMessageBlock(sha256ctx *context);

/*
 * SHA256Reset
 *
 * Description:
 *   This function will initialize the sha256ctx in preparation
 *   for computing a new SHA256 message digest.
 *
 * Parameters:
 *   context: [out]
 *     The context to init.
 */
void sha256_init(sha256ctx *ctx)
{
	/* Initial Hash Values: FIPS-180-2 section 5.3.2 */
	static uint32_t SHA256_H0[SHA256_DIGEST_SIZE/4] = {
		0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
		0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
	};

	memset(ctx, 0, sizeof(sha256ctx));
	memcpy(ctx->h, SHA256_H0, sizeof(SHA256_H0));
}

/*
 * SHA256Input
 *
 * Description:
 *   This function accepts an array of octets as the next portion
 *   of the message.
 *
 * Parameters:
 *   context: [in/out]
 *     The SHA context to update
 *   message_array: [in]
 *     An array of characters representing the next portion of
 *     the message.
 *   length: [in]
 *     The length of the message in message_array
 */
void sha256_update(sha256ctx *context, const uint8_t *message_array,
				   unsigned int length)
{
	if (length == 0)
		return;

	context->len += length;

	if (context->index == 0)
		while (length >= SHA256_Message_Block_Size) {
			memcpy(context->block, message_array, SHA256_Message_Block_Size);
			SHA256ProcessMessageBlock(context);
			message_array += SHA256_Message_Block_Size;
			length -= SHA256_Message_Block_Size;
		}

	while (length-- > 0) {
		context->block[context->index++] = *message_array & 0xFF;
		if (context->index == SHA256_Message_Block_Size)
			SHA256ProcessMessageBlock(context);

		message_array++;
	}
}

/*
 * SHA256PadMessage
 *
 * Description:
 *   According to the standard, the message must be padded to an
 *   even 512 bits. The first padding bit must be a '1'. The
 *   last 64 bits represent the length of the original message.
 *   All bits in between should be 0. This helper function will pad
 *   the message according to those rules by filling the
 *   block array accordingly. When it returns, it can be
 *   assumed that the message digest has been computed.
 *
 * Parameters:
 *   context: [in/out]
 *     The context to pad
 *   Pad_Byte: [in]
 *     The last byte to add to the digest before the 0-padding
 *     and length. This will contain the last bits of the message
 *     followed by another single bit. If the message was an
 *     exact multiple of 8-bits long, Pad_Byte will be 0x80.
 *
 * Returns:
 *   Nothing.
 */
static inline void SHA256PadMessage(sha256ctx *context)
{
	/*
	 * Check to see if the current message block is too small to hold
	 * the initial padding bits and length. If so, we will pad the
	 * block, process it, and then continue padding into a second
	 * block.
	 */
	if (context->index >= (SHA256_Message_Block_Size-8)) {
		context->block[context->index++] = PAD_BYTE;
		while (context->index < SHA256_Message_Block_Size)
			context->block[context->index++] = 0;
		SHA256ProcessMessageBlock(context);
	} else
		context->block[context->index++] = PAD_BYTE;

	while (context->index < (SHA256_Message_Block_Size-8))
		context->block[context->index++] = 0;

	/*
	 * Store the message length as the last 8 octets
	 */
	context->len *= 8; /* convert to bits */
	context->block[56] = (uint8_t)(context->len >> 56);
	context->block[57] = (uint8_t)(context->len >> 48);
	context->block[58] = (uint8_t)(context->len >> 40);
	context->block[59] = (uint8_t)(context->len >> 32);
	context->block[60] = (uint8_t)(context->len >> 24);
	context->block[61] = (uint8_t)(context->len >> 16);
	context->block[62] = (uint8_t)(context->len >> 8);
	context->block[63] = (uint8_t)(context->len);

	SHA256ProcessMessageBlock(context);
}

/*
 * SHA256Result
 *
 * Description:
 *   This function will return the 256-bit message
 *   digest into the Message_Digest array provided by the caller.
 *   NOTE: The first octet of hash is stored in the 0th element,
 *      the last octet of hash in the 32nd element.
 *
 * Parameters:
 *   context: [in/out]
 *     The context to use to calculate the SHA hash.
 *   Message_Digest: [out]
 *     Where the digest is returned.
 */
void sha256_final(sha256ctx *ctx, uint8_t *digest)
{
	SHA256PadMessage(ctx);

	for (int i = 0; i < SHA256_DIGEST_SIZE; ++i)
		digest[i] = (uint8_t)(ctx->h[i>>2] >> 8 * (3 - (i & 0x03)));

	memset(ctx, 0, sizeof(sha256ctx)); /* zeroize */
}

/*
 * SHA256ProcessMessageBlock
 *
 * Description:
 *   This function will process the next 512 bits of the message
 *   stored in the block array.
 *
 * Parameters:
 *   context: [in/out]
 *     The SHA context to update
 *
 * Returns:
 *   Nothing.
 *
 * Comments:
 *   Many of the variable names in this code, especially the
 *   single character names, were used because those were the
 *   names used in the publication.
 */
static void SHA256ProcessMessageBlock(sha256ctx *context)
{
#if 1
	/* Constants defined in FIPS-180-2, section 4.2.2 */
	static const uint32_t K[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
		0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
		0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
		0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
		0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
		0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
		0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
		0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
		0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
		0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};
#endif
	int        t;                       /* Loop counter */
	uint32_t   temp1, temp2;            /* Temporary word value */
	uint32_t   W[64];                   /* Word sequence */
	uint32_t   A, B, C, D, E, F, G, H;  /* Word buffers */

	/*
	 * Initialize the first 16 words in the array W
	 * This is worth unrolling
	 */
#define CB_INIT(t4) \
	((((uint32_t)context->block[t4]) << 24) |		 \
	 (((uint32_t)context->block[t4 + 1]) << 16) |	 \
	 (((uint32_t)context->block[t4 + 2]) << 8) |	 \
	 (((uint32_t)context->block[t4 + 3])))

	W[0] = CB_INIT(0);
	W[1] = CB_INIT(4);
	W[2] = CB_INIT(8);
	W[3] = CB_INIT(12);
	W[4] = CB_INIT(16);
	W[5] = CB_INIT(20);
	W[6] = CB_INIT(24);
	W[7] = CB_INIT(28);
	W[8] = CB_INIT(32);
	W[9] = CB_INIT(36);
	W[10] = CB_INIT(40);
	W[11] = CB_INIT(44);
	W[12] = CB_INIT(48);
	W[13] = CB_INIT(52);
	W[14] = CB_INIT(56);
	W[15] = CB_INIT(60);

	/* This is not worth unrolling */
	for (t = 16; t < 64; t++)
		W[t] = SHA256_sigma1(W[t-2]) + W[t-7] +
			SHA256_sigma0(W[t-15]) + W[t-16];

	A = context->h[0];
	B = context->h[1];
	C = context->h[2];
	D = context->h[3];
	E = context->h[4];
	F = context->h[5];
	G = context->h[6];
	H = context->h[7];

#if 1
	for (t = 0; t < 64; t++) {
		temp1 = H + SHA256_SIGMA1(E) + SHA_Ch(E,F,G) + K[t] + W[t];
		temp2 = SHA256_SIGMA0(A) + SHA_Maj(A,B,C);
		H = G;
		G = F;
		F = E;
		E = D + temp1;
		D = C;
		C = B;
		B = A;
		A = temp1 + temp2;
	}
#else
	// this implements the message block process by unrolling the loop calls
#define UNROLL( a, b, c, d, e, f, g, h, Wi, Ki ) \
		temp1 = h + SHA256_SIGMA1(e) + SHA_Ch(e, f, g) + Ki + W[Wi]; \
		temp2 = SHA256_SIGMA0(a) + SHA_Maj(a, b, c); \
		d += temp1; \
		h = temp1 + temp2

	UNROLL( A, B, C, D, E, F, G, H, 0,  0x428a2f98 );
	UNROLL( H, A, B, C, D, E, F, G, 1,  0x71374491 );
	UNROLL( G, H, A, B, C, D, E, F, 2,  0xb5c0fbcf );
	UNROLL( F, G, H, A, B, C, D, E, 3,  0xe9b5dba5 );
	UNROLL( E, F, G, H, A, B, C, D, 4,  0x3956c25b );
	UNROLL( D, E, F, G, H, A, B, C, 5,  0x59f111f1 );
	UNROLL( C, D, E, F, G, H, A, B, 6,  0x923f82a4 );
	UNROLL( B, C, D, E, F, G, H, A, 7,  0xab1c5ed5 );
	UNROLL( A, B, C, D, E, F, G, H, 8,  0xd807aa98 );
	UNROLL( H, A, B, C, D, E, F, G, 9,  0x12835b01 );
	UNROLL( G, H, A, B, C, D, E, F, 10, 0x243185be );
	UNROLL( F, G, H, A, B, C, D, E, 11, 0x550c7dc3 );
	UNROLL( E, F, G, H, A, B, C, D, 12, 0x72be5d74 );
	UNROLL( D, E, F, G, H, A, B, C, 13, 0x80deb1fe );
	UNROLL( C, D, E, F, G, H, A, B, 14, 0x9bdc06a7 );
	UNROLL( B, C, D, E, F, G, H, A, 15, 0xc19bf174 );
	UNROLL( A, B, C, D, E, F, G, H, 16, 0xe49b69c1 );
	UNROLL( H, A, B, C, D, E, F, G, 17, 0xefbe4786 );
	UNROLL( G, H, A, B, C, D, E, F, 18, 0x0fc19dc6 );
	UNROLL( F, G, H, A, B, C, D, E, 19, 0x240ca1cc );
	UNROLL( E, F, G, H, A, B, C, D, 20, 0x2de92c6f );
	UNROLL( D, E, F, G, H, A, B, C, 21, 0x4a7484aa );
	UNROLL( C, D, E, F, G, H, A, B, 22, 0x5cb0a9dc );
	UNROLL( B, C, D, E, F, G, H, A, 23, 0x76f988da );
	UNROLL( A, B, C, D, E, F, G, H, 24, 0x983e5152 );
	UNROLL( H, A, B, C, D, E, F, G, 25, 0xa831c66d );
	UNROLL( G, H, A, B, C, D, E, F, 26, 0xb00327c8 );
	UNROLL( F, G, H, A, B, C, D, E, 27, 0xbf597fc7 );
	UNROLL( E, F, G, H, A, B, C, D, 28, 0xc6e00bf3 );
	UNROLL( D, E, F, G, H, A, B, C, 29, 0xd5a79147 );
	UNROLL( C, D, E, F, G, H, A, B, 30, 0x06ca6351 );
	UNROLL( B, C, D, E, F, G, H, A, 31, 0x14292967 );
	UNROLL( A, B, C, D, E, F, G, H, 32, 0x27b70a85 );
	UNROLL( H, A, B, C, D, E, F, G, 33, 0x2e1b2138 );
	UNROLL( G, H, A, B, C, D, E, F, 34, 0x4d2c6dfc );
	UNROLL( F, G, H, A, B, C, D, E, 35, 0x53380d13 );
	UNROLL( E, F, G, H, A, B, C, D, 36, 0x650a7354 );
	UNROLL( D, E, F, G, H, A, B, C, 37, 0x766a0abb );
	UNROLL( C, D, E, F, G, H, A, B, 38, 0x81c2c92e );
	UNROLL( B, C, D, E, F, G, H, A, 39, 0x92722c85 );
	UNROLL( A, B, C, D, E, F, G, H, 40, 0xa2bfe8a1 );
	UNROLL( H, A, B, C, D, E, F, G, 41, 0xa81a664b );
	UNROLL( G, H, A, B, C, D, E, F, 42, 0xc24b8b70 );
	UNROLL( F, G, H, A, B, C, D, E, 43, 0xc76c51a3 );
	UNROLL( E, F, G, H, A, B, C, D, 44, 0xd192e819 );
	UNROLL( D, E, F, G, H, A, B, C, 45, 0xd6990624 );
	UNROLL( C, D, E, F, G, H, A, B, 46, 0xf40e3585 );
	UNROLL( B, C, D, E, F, G, H, A, 47, 0x106aa070 );
	UNROLL( A, B, C, D, E, F, G, H, 48, 0x19a4c116 );
	UNROLL( H, A, B, C, D, E, F, G, 49, 0x1e376c08 );
	UNROLL( G, H, A, B, C, D, E, F, 50, 0x2748774c );
	UNROLL( F, G, H, A, B, C, D, E, 51, 0x34b0bcb5 );
	UNROLL( E, F, G, H, A, B, C, D, 52, 0x391c0cb3 );
	UNROLL( D, E, F, G, H, A, B, C, 53, 0x4ed8aa4a );
	UNROLL( C, D, E, F, G, H, A, B, 54, 0x5b9cca4f );
	UNROLL( B, C, D, E, F, G, H, A, 55, 0x682e6ff3 );
	UNROLL( A, B, C, D, E, F, G, H, 56, 0x748f82ee );
	UNROLL( H, A, B, C, D, E, F, G, 57, 0x78a5636f );
	UNROLL( G, H, A, B, C, D, E, F, 58, 0x84c87814 );
	UNROLL( F, G, H, A, B, C, D, E, 59, 0x8cc70208 );
	UNROLL( E, F, G, H, A, B, C, D, 60, 0x90befffa );
	UNROLL( D, E, F, G, H, A, B, C, 61, 0xa4506ceb );
	UNROLL( C, D, E, F, G, H, A, B, 62, 0xbef9a3f7 );
	UNROLL( B, C, D, E, F, G, H, A, 63, 0xc67178f2 );
#endif

	context->h[0] += A;
	context->h[1] += B;
	context->h[2] += C;
	context->h[3] += D;
	context->h[4] += E;
	context->h[5] += F;
	context->h[6] += G;
	context->h[7] += H;

	context->index = 0;
}

/* Convenience function to convert a digest to a string. The str
 * argument must be at least (SHA256_DIGEST_SIZE * 2 + 1) = 65 bytes.
 */
char *sha256str(const uint8_t *digest, char *str)
{
	char *p = str;

	for (int i = 0; i < SHA256_DIGEST_SIZE * 2; i += 2, ++digest) {
		*p++ = tohex[(*digest >> 4) & 0xf];
		*p++ = tohex[*digest & 0xf];
	}
	*p++ = 0;

	return str;
}

/* Compute a sha256 message digest. The digest arg should be
 * SHA256_DIGEST_SIZE.
 */
void sha256(const void *data, int len, uint8_t *digest)
{
	sha256ctx ctx;

	sha256_init(&ctx);
	sha256_update(&ctx, data, len);
	sha256_final(&ctx, digest);
}

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

#include <stdio.h>
#include <string.h>
#include <errno.h>
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

/* Define the SHA shift, rotate left and rotate right macro */
#define SHA256_SHR(bits,word)      ((word) >> (bits))
#define SHA256_ROTL(bits,word)						\
	(((word) << (bits)) | ((word) >> (32-(bits))))
#define SHA256_ROTR(bits,word)						\
	(((word) >> (bits)) | ((word) << (32-(bits))))

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
 *   context: [in/out]
 *     The context to reset.
 *
 * Returns:
 *   sha Error Code.
 */
int sha256_init(sha256ctx *ctx)
{
	/* Initial Hash Values: FIPS-180-2 section 5.3.2 */
	static uint32_t SHA256_H0[SHA256_DIGEST_SIZE/4] = {
		0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
		0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
	};

	if (!ctx)
		return EINVAL;

	memset(ctx, 0, sizeof(sha256ctx));
	memcpy(ctx->h, SHA256_H0, sizeof(SHA256_H0));
	return 0;
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
 *
 * Returns:
 *   sha Error Code.
 */
int sha256_update(sha256ctx *context, const uint8_t *message_array,
				  unsigned int length)
{
	if (!length)
		return 0;

	if (!context || !message_array)
		return EINVAL;

	context->len += length;

	while (length--) {
		context->block[context->index++] = *message_array & 0xFF;
		if (context->index == SHA256_Message_Block_Size)
			SHA256ProcessMessageBlock(context);

		message_array++;
	}

	return 0;
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
 *
 * Returns:
 *   sha Error Code.
 */
int sha256_final(sha256ctx *ctx, uint8_t *digest)
{
	int i;

	if (!ctx || !digest)
		return EINVAL;

	SHA256PadMessage(ctx);

	for (i = 0; i < SHA256_DIGEST_SIZE; ++i)
		digest[i] = (uint8_t)(ctx->h[i>>2] >> 8 * (3 - (i & 0x03)));

	memset(ctx, 0, sizeof(sha256ctx)); /* zeroize */

	return 0;
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
	int        t, t4;                   /* Loop counter */
	uint32_t   temp1, temp2;            /* Temporary word value */
	uint32_t   W[64];                   /* Word sequence */
	uint32_t   A, B, C, D, E, F, G, H;  /* Word buffers */

	/*
	 * Initialize the first 16 words in the array W
	 */
	for (t = t4 = 0; t < 16; t++, t4 += 4)
		W[t] = (((uint32_t)context->block[t4]) << 24) |
			(((uint32_t)context->block[t4 + 1]) << 16) |
			(((uint32_t)context->block[t4 + 2]) << 8) |
			(((uint32_t)context->block[t4 + 3]));

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

char *sha256str(uint8_t *digest, char *str)
{
	int i;

	for (i = 0; i < SHA256_DIGEST_SIZE * 2; i += 2, ++digest)
		sprintf(str + i, "%02x", *digest);

	return str;
}

int sha256(const void *data, int len, uint8_t *digest)
{
	sha256ctx ctx;
	int rc;

	sha256_init(&ctx);
	rc = sha256_update(&ctx, data, len);
	if (rc == 0)
		rc = sha256_final(&ctx, digest);
	return rc;
}

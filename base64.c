#include <string.h>
#include "samlib.h"

/* Based on RFC 4648. Encodes using standard or base64url but can decode base64url
 * and standard.
 */

/* Only multi-thread safe if left at one setting */
int base64url_safe;

static char alphabet[] = {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789"
	"-_"
};

static const uint8_t reverse[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x3e, 0xff, 0x3e, 0xff, 0x3f,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0x3f,
	0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
	0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static const char pad = '=';

/* encode 3 bytes into 4 bytes
 *  < 6 | 2 > < 4 | 4 > < 2 | 6 >
 */
static void encode_block(char *dst, const uint8_t *src)
{
	*dst++ = alphabet[(src[0] >> 2) & 0x3f];
	*dst++ = alphabet[((src[0] & 3) << 4) | ((src[1] >> 4) & 0x3f)];
	*dst++ = alphabet[((src[1] << 2) & 0x3c) | ((src[2] >> 6) & 3)];
	*dst++ = alphabet[src[2] & 0x3f];
}

/* dst is returned null terminated but the return does not include the null. */
int base64_encode(char *dst, int dlen, const uint8_t *src, int len)
{
	int cnt = 0;

	if (base64_encoded_len(len) >= dlen)
		return -1;

	if (base64url_safe) {
		alphabet[62] = '-';
		alphabet[63] = '_';
	} else {
		alphabet[62] = '+';
		alphabet[63] = '/';
	}

	while (len >= 3) {
		encode_block(dst, src);
		dst += 4;
		src += 3;
		len -= 3;
		cnt += 4;
	}
	if (len > 0) {
		uint8_t block[3];
		memset(block, 0, 3);
		memcpy(block, src, len);
		encode_block(dst, block);
		dst[3] = pad;
		if (len == 1) dst[2] = pad;
		dst += 4;
		cnt += 4;
	}

	*dst = '\0';
	return cnt;
}

/* Note: the len includes the NULL */
int base64_encoded_len(int len)
{
	return (len + 2) / 3 * 4;
}

/* decode 4 bytes into 3
 * < 6 > < 2 | 4 > < 4 | 2 > < 6 >
 */
static int decode_block(uint8_t *dst, const char *src)
{
	uint8_t block[4];
	int i;

	for (i = 0; i < 4; ++i, ++src) {
		if (*src == pad)
			break;
		block[i] = reverse[*src & 0x7f];
		if (block[i] == 0xff)
			return -1;
	}

	/* For legal input, i > 1 */
	*dst++ = ((block[0] << 2) & 0xfc) | ((block[1] >> 4) & 3);
	if (i > 2)
		*dst++ = ((block[1] << 4) & 0xf0) | ((block[2] >> 2) & 0xf);
	if (i > 3)
		*dst++ = ((block[2] << 6) & 0xc0) | (block[3] & 0x3f);
	return i - 1;
}

/* Returns 0 on decode error. Some legacy code assumes decode can never fail. */
int base64_decode(uint8_t *dst, int dlen, const char *src, int len)
{
	int n, cnt = 0;

	if (base64_decoded_len(len) > dlen)
		return -1;

	while (len >= 4) {
		if ((n = decode_block(dst, src)) == -1)
			return 0; /* invalid input */
		dst += n;
		cnt += n;
		src += 4;
		len -= 4;
	}

	return cnt;
}

int base64_decoded_len(int len)
{
	return len / 4 * 3;
}

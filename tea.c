#include "samlib.h"

/* Notes:
 * - Unrolling the loops does not help.
 * - HW assisted AES is much faster :(
 */

#define TEA_CONSTANT 0x9e3779b9u /* key schedule constant */

static void encrypt_block(const uint32_t *k, uint32_t *v)
{
	uint32_t sum = 0;
	uint32_t v0 = v[0], v1 = v[1];

	for (int i = 0; i < 32; i++) {
		sum += TEA_CONSTANT;
		v0 += ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
		v1 += ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
	}

	v[0] = v0; v[1] = v1;
}

void tea_encrypt(const void *key, void *data, int len)
{
	while (len >= TEA_BAG_SIZE) {
		encrypt_block(key, data);
		data += TEA_BAG_SIZE;
		len -= TEA_BAG_SIZE;
	}

	if (len > 0) {
		uint32_t v[2];
		memset(v, 0, TEA_BAG_SIZE);
		memcpy(v, data, len);
		encrypt_block((uint32_t *)key, v);
		memcpy(data, v, TEA_BAG_SIZE);
	}
}

int tea_bag_size(int len)
{
	int mod = len & (TEA_BAG_SIZE - 1);
	if (mod)
		len += TEA_BAG_SIZE - mod;
	return len;
}

static void decrypt_block(const uint32_t *k, uint32_t *v)
{
	uint32_t sum = 0xC6EF3720;
	uint32_t v0 = v[0], v1 = v[1];

	for (int i = 0; i < 32; i++) {
		v1 -= ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
		v0 -= ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
		sum -= TEA_CONSTANT;
	}

	v[0] = v0; v[1] = v1;
}

/* If out is NULL, decrypt in place */
void tea_decrypt(const void *key, void *data, int len)
{
	while (len >= TEA_BAG_SIZE) {
		decrypt_block(key, data);
		data += TEA_BAG_SIZE;
		len -= TEA_BAG_SIZE;
	}
}

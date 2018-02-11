#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../samlib.h"

// prints string as hex
static void phex(uint8_t* str)
{
    unsigned char i;
    for(i = 0; i < 16; ++i)
        printf("%.2x", str[i]);
    printf("\n");
}

void out_msg(const char *msg, int rc, int hw)
{
#ifdef TESTALL
	if (rc)
#endif
	{
		if (hw)
			printf("HW ");
		printf("ECB %s: %s!\n", msg, rc ? "FAILURE" : "SUCCESS");
	}
}

static int test_encrypt_ecb_malloc(void)
{
	// 128bit key
	uint8_t key[16] = {
0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
	};
	// 512bit text
	uint8_t plain_text[64] = {
0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10
	};
	uint8_t cipher_text[64] = {
0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97,
0xf5, 0xd3, 0xd5, 0x85, 0x03, 0xb9, 0x69, 0x9d, 0xe7, 0x85, 0x89, 0x5a, 0x96, 0xfd, 0xba, 0xaf,
0x43, 0xb1, 0xcd, 0x7f, 0x59, 0x8e, 0xce, 0x23, 0x88, 0x1b, 0x00, 0xe3, 0xed, 0x03, 0x06, 0x88,
0x7b, 0x0c, 0x78, 0x5e, 0x27, 0xe8, 0xad, 0x3f, 0x82, 0x23, 0x20, 0x71, 0x04, 0x72, 0x5d, 0xd4
	};

	uint8_t i, buf[64], buf2[64];
	aes128_ctx *ctx;
	int rc = 0;

    memset(buf, 0, 64);
	memset(buf2, 0, 64);

	ctx = malloc(sizeof(aes128_ctx));
	if (!ctx) {
		puts("OOM!");
		return 1;
	}

	AES128_init_ctx(ctx, key, 1);

	for(i = 0; i < 4; ++i)
	{
		AES128_ECB_encrypt(ctx, plain_text + (i*16), buf+(i*16));
		if (memcmp(buf + i * 16, cipher_text + i * 16, 16)) {
			phex(buf + i * 16);
			phex(cipher_text + i * 16);
			rc = 1;
		}
	}

	out_msg("encrypt malloc", rc, ctx->have_hw);
	free(ctx);

	return rc;
}


static int test_encrypt_ecb(void)
{
  uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
  uint8_t in[]  = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
  uint8_t out[] = {0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};
  uint8_t buffer[16];
  aes128_ctx ctx;
  int rc;

  AES128_init_ctx(&ctx, key, 1);
  AES128_ECB_encrypt(&ctx, in, buffer);

  rc = strncmp((char*)out, (char*) buffer, 16);
  out_msg("encrypt", rc, ctx.have_hw);
  if (rc) return rc;

  AES128_ECB_encrypt_buffer(in, key, buffer, 16);
  rc = strncmp((char*)out, (char*) buffer, 16);
  out_msg("encrypt buffer", rc, ctx.have_hw);

  return rc;
}

static int test_decrypt_ecb(void)
{
  uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
  uint8_t in[]  = {0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};
  uint8_t out[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
  uint8_t buffer[16];
  aes128_ctx ctx;
  int rc;

  AES128_init_ctx(&ctx, key, 0);

  AES128_ECB_decrypt(&ctx, in, buffer);

  rc = strncmp((char*) out, (char*) buffer, 16);
  out_msg("decrypt", rc, ctx.have_hw);
  if (rc) return rc;

  AES128_ECB_decrypt_buffer(in, key, buffer, 16);

  rc = strncmp((char*) out, (char*) buffer, 16);
  out_msg("decrypt buffer", rc, ctx.have_hw);

  return rc;
}

#ifdef TESTALL
int aes_main(void)
#else
int main(void)
#endif
{
	int rc = 0;

	rc |= test_decrypt_ecb();
	rc |= test_encrypt_ecb();
	rc |= test_encrypt_ecb_malloc();

	return rc;
}

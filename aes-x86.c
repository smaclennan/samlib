#include <wmmintrin.h>

/* Originally from:
 * https://www.intel.com/content/dam/doc/white-paper/advanced-encryption-standard-new-instructions-set-paper.pdf
 */

static int hw_aes_support = -1;

static int cpu_supports_aes(void)
{
	if (hw_aes_support == -1) {
		uint32_t regs[4];
		cpuid(1, regs);
		hw_aes_support = !!(regs[2] & (1 << 25)); /* bit 25 is aes */
	}

	return hw_aes_support;
}

static inline __m128i AES_128_ASSIST (__m128i temp1, __m128i temp2)
{
	__m128i temp3;
	temp2 = _mm_shuffle_epi32 (temp2 ,0xff);
	temp3 = _mm_slli_si128 (temp1, 0x4);
	temp1 = _mm_xor_si128 (temp1, temp3);
	temp3 = _mm_slli_si128 (temp3, 0x4);
	temp1 = _mm_xor_si128 (temp1, temp3);
	temp3 = _mm_slli_si128 (temp3, 0x4);
	temp1 = _mm_xor_si128 (temp1, temp3);
	temp1 = _mm_xor_si128 (temp1, temp2);
	return temp1;
}

static void AES_128_Key_Expansion (const unsigned char *userkey,
								   unsigned char *key)
{
	__m128i temp1, temp2;
	__m128i *Key_Schedule = (__m128i*)key;

	temp1 = _mm_loadu_si128((__m128i*)userkey);
	Key_Schedule[0] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1 ,0x1);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[1] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x2);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[2] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x4);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[3] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x8);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[4] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x10);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[5] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x20);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[6] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x40);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[7] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x80);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[8] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x1b);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[9] = temp1;
	temp2 = _mm_aeskeygenassist_si128 (temp1,0x36);
	temp1 = AES_128_ASSIST(temp1, temp2);
	Key_Schedule[10] = temp1;
}

static inline void KEY_256_ASSIST_1(__m128i* temp1, __m128i * temp2)
{
	__m128i temp4;
	*temp2 = _mm_shuffle_epi32(*temp2, 0xff);
	temp4 = _mm_slli_si128 (*temp1, 0x4);
	*temp1 = _mm_xor_si128 (*temp1, temp4);
	temp4 = _mm_slli_si128 (temp4, 0x4);
	*temp1 = _mm_xor_si128 (*temp1, temp4);
	temp4 = _mm_slli_si128 (temp4, 0x4);
	*temp1 = _mm_xor_si128 (*temp1, temp4);
	*temp1 = _mm_xor_si128 (*temp1, *temp2);
}

static inline void KEY_256_ASSIST_2(__m128i* temp1, __m128i * temp3)
{
	__m128i temp2,temp4;
	temp4 = _mm_aeskeygenassist_si128 (*temp1, 0x0);
	temp2 = _mm_shuffle_epi32(temp4, 0xaa);
	temp4 = _mm_slli_si128 (*temp3, 0x4);
	*temp3 = _mm_xor_si128 (*temp3, temp4);
	temp4 = _mm_slli_si128 (temp4, 0x4);
	*temp3 = _mm_xor_si128 (*temp3, temp4);
	temp4 = _mm_slli_si128 (temp4, 0x4);
	*temp3 = _mm_xor_si128 (*temp3, temp4);
	*temp3 = _mm_xor_si128 (*temp3, temp2);
}

static void AES_256_Key_Expansion (const unsigned char *userkey,
								   unsigned char *key)
{
	__m128i temp1, temp2, temp3;
	__m128i *Key_Schedule = (__m128i*)key;

	temp1 = _mm_loadu_si128((__m128i*)userkey);
	temp3 = _mm_loadu_si128((__m128i*)(userkey+16));
	Key_Schedule[0] = temp1;
	Key_Schedule[1] = temp3;
	temp2 = _mm_aeskeygenassist_si128 (temp3,0x01);
	KEY_256_ASSIST_1(&temp1, &temp2);
	Key_Schedule[2]=temp1;
	KEY_256_ASSIST_2(&temp1, &temp3);
	Key_Schedule[3]=temp3;
	temp2 = _mm_aeskeygenassist_si128 (temp3,0x02);
	KEY_256_ASSIST_1(&temp1, &temp2);
	Key_Schedule[4]=temp1;
	KEY_256_ASSIST_2(&temp1, &temp3);
	Key_Schedule[5]=temp3;
	temp2 = _mm_aeskeygenassist_si128 (temp3,0x04);
	KEY_256_ASSIST_1(&temp1, &temp2);
	Key_Schedule[6]=temp1;
	KEY_256_ASSIST_2(&temp1, &temp3);
	Key_Schedule[7]=temp3;
	temp2 = _mm_aeskeygenassist_si128 (temp3,0x08);
	KEY_256_ASSIST_1(&temp1, &temp2);
	Key_Schedule[8]=temp1;
	KEY_256_ASSIST_2(&temp1, &temp3);
	Key_Schedule[9]=temp3;
	temp2 = _mm_aeskeygenassist_si128 (temp3,0x10);
	KEY_256_ASSIST_1(&temp1, &temp2);
	Key_Schedule[10]=temp1;
	KEY_256_ASSIST_2(&temp1, &temp3);
	Key_Schedule[11]=temp3;
	temp2 = _mm_aeskeygenassist_si128 (temp3,0x20);
	KEY_256_ASSIST_1(&temp1, &temp2);
	Key_Schedule[12]=temp1;
	KEY_256_ASSIST_2(&temp1, &temp3);
	Key_Schedule[13]=temp3;
	temp2 = _mm_aeskeygenassist_si128 (temp3,0x40);
	KEY_256_ASSIST_1(&temp1, &temp2);
	Key_Schedule[14]=temp1;
}


static void AES128_expand_key(void *key, int nr)
{
	ALIGN16 unsigned char temp_key[240];
	__m128i *Key_Schedule = (__m128i *)key;
	__m128i *Temp_Key_Schedule = (__m128i *)temp_key;

	memcpy(temp_key, key, sizeof(temp_key));

	Key_Schedule[nr] = Temp_Key_Schedule[0];
	Key_Schedule[nr-1] = _mm_aesimc_si128(Temp_Key_Schedule[1]);
	Key_Schedule[nr-2] = _mm_aesimc_si128(Temp_Key_Schedule[2]);
	Key_Schedule[nr-3] = _mm_aesimc_si128(Temp_Key_Schedule[3]);
	Key_Schedule[nr-4] = _mm_aesimc_si128(Temp_Key_Schedule[4]);
	Key_Schedule[nr-5] = _mm_aesimc_si128(Temp_Key_Schedule[5]);
	Key_Schedule[nr-6] = _mm_aesimc_si128(Temp_Key_Schedule[6]);
	Key_Schedule[nr-7] = _mm_aesimc_si128(Temp_Key_Schedule[7]);
	Key_Schedule[nr-8] = _mm_aesimc_si128(Temp_Key_Schedule[8]);
	Key_Schedule[nr-9] = _mm_aesimc_si128(Temp_Key_Schedule[9]);
	if(nr > 10) {
		Key_Schedule[nr-10] = _mm_aesimc_si128(Temp_Key_Schedule[10]);
		Key_Schedule[nr-11] = _mm_aesimc_si128(Temp_Key_Schedule[11]);
		Key_Schedule[nr-12] = _mm_aesimc_si128(Temp_Key_Schedule[12]);
		Key_Schedule[nr-13] = _mm_aesimc_si128(Temp_Key_Schedule[13]);
	}
	Key_Schedule[0] = Temp_Key_Schedule[nr];
}

/* Public functions */

static int aes_set_hw(int hw)
{
	switch (hw) {
	case 0:
	case -1:
		hw_aes_support = hw;
		return 0;
	case 1:
		hw_aes_support = -1;
		cpu_supports_aes();
		return hw_aes_support == 1 ? 0 : ENOSYS;
	default:
		return EINVAL;
	}
}

static int AES_expand_key(const void *userkey, void *key, int bits, int encrypt)
{
	int nr;

	switch (bits) {
	case 256:
		AES_256_Key_Expansion(userkey, key);
		nr = 14;
		break;
	case 128:
		AES_128_Key_Expansion(userkey, key);
		nr = 10;
		break;
	default:
		return EINVAL;
	}

	if (encrypt == 0)
		AES128_expand_key(key, nr);

	return 0;
}

#ifdef ECB
static void AES128_ECB_encrypt_block(const void *in,
									 void *out,
									 const void *key)
{
	__m128i tmp, *key128 = (__m128i *)key;

	tmp = _mm_loadu_si128((__m128i *)in);
	tmp = _mm_xor_si128(tmp, key128[0]);
	tmp = _mm_aesenc_si128(tmp, key128[1]);
	tmp = _mm_aesenc_si128(tmp, key128[2]);
	tmp = _mm_aesenc_si128(tmp, key128[3]);
	tmp = _mm_aesenc_si128(tmp, key128[4]);
	tmp = _mm_aesenc_si128(tmp, key128[5]);
	tmp = _mm_aesenc_si128(tmp, key128[6]);
	tmp = _mm_aesenc_si128(tmp, key128[7]);
	tmp = _mm_aesenc_si128(tmp, key128[8]);
	tmp = _mm_aesenc_si128(tmp, key128[9]);
	tmp = _mm_aesenclast_si128(tmp, key128[10]);
	_mm_storeu_si128((__m128i *)out, tmp);
}

static void AES128_ECB_decrypt_block(const void *in,
									 void *out,
									 const void *key)
{
	__m128i tmp, *key128 = (__m128i *)key;

	tmp = _mm_loadu_si128((__m128i *)in);
	tmp = _mm_xor_si128(tmp, key128[0]);
	tmp = _mm_aesdec_si128(tmp, key128[1]);
	tmp = _mm_aesdec_si128(tmp, key128[2]);
	tmp = _mm_aesdec_si128(tmp, key128[3]);
	tmp = _mm_aesdec_si128(tmp, key128[4]);
	tmp = _mm_aesdec_si128(tmp, key128[5]);
	tmp = _mm_aesdec_si128(tmp, key128[6]);
	tmp = _mm_aesdec_si128(tmp, key128[7]);
	tmp = _mm_aesdec_si128(tmp, key128[8]);
	tmp = _mm_aesdec_si128(tmp, key128[9]);
	tmp = _mm_aesdeclast_si128(tmp, key128[10]);
	_mm_storeu_si128 ((__m128i *)out, tmp);
}
#endif

#ifdef CBC
static void AES_CBC_encrypt_blocks(aes_cbc_ctx *ctx, const void *in, void *out, unsigned long blocks)
{
	__m128i feedback, data;
	int i, j;

	feedback = _mm_loadu_si128((__m128i*)ctx->ivec);
	for (i = 0; i < blocks; i++) {
		data = _mm_loadu_si128 (&((__m128i*)in)[i]);
		feedback = _mm_xor_si128(data, feedback);
		feedback = _mm_xor_si128(feedback, ((__m128i*)ctx->roundkey)[0]);
		for(j = 1; j < ctx->Nr; j++)
			feedback = _mm_aesenc_si128(feedback, ((__m128i*)ctx->roundkey)[j]);
		feedback = _mm_aesenclast_si128(feedback, ((__m128i*)ctx->roundkey)[j]);
		_mm_storeu_si128(&((__m128i*)out)[i], feedback);
	}

	/* Save for possible next iter */
	_mm_storeu_si128 ((__m128i*)ctx->ivec, feedback);
}

static void AES_CBC_decrypt_blocks(aes_cbc_ctx *ctx, const void *in, void *out, unsigned long blocks)
{
	__m128i data, last_in, feedback;
	int i, j;

	feedback = _mm_loadu_si128 ((__m128i*)ctx->ivec);
	for (i = 0; i < blocks; i++) {
		last_in =_mm_loadu_si128(&((__m128i*)in)[i]);
		data = _mm_xor_si128(last_in,((__m128i*)ctx->roundkey)[0]);
		for(j = 1; j < ctx->Nr; j++)
			data = _mm_aesdec_si128(data,((__m128i*)ctx->roundkey)[j]);
		data = _mm_aesdeclast_si128(data,((__m128i*)ctx->roundkey)[j]);
		data = _mm_xor_si128(data,feedback);
		_mm_storeu_si128 (&((__m128i*)out)[i],data);
		feedback = last_in;
	}

	/* Save for possible next iter */
	_mm_storeu_si128 ((__m128i*)ctx->ivec, feedback);
}
#endif

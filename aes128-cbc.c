/*

This is an implementation of the AES128 algorithm, specifically CBC mode.

The implementation is verified against the test vectors in:
  National Institute of Standards and Technology Special Publication 800-38A 2001 ED

NOTE:   String length must be evenly divisible by 16byte (str_len % 16 == 0)
        You should pad the end of the string with zeros if this is not the case.

*/

/*****************************************************************************/
/* Includes:                                                                 */
/*****************************************************************************/

#include "samlib.h"

/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/
// The number of columns comprising a state in AES. This is a constant in AES. Value=4
#define Nb 4
// The number of 32 bit words in a key.
#define Nk 4
// The number of rounds in AES Cipher.
#define Nr 10

/* It is always safe to enable AES_HW on x86 systems. It will only be
 * used if the CPU supports it.
 */
#if defined(__x86_64__) || defined(_M_X64)
#define AES_HW 1
#endif

typedef uint8_t state_t[4][4];

/*****************************************************************************/
/* Private functions:                                                        */
/*****************************************************************************/

/* From aes128.c */
extern const unsigned char sbox[256];
extern const unsigned char rsbox[256];

// This function adds the round key to state.
// The round key is added to the state by an XOR function.
static void AddRoundKey(aes128_ctx *ctx, uint8_t round, state_t *state)
{
  uint8_t i,j;
  for(i=0;i<4;++i)
	for(j = 0; j < 4; ++j)
	  (*state)[i][j] ^= ctx->roundkey[round * Nb * 4 + i * Nb + j];
}

// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void SubBytes(state_t *state)
{
  uint8_t i, j;
  for(i = 0; i < 4; ++i)
	for(j = 0; j < 4; ++j)
	  (*state)[j][i] = sbox[(*state)[j][i]];
}

// The ShiftRows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static void ShiftRows(state_t *state)
{
  uint8_t temp;

  // Rotate first row 1 columns to left
  temp           = (*state)[0][1];
  (*state)[0][1] = (*state)[1][1];
  (*state)[1][1] = (*state)[2][1];
  (*state)[2][1] = (*state)[3][1];
  (*state)[3][1] = temp;

  // Rotate second row 2 columns to left
  temp           = (*state)[0][2];
  (*state)[0][2] = (*state)[2][2];
  (*state)[2][2] = temp;

  temp       = (*state)[1][2];
  (*state)[1][2] = (*state)[3][2];
  (*state)[3][2] = temp;

  // Rotate third row 3 columns to left
  temp       = (*state)[0][3];
  (*state)[0][3] = (*state)[3][3];
  (*state)[3][3] = (*state)[2][3];
  (*state)[2][3] = (*state)[1][3];
  (*state)[1][3] = temp;
}

static uint8_t xtime(uint8_t x)
{
  return ((x<<1) ^ (((x>>7) & 1) * 0x1b));
}

// MixColumns function mixes the columns of the state matrix
static void MixColumns(state_t *state)
{
  uint8_t i;
  uint8_t Tmp,Tm,t;
  for(i = 0; i < 4; ++i)
  {
	t   = (*state)[i][0];
	Tmp = (*state)[i][0] ^ (*state)[i][1] ^ (*state)[i][2] ^ (*state)[i][3] ;
	Tm  = (*state)[i][0] ^ (*state)[i][1] ; Tm = xtime(Tm);  (*state)[i][0] ^= Tm ^ Tmp ;
	Tm  = (*state)[i][1] ^ (*state)[i][2] ; Tm = xtime(Tm);  (*state)[i][1] ^= Tm ^ Tmp ;
	Tm  = (*state)[i][2] ^ (*state)[i][3] ; Tm = xtime(Tm);  (*state)[i][2] ^= Tm ^ Tmp ;
	Tm  = (*state)[i][3] ^ t ;        Tm = xtime(Tm);  (*state)[i][3] ^= Tm ^ Tmp ;
  }
}

// Multiply is used to multiply numbers in the field GF(2^8)
#define Multiply(x, y)                                \
	  (  ((y & 1) * x) ^                              \
	  ((y>>1 & 1) * xtime(x)) ^                       \
	  ((y>>2 & 1) * xtime(xtime(x))) ^                \
	  ((y>>3 & 1) * xtime(xtime(xtime(x)))) ^         \
	  ((y>>4 & 1) * xtime(xtime(xtime(xtime(x))))))   \


// MixColumns function mixes the columns of the state matrix.
// The method used to multiply may be difficult to understand for the inexperienced.
// Please use the references to gain more information.
static void InvMixColumns(state_t *state)
{
  int i;
  uint8_t a,b,c,d;
  for(i=0;i<4;++i)
  {
	a = (*state)[i][0];
	b = (*state)[i][1];
	c = (*state)[i][2];
	d = (*state)[i][3];

	(*state)[i][0] = Multiply(a, 0x0e) ^ Multiply(b, 0x0b) ^ Multiply(c, 0x0d) ^ Multiply(d, 0x09);
	(*state)[i][1] = Multiply(a, 0x09) ^ Multiply(b, 0x0e) ^ Multiply(c, 0x0b) ^ Multiply(d, 0x0d);
	(*state)[i][2] = Multiply(a, 0x0d) ^ Multiply(b, 0x09) ^ Multiply(c, 0x0e) ^ Multiply(d, 0x0b);
	(*state)[i][3] = Multiply(a, 0x0b) ^ Multiply(b, 0x0d) ^ Multiply(c, 0x09) ^ Multiply(d, 0x0e);
  }
}


// The SubBytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void InvSubBytes(state_t *state)
{
  uint8_t i,j;
  for(i=0;i<4;++i)
  {
	for(j=0;j<4;++j)
	{
	  (*state)[j][i] = rsbox[(*state)[j][i]];
	}
  }
}

static void InvShiftRows(state_t *state)
{
  uint8_t temp;

  // Rotate first row 1 columns to right
  temp=(*state)[3][1];
  (*state)[3][1]=(*state)[2][1];
  (*state)[2][1]=(*state)[1][1];
  (*state)[1][1]=(*state)[0][1];
  (*state)[0][1]=temp;

  // Rotate second row 2 columns to right
  temp=(*state)[0][2];
  (*state)[0][2]=(*state)[2][2];
  (*state)[2][2]=temp;

  temp=(*state)[1][2];
  (*state)[1][2]=(*state)[3][2];
  (*state)[3][2]=temp;

  // Rotate third row 3 columns to right
  temp=(*state)[0][3];
  (*state)[0][3]=(*state)[1][3];
  (*state)[1][3]=(*state)[2][3];
  (*state)[2][3]=(*state)[3][3];
  (*state)[3][3]=temp;
}


// Cipher is the main function that encrypts the PlainText.
static void Cipher(aes128_ctx *ctx, state_t *state)
{
  uint8_t round = 0;

  // Add the First round key to the state before starting the rounds.
  AddRoundKey(ctx, 0, state);

  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
  for(round = 1; round < Nr; ++round)
  {
	SubBytes(state);
	ShiftRows(state);
	MixColumns(state);
	AddRoundKey(ctx, round, state);
  }

  // The last round is given below.
  // The MixColumns function is not here in the last round.
  SubBytes(state);
  ShiftRows(state);
  AddRoundKey(ctx, Nr, state);
}

static void InvCipher(aes128_ctx *ctx, state_t *state)
{
  uint8_t round=0;

  // Add the First round key to the state before starting the rounds.
  AddRoundKey(ctx, Nr, state);

  // There will be Nr rounds.
  // The first Nr-1 rounds are identical.
  // These Nr-1 rounds are executed in the loop below.
  for(round=Nr-1;round>0;round--)
  {
	InvShiftRows(state);
	InvSubBytes(state);
	AddRoundKey(ctx, round, state);
	InvMixColumns(state);
  }

  // The last round is given below.
  // The MixColumns function is not here in the last round.
  InvShiftRows(state);
  InvSubBytes(state);
  AddRoundKey(ctx, 0, state);
}

static inline void BlockCopy(void *output, const void *input)
{
	memcpy(output, input, AES128_KEYLEN);
}

/*****************************************************************************/
/* AES HW functions:                                                         */
/*****************************************************************************/

#if AES_HW
#include <wmmintrin.h>

/* Originally from:
 * https://www.intel.com/content/dam/doc/white-paper/advanced-encryption-standard-new-instructions-set-paper.pdf
 */

static void AES_CBC_encrypt(aes128_ctx *ctx, const void *in, void *out, unsigned long length)
{
	__m128i feedback, data;
	int i, j;

	length /= 16;

	feedback = _mm_loadu_si128((__m128i*)ctx->ivec);
	for(i=0; i < length; i++) {
		data = _mm_loadu_si128 (&((__m128i*)in)[i]);
		feedback = _mm_xor_si128 (data,feedback);
		feedback = _mm_xor_si128 (feedback, ((__m128i*)ctx->roundkey)[0]);
		for(j=1; j < Nr; j++)
			feedback = _mm_aesenc_si128(feedback, ((__m128i*)ctx->roundkey)[j]);
		feedback = _mm_aesenclast_si128(feedback, ((__m128i*)ctx->roundkey)[j]);
		_mm_storeu_si128(&((__m128i*)out)[i], feedback);
	}
}

static void AES_CBC_decrypt(aes128_ctx *ctx, const void *in, void *out, unsigned long length)
{
	__m128i data, last_in, feedback;
	int i, j;

	length /= 16;

	if (ctx->ivec)
		feedback = _mm_loadu_si128 ((__m128i*)ctx->ivec);
	for (i = 0; i < length; i++) {
		last_in =_mm_loadu_si128 (&((__m128i*)in)[i]);
		data = _mm_xor_si128 (last_in,((__m128i*)ctx->roundkey)[0]);
		for(j = 1; j < Nr; j++)
			data = _mm_aesdec_si128 (data,((__m128i*)ctx->roundkey)[j]);
		data = _mm_aesdeclast_si128 (data,((__m128i*)ctx->roundkey)[j]);
		data = _mm_xor_si128 (data,feedback);
		_mm_storeu_si128 (&((__m128i*)out)[i],data);
		feedback = last_in;
	}

	/* Save for possible next iter */
	_mm_storeu_si128 ((__m128i*)ctx->ivec, feedback);
}

#endif

static inline void XorWithIv(aes128_ctx *ctx, uint8_t *buf)
{
	for (int i = 0; i < AES128_KEYLEN; ++i)
		buf[i] ^= ctx->ivptr[i];
}

/*****************************************************************************/
/* Public functions:                                                         */
/*****************************************************************************/

int AES128_CBC_encrypt_buffer(aes128_ctx *ctx, void *output, const void *input, unsigned long length)
{
	if (length & (AES128_KEYLEN - 1))
		return EINVAL;

#if AES_HW
	if (ctx->have_hw) {
		AES_CBC_encrypt(ctx, input, output, length);
		return 0;
	}
#endif

	for (unsigned long i = 0; i < length; i += AES128_KEYLEN) {
		BlockCopy(output, input);
		XorWithIv(ctx, output);
		Cipher(ctx, (state_t *)output);
		ctx->ivptr = output;
		input += AES128_KEYLEN;
		output += AES128_KEYLEN;
	}

	return 0;
}

int AES128_CBC_decrypt_buffer(aes128_ctx *ctx, void *output, const void *input, unsigned long length)
{
	if (length & (AES128_KEYLEN - 1))
		return EINVAL;

#if AES_HW
	if (ctx->have_hw) {
		AES_CBC_decrypt(ctx, input, output, length);
		return 0;
	}
#endif

	for (unsigned long i = 0; i < length; i += AES128_KEYLEN) {
		BlockCopy(output, input);
		InvCipher(ctx, (state_t *)output);
		XorWithIv(ctx, output);
		ctx->ivptr = input;
		input += AES128_KEYLEN;
		output += AES128_KEYLEN;
	}

	return 0;
}

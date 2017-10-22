#include <string.h>
#include <errno.h>
#include "samlib.h"

/* It is always safe to enable AES_HW on x86 systems. It will only be
 * used if the CPU supports it.
 */
#ifndef AES_HW
#define AES_HW 1
#endif

/* TI_aes.c from:
https://www.element14.com/community/docs/DOC-17134/l/aes128-a-c-implementation-for-encryption-and-decryption-source-code
 */

/**************************************************************
						AES128
Author:   Uli Kretzschmar
			 MSP430 Systems
			 Freising
AES software support for encryption and decryption
ECCN 5D002 TSU - Technology / Software Unrestricted
**************************************************************/

// forward sbox
static const unsigned char sbox[256] =   {
//0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, //0
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //1
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, //2
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //3
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, //4
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //5
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, //6
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //7
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, //8
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //9
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, //A
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //B
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, //C
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //D
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, //E
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 }; //F

// inverse sbox
static const unsigned char rsbox[256] =
{ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb
, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb
, 0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e
, 0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25
, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92
, 0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84
, 0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06
, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b
, 0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73
, 0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e
, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b
, 0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4
, 0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f
, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef
, 0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61
, 0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d };

// round constant
static const unsigned char Rcon[11] = {
  0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};


// expand the key
static void expandKey(unsigned char *expandedKey, const unsigned char *key)
{
  unsigned short ii, buf1;
  for (ii=0;ii<16;ii++)
	expandedKey[ii] = key[ii];
  for (ii=1;ii<11;ii++){
	buf1 = expandedKey[ii*16 - 4];
	expandedKey[ii*16 + 0] = sbox[expandedKey[ii*16 - 3]]^expandedKey[(ii-1)*16 + 0]^Rcon[ii];
	expandedKey[ii*16 + 1] = sbox[expandedKey[ii*16 - 2]]^expandedKey[(ii-1)*16 + 1];
	expandedKey[ii*16 + 2] = sbox[expandedKey[ii*16 - 1]]^expandedKey[(ii-1)*16 + 2];
	expandedKey[ii*16 + 3] = sbox[buf1                  ]^expandedKey[(ii-1)*16 + 3];
	expandedKey[ii*16 + 4] = expandedKey[(ii-1)*16 + 4]^expandedKey[ii*16 + 0];
	expandedKey[ii*16 + 5] = expandedKey[(ii-1)*16 + 5]^expandedKey[ii*16 + 1];
	expandedKey[ii*16 + 6] = expandedKey[(ii-1)*16 + 6]^expandedKey[ii*16 + 2];
	expandedKey[ii*16 + 7] = expandedKey[(ii-1)*16 + 7]^expandedKey[ii*16 + 3];
	expandedKey[ii*16 + 8] = expandedKey[(ii-1)*16 + 8]^expandedKey[ii*16 + 4];
	expandedKey[ii*16 + 9] = expandedKey[(ii-1)*16 + 9]^expandedKey[ii*16 + 5];
	expandedKey[ii*16 +10] = expandedKey[(ii-1)*16 +10]^expandedKey[ii*16 + 6];
	expandedKey[ii*16 +11] = expandedKey[(ii-1)*16 +11]^expandedKey[ii*16 + 7];
	expandedKey[ii*16 +12] = expandedKey[(ii-1)*16 +12]^expandedKey[ii*16 + 8];
	expandedKey[ii*16 +13] = expandedKey[(ii-1)*16 +13]^expandedKey[ii*16 + 9];
	expandedKey[ii*16 +14] = expandedKey[(ii-1)*16 +14]^expandedKey[ii*16 +10];
	expandedKey[ii*16 +15] = expandedKey[(ii-1)*16 +15]^expandedKey[ii*16 +11];
  }
}

// multiply by 2 in the galois field
static unsigned char galois_mul2(unsigned char value)
{
	if (value>>7)
	{
		value = value << 1;
		return (value^0x1b);
	} else
		return value<<1;
}

// straight foreward aes encryption implementation
//   first the group of operations
//     - addroundkey
//     - subbytes
//     - shiftrows
//     - mixcolums
//   is executed 9 times, after this addroundkey to finish the 9th round,
//   after that the 10th round without mixcolums
//   no further subfunctions to save cycles for function calls
//   no structuring with "for (....)" to save cycles
static void aes_encr(unsigned char *state, unsigned char *expandedKey)
{
  unsigned char buf1, buf2, buf3, round;


  for (round = 0; round < 9; round ++){
	// addroundkey, sbox and shiftrows
	// row 0
	state[ 0]  = sbox[(state[ 0] ^ expandedKey[(round*16)     ])];
	state[ 4]  = sbox[(state[ 4] ^ expandedKey[(round*16) +  4])];
	state[ 8]  = sbox[(state[ 8] ^ expandedKey[(round*16) +  8])];
	state[12]  = sbox[(state[12] ^ expandedKey[(round*16) + 12])];
	// row 1
	buf1 = state[1] ^ expandedKey[(round*16) + 1];
	state[ 1]  = sbox[(state[ 5] ^ expandedKey[(round*16) +  5])];
	state[ 5]  = sbox[(state[ 9] ^ expandedKey[(round*16) +  9])];
	state[ 9]  = sbox[(state[13] ^ expandedKey[(round*16) + 13])];
	state[13]  = sbox[buf1];
	// row 2
	buf1 = state[2] ^ expandedKey[(round*16) + 2];
	buf2 = state[6] ^ expandedKey[(round*16) + 6];
	state[ 2]  = sbox[(state[10] ^ expandedKey[(round*16) + 10])];
	state[ 6]  = sbox[(state[14] ^ expandedKey[(round*16) + 14])];
	state[10]  = sbox[buf1];
	state[14]  = sbox[buf2];
	// row 3
	buf1 = state[15] ^ expandedKey[(round*16) + 15];
	state[15]  = sbox[(state[11] ^ expandedKey[(round*16) + 11])];
	state[11]  = sbox[(state[ 7] ^ expandedKey[(round*16) +  7])];
	state[ 7]  = sbox[(state[ 3] ^ expandedKey[(round*16) +  3])];
	state[ 3]  = sbox[buf1];

	// mixcolums //////////
	// col1
	buf1 = state[0] ^ state[1] ^ state[2] ^ state[3];
	buf2 = state[0];
	buf3 = state[0]^state[1]; buf3=galois_mul2(buf3); state[0] = state[0] ^ buf3 ^ buf1;
	buf3 = state[1]^state[2]; buf3=galois_mul2(buf3); state[1] = state[1] ^ buf3 ^ buf1;
	buf3 = state[2]^state[3]; buf3=galois_mul2(buf3); state[2] = state[2] ^ buf3 ^ buf1;
	buf3 = state[3]^buf2;     buf3=galois_mul2(buf3); state[3] = state[3] ^ buf3 ^ buf1;
	// col2
	buf1 = state[4] ^ state[5] ^ state[6] ^ state[7];
	buf2 = state[4];
	buf3 = state[4]^state[5]; buf3=galois_mul2(buf3); state[4] = state[4] ^ buf3 ^ buf1;
	buf3 = state[5]^state[6]; buf3=galois_mul2(buf3); state[5] = state[5] ^ buf3 ^ buf1;
	buf3 = state[6]^state[7]; buf3=galois_mul2(buf3); state[6] = state[6] ^ buf3 ^ buf1;
	buf3 = state[7]^buf2;     buf3=galois_mul2(buf3); state[7] = state[7] ^ buf3 ^ buf1;
	// col3
	buf1 = state[8] ^ state[9] ^ state[10] ^ state[11];
	buf2 = state[8];
	buf3 = state[8]^state[9];   buf3=galois_mul2(buf3); state[8] = state[8] ^ buf3 ^ buf1;
	buf3 = state[9]^state[10];  buf3=galois_mul2(buf3); state[9] = state[9] ^ buf3 ^ buf1;
	buf3 = state[10]^state[11]; buf3=galois_mul2(buf3); state[10] = state[10] ^ buf3 ^ buf1;
	buf3 = state[11]^buf2;      buf3=galois_mul2(buf3); state[11] = state[11] ^ buf3 ^ buf1;
	// col4
	buf1 = state[12] ^ state[13] ^ state[14] ^ state[15];
	buf2 = state[12];
	buf3 = state[12]^state[13]; buf3=galois_mul2(buf3); state[12] = state[12] ^ buf3 ^ buf1;
	buf3 = state[13]^state[14]; buf3=galois_mul2(buf3); state[13] = state[13] ^ buf3 ^ buf1;
	buf3 = state[14]^state[15]; buf3=galois_mul2(buf3); state[14] = state[14] ^ buf3 ^ buf1;
	buf3 = state[15]^buf2;      buf3=galois_mul2(buf3); state[15] = state[15] ^ buf3 ^ buf1;

  }
  // 10th round without mixcols
  state[ 0]  = sbox[(state[ 0] ^ expandedKey[(round*16)     ])];
  state[ 4]  = sbox[(state[ 4] ^ expandedKey[(round*16) +  4])];
  state[ 8]  = sbox[(state[ 8] ^ expandedKey[(round*16) +  8])];
  state[12]  = sbox[(state[12] ^ expandedKey[(round*16) + 12])];
  // row 1
  buf1 = state[1] ^ expandedKey[(round*16) + 1];
  state[ 1]  = sbox[(state[ 5] ^ expandedKey[(round*16) +  5])];
  state[ 5]  = sbox[(state[ 9] ^ expandedKey[(round*16) +  9])];
  state[ 9]  = sbox[(state[13] ^ expandedKey[(round*16) + 13])];
  state[13]  = sbox[buf1];
  // row 2
  buf1 = state[2] ^ expandedKey[(round*16) + 2];
  buf2 = state[6] ^ expandedKey[(round*16) + 6];
  state[ 2]  = sbox[(state[10] ^ expandedKey[(round*16) + 10])];
  state[ 6]  = sbox[(state[14] ^ expandedKey[(round*16) + 14])];
  state[10]  = sbox[buf1];
  state[14]  = sbox[buf2];
  // row 3
  buf1 = state[15] ^ expandedKey[(round*16) + 15];
  state[15]  = sbox[(state[11] ^ expandedKey[(round*16) + 11])];
  state[11]  = sbox[(state[ 7] ^ expandedKey[(round*16) +  7])];
  state[ 7]  = sbox[(state[ 3] ^ expandedKey[(round*16) +  3])];
  state[ 3]  = sbox[buf1];
  // last addroundkey
  state[ 0]^=expandedKey[160];
  state[ 1]^=expandedKey[161];
  state[ 2]^=expandedKey[162];
  state[ 3]^=expandedKey[163];
  state[ 4]^=expandedKey[164];
  state[ 5]^=expandedKey[165];
  state[ 6]^=expandedKey[166];
  state[ 7]^=expandedKey[167];
  state[ 8]^=expandedKey[168];
  state[ 9]^=expandedKey[169];
  state[10]^=expandedKey[170];
  state[11]^=expandedKey[171];
  state[12]^=expandedKey[172];
  state[13]^=expandedKey[173];
  state[14]^=expandedKey[174];
  state[15]^=expandedKey[175];
}

// straight foreward aes decryption implementation
//   the order of substeps is the exact reverse of decryption
//   inverse functions:
//       - addRoundKey is its own inverse
//       - rsbox is inverse of sbox
//       - rightshift instead of leftshift
//       - invMixColumns = barreto + mixColumns
//   no further subfunctions to save cycles for function calls
//   no structuring with "for (....)" to save cycles
static void aes_decr(unsigned char *state, unsigned char *expandedKey)
{
  unsigned char buf1, buf2, buf3;
  signed char round;
  round = 9;

  // initial addroundkey
  state[ 0]^=expandedKey[160];
  state[ 1]^=expandedKey[161];
  state[ 2]^=expandedKey[162];
  state[ 3]^=expandedKey[163];
  state[ 4]^=expandedKey[164];
  state[ 5]^=expandedKey[165];
  state[ 6]^=expandedKey[166];
  state[ 7]^=expandedKey[167];
  state[ 8]^=expandedKey[168];
  state[ 9]^=expandedKey[169];
  state[10]^=expandedKey[170];
  state[11]^=expandedKey[171];
  state[12]^=expandedKey[172];
  state[13]^=expandedKey[173];
  state[14]^=expandedKey[174];
  state[15]^=expandedKey[175];

  // 10th round without mixcols
  state[ 0]  = rsbox[state[ 0]] ^ expandedKey[(round*16)     ];
  state[ 4]  = rsbox[state[ 4]] ^ expandedKey[(round*16) +  4];
  state[ 8]  = rsbox[state[ 8]] ^ expandedKey[(round*16) +  8];
  state[12]  = rsbox[state[12]] ^ expandedKey[(round*16) + 12];
  // row 1
  buf1 =       rsbox[state[13]] ^ expandedKey[(round*16) +  1];
  state[13]  = rsbox[state[ 9]] ^ expandedKey[(round*16) + 13];
  state[ 9]  = rsbox[state[ 5]] ^ expandedKey[(round*16) +  9];
  state[ 5]  = rsbox[state[ 1]] ^ expandedKey[(round*16) +  5];
  state[ 1]  = buf1;
  // row 2
  buf1 =       rsbox[state[ 2]] ^ expandedKey[(round*16) + 10];
  buf2 =       rsbox[state[ 6]] ^ expandedKey[(round*16) + 14];
  state[ 2]  = rsbox[state[10]] ^ expandedKey[(round*16) +  2];
  state[ 6]  = rsbox[state[14]] ^ expandedKey[(round*16) +  6];
  state[10]  = buf1;
  state[14]  = buf2;
  // row 3
  buf1 =       rsbox[state[ 3]] ^ expandedKey[(round*16) + 15];
  state[ 3]  = rsbox[state[ 7]] ^ expandedKey[(round*16) +  3];
  state[ 7]  = rsbox[state[11]] ^ expandedKey[(round*16) +  7];
  state[11]  = rsbox[state[15]] ^ expandedKey[(round*16) + 11];
  state[15]  = buf1;

  for (round = 8; round >= 0; round--){
	// barreto
	//col1
	buf1 = galois_mul2(galois_mul2(state[0]^state[2]));
	buf2 = galois_mul2(galois_mul2(state[1]^state[3]));
	state[0] ^= buf1;     state[1] ^= buf2;    state[2] ^= buf1;    state[3] ^= buf2;
	//col2
	buf1 = galois_mul2(galois_mul2(state[4]^state[6]));
	buf2 = galois_mul2(galois_mul2(state[5]^state[7]));
	state[4] ^= buf1;    state[5] ^= buf2;    state[6] ^= buf1;    state[7] ^= buf2;
	//col3
	buf1 = galois_mul2(galois_mul2(state[8]^state[10]));
	buf2 = galois_mul2(galois_mul2(state[9]^state[11]));
	state[8] ^= buf1;    state[9] ^= buf2;    state[10] ^= buf1;    state[11] ^= buf2;
	//col4
	buf1 = galois_mul2(galois_mul2(state[12]^state[14]));
	buf2 = galois_mul2(galois_mul2(state[13]^state[15]));
	state[12] ^= buf1;    state[13] ^= buf2;    state[14] ^= buf1;    state[15] ^= buf2;
	// mixcolums //////////
	// col1
	buf1 = state[0] ^ state[1] ^ state[2] ^ state[3];
	buf2 = state[0];
	buf3 = state[0]^state[1]; buf3=galois_mul2(buf3); state[0] = state[0] ^ buf3 ^ buf1;
	buf3 = state[1]^state[2]; buf3=galois_mul2(buf3); state[1] = state[1] ^ buf3 ^ buf1;
	buf3 = state[2]^state[3]; buf3=galois_mul2(buf3); state[2] = state[2] ^ buf3 ^ buf1;
	buf3 = state[3]^buf2;     buf3=galois_mul2(buf3); state[3] = state[3] ^ buf3 ^ buf1;
	// col2
	buf1 = state[4] ^ state[5] ^ state[6] ^ state[7];
	buf2 = state[4];
	buf3 = state[4]^state[5]; buf3=galois_mul2(buf3); state[4] = state[4] ^ buf3 ^ buf1;
	buf3 = state[5]^state[6]; buf3=galois_mul2(buf3); state[5] = state[5] ^ buf3 ^ buf1;
	buf3 = state[6]^state[7]; buf3=galois_mul2(buf3); state[6] = state[6] ^ buf3 ^ buf1;
	buf3 = state[7]^buf2;     buf3=galois_mul2(buf3); state[7] = state[7] ^ buf3 ^ buf1;
	// col3
	buf1 = state[8] ^ state[9] ^ state[10] ^ state[11];
	buf2 = state[8];
	buf3 = state[8]^state[9];   buf3=galois_mul2(buf3); state[8] = state[8] ^ buf3 ^ buf1;
	buf3 = state[9]^state[10];  buf3=galois_mul2(buf3); state[9] = state[9] ^ buf3 ^ buf1;
	buf3 = state[10]^state[11]; buf3=galois_mul2(buf3); state[10] = state[10] ^ buf3 ^ buf1;
	buf3 = state[11]^buf2;      buf3=galois_mul2(buf3); state[11] = state[11] ^ buf3 ^ buf1;
	// col4
	buf1 = state[12] ^ state[13] ^ state[14] ^ state[15];
	buf2 = state[12];
	buf3 = state[12]^state[13]; buf3=galois_mul2(buf3); state[12] = state[12] ^ buf3 ^ buf1;
	buf3 = state[13]^state[14]; buf3=galois_mul2(buf3); state[13] = state[13] ^ buf3 ^ buf1;
	buf3 = state[14]^state[15]; buf3=galois_mul2(buf3); state[14] = state[14] ^ buf3 ^ buf1;
	buf3 = state[15]^buf2;      buf3=galois_mul2(buf3); state[15] = state[15] ^ buf3 ^ buf1;

	// addroundkey, rsbox and shiftrows
	// row 0
	state[ 0]  = rsbox[state[ 0]] ^ expandedKey[(round*16)     ];
	state[ 4]  = rsbox[state[ 4]] ^ expandedKey[(round*16) +  4];
	state[ 8]  = rsbox[state[ 8]] ^ expandedKey[(round*16) +  8];
	state[12]  = rsbox[state[12]] ^ expandedKey[(round*16) + 12];
	// row 1
	buf1 =       rsbox[state[13]] ^ expandedKey[(round*16) +  1];
	state[13]  = rsbox[state[ 9]] ^ expandedKey[(round*16) + 13];
	state[ 9]  = rsbox[state[ 5]] ^ expandedKey[(round*16) +  9];
	state[ 5]  = rsbox[state[ 1]] ^ expandedKey[(round*16) +  5];
	state[ 1]  = buf1;
	// row 2
	buf1 =       rsbox[state[ 2]] ^ expandedKey[(round*16) + 10];
	buf2 =       rsbox[state[ 6]] ^ expandedKey[(round*16) + 14];
	state[ 2]  = rsbox[state[10]] ^ expandedKey[(round*16) +  2];
	state[ 6]  = rsbox[state[14]] ^ expandedKey[(round*16) +  6];
	state[10]  = buf1;
	state[14]  = buf2;
	// row 3
	buf1 =       rsbox[state[ 3]] ^ expandedKey[(round*16) + 15];
	state[ 3]  = rsbox[state[ 7]] ^ expandedKey[(round*16) +  3];
	state[ 7]  = rsbox[state[11]] ^ expandedKey[(round*16) +  7];
	state[11]  = rsbox[state[15]] ^ expandedKey[(round*16) + 11];
	state[15]  = buf1;
  }
}

/**************************************************************
					   End of AES128
 **************************************************************/

#if AES_HW
#include <wmmintrin.h>

/* Originally from:
 * https://www.intel.com/content/dam/doc/white-paper/advanced-encryption-standard-new-instructions-set-paper.pdf
 */

static int cpu_supports_aes(void)
{
	static int supported = -1;
	uint32_t regs[4];

	if (supported == -1) {
#ifdef WIN32
		__cpuid(regs, 1);
#else
		asm volatile
			("cpuid"
			 : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
			 : "a" (1), "c" (0));
#endif

		supported = !!(regs[2] & (1 << 25)); /* bit 25 is aes */
	}

	return supported;
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

static void AES_128_Key_Expansion(const unsigned char *userkey, unsigned char *key)
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

/* Unrolling the loop is a big win for ECB... but not for CBC */
static void AES128_ECB_encrypt_block(const unsigned char *in,
									 unsigned char *out,
									 const uint8_t *key)
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

static void AES128_ECB_decrypt_block(const unsigned char *in,
									 unsigned char *out,
									 const uint8_t *key)
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

static void AES128_expand_key(const uint8_t *userkey, uint8_t *key, int encrypt)
{
	if (encrypt)
		AES_128_Key_Expansion(userkey, key);
	else {
		ALIGN16 unsigned char temp_key[16 * 11];
		__m128i *Key_Schedule = (__m128i *)key;
		__m128i *Temp_Key_Schedule = (__m128i *)temp_key;

		AES_128_Key_Expansion(userkey, temp_key);

		Key_Schedule[10] = Temp_Key_Schedule[0];
		Key_Schedule[9] = _mm_aesimc_si128(Temp_Key_Schedule[1]);
		Key_Schedule[8] = _mm_aesimc_si128(Temp_Key_Schedule[2]);
		Key_Schedule[7] = _mm_aesimc_si128(Temp_Key_Schedule[3]);
		Key_Schedule[6] = _mm_aesimc_si128(Temp_Key_Schedule[4]);
		Key_Schedule[5] = _mm_aesimc_si128(Temp_Key_Schedule[5]);
		Key_Schedule[4] = _mm_aesimc_si128(Temp_Key_Schedule[6]);
		Key_Schedule[3] = _mm_aesimc_si128(Temp_Key_Schedule[7]);
		Key_Schedule[2] = _mm_aesimc_si128(Temp_Key_Schedule[8]);
		Key_Schedule[1] = _mm_aesimc_si128(Temp_Key_Schedule[9]);
		Key_Schedule[0] = Temp_Key_Schedule[10];
	}
}
#endif

/* Public functions */

int AES128_init_ctx(aes128_ctx *ctx, const uint8_t *key, int encrypt)
{
	if (!key)
		return EINVAL;

	memset(ctx, 0, sizeof(aes128_ctx));

#if AES_HW
	if (cpu_supports_aes()) {
		ctx->have_hw = 1;
		AES128_expand_key(key, ctx->roundkey, encrypt);
	} else
#endif
		expandKey(ctx->roundkey, key);

	return 0;
}

extern void aes_encr(unsigned char *state, unsigned char *expandedKey);


void AES128_ECB_encrypt(aes128_ctx *ctx, const uint8_t* input, uint8_t* output)
{
#if AES_HW
	if (ctx->have_hw)
		AES128_ECB_encrypt_block(input, output, ctx->roundkey);
	else
#endif
	{
		memcpy(output, input, 16);
		aes_encr(output, ctx->roundkey);
	}
}

/* wrapper function for the simple case */
void AES128_ECB_encrypt_buffer(const uint8_t* input,
							   const uint8_t *key,
							   uint8_t *output,
							   int len)
{
	aes128_ctx ctx;

	AES128_init_ctx(&ctx, key, 1);

	while (len >= AES128_KEYLEN) {
		AES128_ECB_encrypt(&ctx, input, output);
		input += AES128_KEYLEN;
		output += AES128_KEYLEN;
		len -= AES128_KEYLEN;
	}

	if (len) {
		uint8_t inbuf[AES128_KEYLEN], outbuf[AES128_KEYLEN];

		memset(inbuf, 0, AES128_KEYLEN); /* zero pad */
		memcpy(inbuf, input, len);
		AES128_ECB_encrypt(&ctx, inbuf, outbuf);
		memcpy(output, outbuf, len);
	}

	memset(&ctx, 0, sizeof(ctx)); /* zeroize */
}

void AES128_ECB_decrypt(aes128_ctx *ctx, const uint8_t* input, uint8_t *output)
{
#if AES_HW
	if (ctx->have_hw)
		AES128_ECB_decrypt_block(input, output, ctx->roundkey);
	else
#endif
	{
		memcpy(output, input, 16);
		aes_decr(output, ctx->roundkey);
	}
}

/* wrapper function for the simple case */
void AES128_ECB_decrypt_buffer(const uint8_t* input,
							   const uint8_t *key,
							   uint8_t *output,
							   int len)
{
	aes128_ctx ctx;

	AES128_init_ctx(&ctx, key, 0);

	while (len >= AES128_KEYLEN) {
		AES128_ECB_decrypt(&ctx, input, output);
		input += AES128_KEYLEN;
		output += AES128_KEYLEN;
		len -= AES128_KEYLEN;
	}

	if (len) {
		uint8_t inbuf[AES128_KEYLEN], outbuf[AES128_KEYLEN];

		memset(inbuf, 0, AES128_KEYLEN); /* zero pad */
		memcpy(inbuf, input, len);
		AES128_ECB_decrypt(&ctx, inbuf, outbuf);
		memcpy(output, outbuf, len);
	}

	memset(&ctx, 0, sizeof(ctx)); /* zeroize */
}
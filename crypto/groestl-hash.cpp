/*######   Copyright (c) 2015-2018 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#																																	  #
# Used Groestl implementation with inline assembly using ssse3, sse4.1, and aes instructions.											  #
# Authors: Günther A.Roland, Martin Schläffer, Krystian Matusiewicz																	  #
#####################################################################################################################################*/

// Implementation if Grøstl Hash function.
// http://groestl.info

#include <el/ext.h>
#include <el/comp/byte-permutation.h>

#include "hash.h"
#include "aes-imp.h"

#if UCFG_IMP_GROESTL=='S'
#	define SPH_NO_64 0
#	define SPH_BIG_ENDIAN (!UCFG_LITLE_ENDIAN)
#	define SPH_I386_GCC 0
#	define SPH_AMD64_GCC 0
#	define SPH_SPARCV9_GCC 0
#	pragma warning(disable: 4242 4244)

extern "C" {
#	include <sphlib/sph_groestl.h>
}
#endif // UCFG_IMP_GROESTL=='S'

using namespace Ext;

extern "C" {

uint64_t (*g_groestl_T_table)[256];

typedef uint64_t(*PRoundConstants_PQ)[15][16];			// round #15 contains Zeros, used in as implementation
PRoundConstants_PQ g_groestl_RoundConstants_PQ;

typedef void(*PFN_AddRoundConstant)(uint64_t d[], int i, int cols);


#if UCFG_CPU_X86_X64
void _cdecl Groestl512_x86x64Sse2(const uint64_t roundConstants[15][16], uint64_t u[16], const uint8_t shiftTable8[]);

static bool s_bHasAesAndSsse3 = CpuInfo().Features.AES && CpuInfo().Features.SSSE3;
static bool s_bHasSse2 = CpuInfo().Features.SSE2;
#endif // UCFG_CPU_X86_X64
}


namespace Ext { namespace Crypto {

static uint8_t
	s_shiftTableP512[16] = { 0, 1, 2, 3,  4, 5, 6, 7 },
	s_shiftTableQ512[16] = { 1, 3, 5, 7,  0, 2, 4, 6 },
	s_shiftTableP1024[16] = { 0, 1, 2, 3,  4, 5, 6, 11 },
	s_shiftTableQ1024[16] = { 1, 3, 5, 11, 0, 2, 4, 6 };

#if UCFG_CPU_X86_X64

static const __m128i s_P1024_ShuffleMasksAfterAes[8] = {
	{ 0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3 },	// 0
	{ 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0 },	// 1
	{ 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13 },	// 2
	{ 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10 },	// 3
	{ 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10, 7 },	// 4
	{ 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10, 7, 4 },	// 5
	{ 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10, 7, 4, 1 },	// 6
	{ 15, 12, 9, 6, 3, 0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2 },	// 11
};

static const __m128i s_Q1024_ShuffleMasksAfterAes[8] = {
	{ 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0 },	// 1
	{ 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10 },	// 3
	{ 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10, 7, 4 },	// 5
	{ 15, 12, 9, 6, 3, 0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2 },	// 11
	{ 0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3 },	// 0
	{ 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13 },	// 2
	{ 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10, 7 },	// 4
	{ 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 0, 13, 10, 7, 4, 1 },	// 6
};

#endif // UCFG_CPU_X86_X64

template <size_t N>
struct GroestlPQParams {
	uint8_t ShiftTable[16];

	typedef uint64_t(*PRoundConstants)[16];			// round #15 contains Zeros, used in as implementation

	PRoundConstants RoundConstants;
#if UCFG_CPU_X86_X64
	__m128i AesRoundConstants[14][8];
	__m128i ShuffleMasksAfterAes[8];
#endif
};

static GroestlPQParams<128> s_groestl_p1024_params;
static GroestlPQParams<128> s_groestl_q1024_params;

BytePermutation<128> Aes8Permutation()
{
	static const uint8_t aesPerm[128] = {
		0x00, 0x05, 0x0a, 0x0f, 0x04, 0x09, 0x0e, 0x03, 0x08, 0x0d, 0x02, 0x07, 0x0c, 0x01, 0x06, 0x0b,
		0x10, 0x15, 0x1a, 0x1f, 0x14, 0x19, 0x1e, 0x13, 0x18, 0x1d, 0x12, 0x17, 0x1c, 0x11, 0x16, 0x1b,
		0x20, 0x25, 0x2a, 0x2f, 0x24, 0x29, 0x2e, 0x23, 0x28, 0x2d, 0x22, 0x27, 0x2c, 0x21, 0x26, 0x2b,
		0x30, 0x35, 0x3a, 0x3f, 0x34, 0x39, 0x3e, 0x33, 0x38, 0x3d, 0x32, 0x37, 0x3c, 0x31, 0x36, 0x3b,
		0x40, 0x45, 0x4a, 0x4f, 0x44, 0x49, 0x4e, 0x43, 0x48, 0x4d, 0x42, 0x47, 0x4c, 0x41, 0x46, 0x4b,
		0x50, 0x55, 0x5a, 0x5f, 0x54, 0x59, 0x5e, 0x53, 0x58, 0x5d, 0x52, 0x57, 0x5c, 0x51, 0x56, 0x5b,
		0x60, 0x65, 0x6a, 0x6f, 0x64, 0x69, 0x6e, 0x63, 0x68, 0x6d, 0x62, 0x67, 0x6c, 0x61, 0x66, 0x6b,
		0x70, 0x75, 0x7a, 0x7f, 0x74, 0x79, 0x7e, 0x73, 0x78, 0x7d, 0x72, 0x77, 0x7c, 0x71, 0x76, 0x7b,
	};
	return BytePermutation<128>(aesPerm);
}


BytePermutation<128> GroestlP1024Permutation()
{
	static const uint8_t p1024Perm[128] = {
		0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x5f, 0x08, 0x11, 0x1a, 0x23, 0x2c, 0x35, 0x3e, 0x67,
		0x10, 0x19, 0x22, 0x2b, 0x34, 0x3d, 0x46, 0x6f, 0x18, 0x21, 0x2a, 0x33, 0x3c, 0x45, 0x4e, 0x77,
		0x20, 0x29, 0x32, 0x3b, 0x44, 0x4d, 0x56, 0x7f, 0x28, 0x31, 0x3a, 0x43, 0x4c, 0x55, 0x5e, 0x07,
		0x30, 0x39, 0x42, 0x4b, 0x54, 0x5d, 0x66, 0x0f, 0x38, 0x41, 0x4a, 0x53, 0x5c, 0x65, 0x6e, 0x17,
		0x40, 0x49, 0x52, 0x5b, 0x64, 0x6d, 0x76, 0x1f, 0x48, 0x51, 0x5a, 0x63, 0x6c, 0x75, 0x7e, 0x27,
		0x50, 0x59, 0x62, 0x6b, 0x74, 0x7d, 0x06, 0x2f, 0x58, 0x61, 0x6a, 0x73, 0x7c, 0x05, 0x0e, 0x37,
		0x60, 0x69, 0x72, 0x7b, 0x04, 0x0d, 0x16, 0x3f, 0x68, 0x71, 0x7a, 0x03, 0x0c, 0x15, 0x1e, 0x47,
		0x70, 0x79, 0x02, 0x0b, 0x14, 0x1d, 0x26, 0x4f, 0x78, 0x01, 0x0a, 0x13, 0x1c, 0x25, 0x2e, 0x57,
	};
	return BytePermutation<128>(p1024Perm);
}

BytePermutation<128> GroestlQ1024Permutation()
{
	static const uint8_t q1024Perm[128] = {
		0x08, 0x19, 0x2a, 0x5b, 0x04, 0x15, 0x26, 0x37, 0x10, 0x21, 0x32, 0x63, 0x0c, 0x1d, 0x2e, 0x3f,
		0x18, 0x29, 0x3a, 0x6b, 0x14, 0x25, 0x36, 0x47, 0x20, 0x31, 0x42, 0x73, 0x1c, 0x2d, 0x3e, 0x4f,
		0x28, 0x39, 0x4a, 0x7b, 0x24, 0x35, 0x46, 0x57, 0x30, 0x41, 0x52, 0x03, 0x2c, 0x3d, 0x4e, 0x5f,
		0x38, 0x49, 0x5a, 0x0b, 0x34, 0x45, 0x56, 0x67, 0x40, 0x51, 0x62, 0x13, 0x3c, 0x4d, 0x5e, 0x6f,
		0x48, 0x59, 0x6a, 0x1b, 0x44, 0x55, 0x66, 0x77, 0x50, 0x61, 0x72, 0x23, 0x4c, 0x5d, 0x6e, 0x7f,
		0x58, 0x69, 0x7a, 0x2b, 0x54, 0x65, 0x76, 0x07, 0x60, 0x71, 0x02, 0x33, 0x5c, 0x6d, 0x7e, 0x0f,
		0x68, 0x79, 0x0a, 0x3b, 0x64, 0x75, 0x06, 0x17, 0x70, 0x01, 0x12, 0x43, 0x6c, 0x7d, 0x0e, 0x1f,
		0x78, 0x09, 0x1a, 0x4b, 0x74, 0x05, 0x16, 0x27, 0x00, 0x11, 0x22, 0x53, 0x7c, 0x0d, 0x1e, 0x2f,
	};
	return BytePermutation<128>(q1024Perm);
}



static void ExtendTable(uint8_t t[16]) {
	for (int row=0; row<8; ++row)
		t[row + 8] = uint8_t(8 * t[row] + row);
}

static void AddRoundConstantP(uint64_t d[], int i, int cols) {
	uint8_t v = (uint8_t)i;
	for (int c=0; c<cols; ++c, v+=0x10)
		*(uint8_t*)(d + c) ^= v;
}

static void AddRoundConstantQ(uint64_t d[], int i, int cols) {
	for (int c=0; c<cols; ++c)
		d[c] ^= 0xFFFFFFFFFFFFFFFFLL ^ (uint64_t((c<<4) | i) << 56);
}

void InitGroestlTables() {
	static bool s_b;
	if (s_b)
		return;
	InitAesTables();

	ExtendTable(s_shiftTableP512);
	ExtendTable(s_shiftTableQ512);
	ExtendTable(s_shiftTableP1024);
	ExtendTable(s_shiftTableQ1024);

	g_groestl_T_table = new uint64_t[8][256];
	for (int i=0; i<256; ++i) {
		uint8_t a = g_aesSubByte[i];
		uint8_t *p = (uint8_t*)&g_groestl_T_table[0][i];
		p[7] = p[0] = Mul(2, a);
		p[6] = p[3] = Mul(3, a);
		p[5] = Mul(4, a);
		p[4] = p[2] = Mul(5, a);
		p[1] = Mul(7, a);
		for (int row=1; row<8; ++row)
			g_groestl_T_table[row][i] = _rotl64(g_groestl_T_table[0][i], row*8);
	}

	size_t size = 2*15*16*sizeof(uint64_t);
	g_groestl_RoundConstants_PQ = (PRoundConstants_PQ)AlignedMalloc(size, 16);
	memset(g_groestl_RoundConstants_PQ, 0, size);
	for (int i=0; i<14; ++i) {
		AddRoundConstantP(g_groestl_RoundConstants_PQ[0][i], i, 16);
		AddRoundConstantQ(g_groestl_RoundConstants_PQ[1][i], i, 16);
	}

	memcpy(s_groestl_p1024_params.ShiftTable, s_shiftTableP1024, 16);
	s_groestl_p1024_params.RoundConstants = g_groestl_RoundConstants_PQ[0];

	memcpy(s_groestl_q1024_params.ShiftTable, s_shiftTableQ1024, 16);
	s_groestl_q1024_params.RoundConstants = g_groestl_RoundConstants_PQ[1];

#if UCFG_CPU_X86_X64
	memcpy(s_groestl_p1024_params.ShuffleMasksAfterAes, s_P1024_ShuffleMasksAfterAes, sizeof s_P1024_ShuffleMasksAfterAes);
	memcpy(s_groestl_q1024_params.ShuffleMasksAfterAes, s_Q1024_ShuffleMasksAfterAes, sizeof s_P1024_ShuffleMasksAfterAes);

	auto transposion = Transposition128bytesBy8();
	for (int i = 0; i < 14; ++i) {
		transposion.Apply((uint8_t*)s_groestl_p1024_params.AesRoundConstants[i], (const uint8_t*)s_groestl_p1024_params.RoundConstants[i]);
		transposion.Apply((uint8_t*)s_groestl_q1024_params.AesRoundConstants[i], (const uint8_t*)s_groestl_q1024_params.RoundConstants[i]);
	}
#endif // UCFG_CPU_X86_X64

	s_b = true;
}

GroestlHash::GroestlHash(size_t hashSize) {
	IsBlockCounted = true;
	HashSize = hashSize;
	BlockSize = (Is64Bit = HashSize>=32) ? 128 : 64;
	IsBigEndian = !UCFG_LITLE_ENDIAN;					// to prevent byte swapping
	InitGroestlTables();
}

void GroestlHash::InitHash(void *dst) noexcept {
	uint64_t *d = (uint64_t *)dst;
	for (int i=BlockSize/8; i--;)
		d[i] = 0;
	((uint16_t*)dst)[BlockSize/2 - 1] = htobe(uint16_t(HashSize*8));
}

hashval Groestl512Hash::ComputeHash(RCSpan mb) {
#if UCFG_IMP_GROESTL=='S'
	sph_groestl512_context ctx;
	sph_groestl512_init(&ctx);
	sph_groestl512(&ctx, mb.P, mb.Size);
	hashval r(64);
	sph_groestl512_close(&ctx, r.data());
	return r;
#else
	return base::ComputeHash(mb);
#endif // UCFG_IMP_GROESTL=='S'
}

#if UCFG_IMP_GROESTL!='T'
static uint8_t MulRow(uint64_t a, uint64_t b) {
	byte *pa = (uint8_t*)&a, *pb = (uint8_t*)&b;
	uint8_t r = Mul(pa[0], pb[0]);
	for (int i = 1; i<8; ++i)
		r ^= Mul(pa[i], pb[i]);
	return r;
}

static uint64_t MulPolynom(uint64_t col, uint64_t p) {
	uint64_t r = MulRow(col, p);
	for (int i = 1; i<8; ++i)
		r |= uint64_t(MulRow(col, _rotl64(p, i * 8))) << i * 8;
	return r;
}

static uint64_t MulColumnAsLe(uint64_t col) {		// returns Little-endian.
	uint64_t r;
	uint8_t *pr = (uint8_t *)&r;
	pr[0] = MulRow(col, 0x0705030504030202);
	pr[1] = MulRow(col, 0x0503050403020207);
	pr[2] = MulRow(col, 0x0305040302020705);
	pr[3] = MulRow(col, 0x0504030202070503);
	pr[4] = MulRow(col, 0x0403020207050305);
	pr[5] = MulRow(col, 0x0302020705030504);
	pr[6] = MulRow(col, 0x0202070503050403);
	pr[7] = MulRow(col, 0x0207050305040302);
	return r;
}
#endif // UCFG_IMP_GROESTL!='T'

#if UCFG_CPU_X86_X64

/* xmm[i] will be multiplied by 2
* xmm[j] will be lost
* xmm[k] has to be all 0x1b */
#define MUL2(i, j, k){\
  j = _mm_xor_si128(j, j);\
  j = _mm_cmpgt_epi8(j, i);\
  i = _mm_add_epi8(i, i);\
  j = _mm_and_si128(j, k);\
  i = _mm_xor_si128(i, j);\
}/**/

/* Yet another implementation of MixBytes.
This time we use the formulae (3) from the paper "Byte Slicing Groestl".
Input: a0, ..., a7
Output: b0, ..., b7 = MixBytes(a0,...,a7).
but we use the relations:
t_i = a_i + a_{i+3}
x_i = t_i + t_{i+3}
y_i = t_i + t+{i+2} + a_{i+6}
z_i = 2*x_i
w_i = z_i + y_{i+4}
v_i = 2*w_i
b_i = v_{i+3} + y_{i+4}
We keep building b_i in registers xmm8..xmm15 by first building y_{i+4} there
and then adding v_i computed in the meantime in registers xmm0..xmm7.
We almost fit into 16 registers, need only 3 spills to memory.
This implementation costs 7.7 c/b giving total speed on SNB: 10.7c/b.
K. Matusiewicz, 2011/05/29 */
#define MixBytes(a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7){\
  /* t_i = a_i + a_{i+1} */\
  b6 = a0;\
  b7 = a1;\
  a0 = _mm_xor_si128(a0, a1);\
  b0 = a2;\
  a1 = _mm_xor_si128(a1, a2);\
  b1 = a3;\
  a2 = _mm_xor_si128(a2, a3);\
  b2 = a4;\
  a3 = _mm_xor_si128(a3, a4);\
  b3 = a5;\
  a4 = _mm_xor_si128(a4, a5);\
  b4 = a6;\
  a5 = _mm_xor_si128(a5, a6);\
  b5 = a7;\
  a6 = _mm_xor_si128(a6, a7);\
  a7 = _mm_xor_si128(a7, b6);\
  \
  /* build y4 y5 y6 ... in regs xmm8, xmm9, xmm10 by adding t_i*/\
  b0 = _mm_xor_si128(b0, a4);\
  b6 = _mm_xor_si128(b6, a4);\
  b1 = _mm_xor_si128(b1, a5);\
  b7 = _mm_xor_si128(b7, a5);\
  b2 = _mm_xor_si128(b2, a6);\
  b0 = _mm_xor_si128(b0, a6);\
  /* spill values y_4, y_5 to memory */\
  TEMP0 = b0;\
  b3 = _mm_xor_si128(b3, a7);\
  b1 = _mm_xor_si128(b1, a7);\
  TEMP1 = b1;\
  b4 = _mm_xor_si128(b4, a0);\
  b2 = _mm_xor_si128(b2, a0);\
  /* save values t0, t1, t2 to xmm8, xmm9 and memory */\
  b0 = a0;\
  b5 = _mm_xor_si128(b5, a1);\
  b3 = _mm_xor_si128(b3, a1);\
  b1 = a1;\
  b6 = _mm_xor_si128(b6, a2);\
  b4 = _mm_xor_si128(b4, a2);\
  TEMP2 = a2;\
  b7 = _mm_xor_si128(b7, a3);\
  b5 = _mm_xor_si128(b5, a3);\
  \
  /* compute x_i = t_i + t_{i+3} */\
  a0 = _mm_xor_si128(a0, a3);\
  a1 = _mm_xor_si128(a1, a4);\
  a2 = _mm_xor_si128(a2, a5);\
  a3 = _mm_xor_si128(a3, a6);\
  a4 = _mm_xor_si128(a4, a7);\
  a5 = _mm_xor_si128(a5, b0);\
  a6 = _mm_xor_si128(a6, b1);\
  a7 = _mm_xor_si128(a7, TEMP2);\
  \
  /* compute z_i : double x_i using temp xmm8 and 1B xmm9 */\
  /* compute w_i : add y_{i+4} */\
  b1 = ALL_1B;\
  MUL2(a0, b0, b1);\
  a0 = _mm_xor_si128(a0, TEMP0);\
  MUL2(a1, b0, b1);\
  a1 = _mm_xor_si128(a1, TEMP1);\
  MUL2(a2, b0, b1);\
  a2 = _mm_xor_si128(a2, b2);\
  MUL2(a3, b0, b1);\
  a3 = _mm_xor_si128(a3, b3);\
  MUL2(a4, b0, b1);\
  a4 = _mm_xor_si128(a4, b4);\
  MUL2(a5, b0, b1);\
  a5 = _mm_xor_si128(a5, b5);\
  MUL2(a6, b0, b1);\
  a6 = _mm_xor_si128(a6, b6);\
  MUL2(a7, b0, b1);\
  a7 = _mm_xor_si128(a7, b7);\
  \
  /* compute v_i : double w_i      */\
  /* add to y_4 y_5 .. v3, v4, ... */\
  MUL2(a0, b0, b1);\
  b5 = _mm_xor_si128(b5, a0);\
  MUL2(a1, b0, b1);\
  b6 = _mm_xor_si128(b6, a1);\
  MUL2(a2, b0, b1);\
  b7 = _mm_xor_si128(b7, a2);\
  MUL2(a5, b0, b1);\
  b2 = _mm_xor_si128(b2, a5);\
  MUL2(a6, b0, b1);\
  b3 = _mm_xor_si128(b3, a6);\
  MUL2(a7, b0, b1);\
  b4 = _mm_xor_si128(b4, a7);\
  MUL2(a3, b0, b1);\
  MUL2(a4, b0, b1);\
  b0 = TEMP0;\
  b1 = TEMP1;\
  b0 = _mm_xor_si128(b0, a3);\
  b1 = _mm_xor_si128(b1, a4);\
}/*MixBytes*/


static const __m128i
	s_zeroM128i = { 0, 0 },
	ALL_FF = _mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff),
	ALL_1B = _mm_set_epi32(0x1b1b1b1b, 0x1b1b1b1b, 0x1b1b1b1b, 0x1b1b1b1b);

static const BytePermutation<128>
	g_groestlTransposion128 = Transposition128bytesBy8(),
	g_groestlReverseTransposion128 = !g_groestlTransposion128;

void _cdecl Groestl512_x86x64Aes(__m128i dst[8], const __m128i s[8], const GroestlPQParams<128>& params)
{
	__m128i x[8],
		TEMP0, TEMP1, TEMP2;

	memcpy(x, s, 128);

	for (int round = 0; round<14; ++round) {
		const __m128i *keys = params.AesRoundConstants[round];
		#define GROESTL_ROW(n, k) _mm_shuffle_epi8(_mm_aesenclast_si128(_mm_xor_si128(x[n], keys[k]), s_zeroM128i), params.ShuffleMasksAfterAes[n]);

		__m128i a = GROESTL_ROW(0, 0);
		__m128i b = GROESTL_ROW(1, 1);
		__m128i c = GROESTL_ROW(2, 1);
		__m128i d = GROESTL_ROW(3, 1);
		__m128i e = GROESTL_ROW(4, 1);
		__m128i f = GROESTL_ROW(5, 1);
		__m128i g = GROESTL_ROW(6, 1);
		__m128i h = GROESTL_ROW(7, 7);

		MixBytes(a, b, c, d, e, f, g, h, x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7]);
	}

	VectorXor8(dst, x);
}

#endif // UCFG_CPU_X86_X64

static void TransformBlock1024(uint64_t d[], const uint64_t s[], const GroestlPQParams<128>& params) {
	const int cols = 16;
	const uint8_t *shiftTable8 = params.ShiftTable + 8;
	int maskCols = cols-1, maskCols8 = (maskCols << 3) | 7;
	uint64_t t[16], u[32], *pT = t, *pU = u;					// u has double size for ASM implementation
#if UCFG_USE_MASM && UCFG_PLATFORM_X64
	if (s_bHasSse2) {
		memcpy(u, s, cols * 8);
		Groestl512_x86x64Sse2(params.RoundConstants, u, shiftTable8);
	} else
#endif
	{
		memcpy(u, s, cols * 8);
		for (int i=0; i<14; ++i) {
			uint8_t *pt = (uint8_t*)pT, *pu = (uint8_t*)pU, *endu = pu + cols * 8;

			VectorXor(pU, params.RoundConstants[i], cols);							// AddRoundConstant
#if UCFG_IMP_GROESTL=='T'
			for (int c=0; c<cols; ++c) {
				uint64_t col = 0;
				for (int row=0; row<8; ++row)
					col ^= g_groestl_T_table[row][pu[(c*8+shiftTable8[row]) & maskCols8]];
				pT[c] = col;
			}
			std::swap(pT, pU);
#else
			for (uint8_t *p = pu; p != endu; ++p) // SubBytes
				*p = g_aesSubByte[*p];
			for (int row=0; row<8; ++row) {									// ShiftBytes
				int offset = params.ShiftTable[row];
				for (int c=0; c<cols; ++c)
					pt[c*8 + row] = pu[((c+offset) & maskCols)*8 + row];
			}
			for (int c=0; c<cols; ++c)										// MixColumns
				u[c] = MulColumnAsLe(letoh(t[c]));
#endif
		}
	}
	VectorXor(d, pU, cols);
}



static void P1024(uint64_t d[16], const uint64_t s[16]) {
	TransformBlock1024(d, s, s_groestl_p1024_params);
}

static void Q1024(uint64_t d[16], const uint64_t s[16]) {
	TransformBlock1024(d, s, s_groestl_q1024_params);
}

void Groestl512Hash::InitHash(void *dst) noexcept {
	base::InitHash(dst);
#if UCFG_CPU_X86_X64
	if (s_bHasAesAndSsse3) {
		__m128i d[8];
		g_groestlTransposion128.Apply((uint8_t*)d, (const uint8_t*)dst);
		memcpy(dst, d, 128);
	}
#endif
}

void Groestl512Hash::HashBlock(void *dst, uint8_t src[256], uint64_t counter) noexcept {
#if UCFG_CPU_X86_X64
	if (s_bHasAesAndSsse3) {
		__m128i hm[8],
			s[8];
		g_groestlTransposion128.Apply((uint8_t*)s, (const uint8_t*)src);
		VectorXor8(hm, (__m128i*)dst, s);
		Groestl512_x86x64Aes((__m128i*)dst, hm, s_groestl_p1024_params);
		Groestl512_x86x64Aes((__m128i*)dst, s, s_groestl_q1024_params);
	} else
#endif
	{
		uint64_t hm[16];
		memcpy(hm, dst, 128);

		VectorXor(hm, (const uint64_t*)src, 16);
		P1024((uint64_t*)dst, hm);

		Q1024((uint64_t*)dst, (const uint64_t*)src);
	}
}

void Groestl512Hash::OutTransform(void *dst) noexcept {
#if UCFG_CPU_X86_X64
	if (s_bHasAesAndSsse3) {
		__m128i d[8];
		Groestl512_x86x64Aes((__m128i*)dst, (const __m128i*)dst, s_groestl_p1024_params);
		g_groestlReverseTransposion128.Apply((uint8_t*)d, (const uint8_t*)dst);
		memcpy((__m128i*)dst, d + 4, 64);
	} else
#endif
	{
		P1024((uint64_t*)dst, (const uint64_t*)dst);
		memcpy(dst, (uint8_t*)dst + 64, 64);
	}
}

}} // Ext::Crypto::

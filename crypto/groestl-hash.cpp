/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

// Implementation if Grøstl Hash function.
// http://groestl.info

#include <el/ext.h>

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

typedef void(*PFN_AddRoundConstant)(uint64_t d[], int i, int cols);


#if UCFG_CPU_X86_X64
void _cdecl Groestl512_x86x64Sse2(PFN_AddRoundConstant pfnAddRoundConstant, uint64_t u[16], const byte shiftTable8[]);

static bool s_bHasSse2 = CpuInfo().HasSse2;
#endif // UCFG_CPU_X86_X64
}

namespace Ext { namespace Crypto {

static byte s_shiftTableP512[16] ={ 0, 1, 2, 3,  4, 5, 6, 7 },
	s_shiftTableQ512[16] ={ 1, 3, 5, 7,  0, 2, 4, 6 },
	s_shiftTableP1024[16] ={ 0, 1, 2, 3,  4, 5, 6, 11 },
	s_shiftTableQ1024[16] ={ 1, 3, 5, 11, 0, 2, 4, 6 };

static void ExtendTable(byte t[16]) {
	for (int i=0; i<8; ++i)
		t[i+8] = 8*t[i];
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
		byte a = g_aesSubByte[i];
		byte *p = (byte*)&g_groestl_T_table[0][i];
		p[0] = Mul(2, a);
		p[1] = Mul(7, a);
		p[2] = Mul(5, a);
		p[3] = Mul(3, a);
		p[4] = Mul(5, a);
		p[5] = Mul(4, a);
		p[6] = Mul(3, a);
		p[7] = Mul(2, a);
		for (int row=1; row<8; ++row)
			g_groestl_T_table[row][i] = _rotl64(g_groestl_T_table[0][i], row*8);
	}

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
	memset(dst, 0, BlockSize-2);
	((uint16_t*)dst)[BlockSize/2 - 1] = htobe(uint16_t(HashSize*8));
}

hashval Groestl512Hash::ComputeHash(const ConstBuf& mb) {
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



static void AddRoundConstantP(uint64_t d[], int i, int cols) {
	byte v = (byte)i;
	for (int c=0; c<cols; ++c, v+=0x10)
		*(byte*)(d+c) ^= v;
}

static void AddRoundConstantQ(uint64_t d[], int i, int cols) {
	for (int c=0; c<cols; ++c)
		d[c] ^= 0xFFFFFFFFFFFFFFFFLL ^ (uint64_t((c<<4) | i) << 56);
}

#if UCFG_IMP_GROESTL!='T'
static uint64_t MulRow(uint64_t a, uint64_t b) {
	byte *pa = (byte*)&a, *pb = (byte*)&b;
	byte r = Mul(pa[0], pb[0]);
	for (int i=1; i<8; ++i)
		r ^= Mul(pa[i], pb[i]);
 	return r;
}

static uint64_t MulPolynom(uint64_t col, uint64_t p) {
	uint64_t r = MulRow(col, p);
	for (int i=1; i<8; ++i)
		r |= MulRow(col, _rotl64(p, i*8)) << i*8;
	return r;
}
#endif // UCFG_IMP_GROESTL!='T'

static void TransformBlock(PFN_AddRoundConstant pfnAddRoundConstant, uint64_t d[], const uint64_t s[], int cols, int rounds, const byte shiftTable[16]) {
	const byte *shiftTable8 = shiftTable+8;
	int maskCols = cols-1, maskCols8 = maskCols << 3;
	uint64_t t[16], u[32], *pT = t, *pU = u;					// u has double size for ASM implementation
	memcpy(u, s, cols*8);
#if UCFG_USE_MASM && UCFG_PLATFORM_X64
	if (s_bHasSse2) {
		Groestl512_x86x64Sse2(pfnAddRoundConstant, u, shiftTable8);
	} else
#endif
	{
		for (int i=0; i<rounds; ++i) {
			byte *pt = (byte*)pT, *pu = (byte*)pU, *endu = pu + cols*8;

			pfnAddRoundConstant(pU, i, cols);
#if UCFG_IMP_GROESTL=='T'
			for (int c=0; c<cols; ++c) {
				uint64_t col = 0;
				for (int row=0; row<8; ++row)
					col ^= g_groestl_T_table[row][pu[row + ((c*8+shiftTable8[row]) & maskCols8)]];
				pT[c] = col;
			}
			std::swap(pT, pU);
#else
			for (byte *p=pu; p!=endu; ++p)									// SubBytes
				*p = g_aesSubByte[*p];
			for (int row=0; row<8; ++row) {									// ShiftBytes
				int offset = shiftTable[row];
				for (int c=0; c<cols; ++c)
					pt[c*8 + row] = pu[((c+offset) & maskCols)*8 + row];
			}
			for (int c=0; c<cols; ++c)										// MixColumns
				u[c] = htole(MulPolynom(letoh(t[c]), 0x0705030504030202));
#endif
		}
	}
	VectorXor(d, pU, cols);
}




static void P1024(uint64_t d[16], const uint64_t s[16]) {
	TransformBlock(AddRoundConstantP, d, s, 16, 14, s_shiftTableP1024);
}

static void Q1024(uint64_t d[16], const uint64_t s[16]) {
	TransformBlock(AddRoundConstantQ, d, s, 16, 14, s_shiftTableQ1024);
}


void Groestl512Hash::HashBlock(void *dst, const byte *src, uint64_t counter) noexcept {
	uint64_t hm[16];
	memcpy(hm, dst, 128);

	VectorXor(hm, (const uint64_t*)src, 16);
	P1024((uint64_t*)dst, hm);

	Q1024((uint64_t*)dst, (const uint64_t*)src);
}

void Groestl512Hash::OutTransform(void *dst) noexcept {
	P1024((uint64_t*)dst, (const uint64_t*)dst);
	memmove(dst, (byte*)dst + 64, 64);
}

}} // Ext::Crypto::



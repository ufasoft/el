/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "hash.h"

using namespace Ext;


namespace Ext { namespace Crypto {

__forceinline uint32_t Rotr(uint32_t v, int n) { return _rotr(v, n); }		//!!! move to .h
__forceinline uint64_t Rotr(uint64_t v, int n) { return _rotr64(v, n); }

const byte g_blake_sigma[10][16] = {
	{	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	12,	13,	14,	15	},
	{	14,	10,	4,	8,	9,	15,	13,	6,	1,	12,	0,	2,	11,	7,	5,	3	},
	{	11,	8,	12,	0,	5,	2,	15,	13,	10,	14,	3,	6,	7,	1,	9,	4	},
	{	7,	9,	3,	1,	13,	12,	11,	14,	2,	6,	5,	10,	4,	0,	15,	8	},
	{	9,	0,	5,	7,	2,	4,	10,	15,	14,	1,	11,	12,	6,	8,	3,	13	},
	{	2,	12,	6,	10,	0,	11,	8,	3,	4,	13,	7,	5,	15,	14,	1,	9	},
	{	12,	5,	1,	15,	14,	13,	4,	10,	0,	7,	6,	3,	9,	2,	8,	11	},
	{	13,	11,	7,	14,	12,	1,	3,	9,	5,	0,	15,	4,	8,	6,	2,	10	},
	{	6,	15,	14,	9,	11,	3,	0,	8,	12,	2,	13,	7,	1,	4,	10,	5	},
	{	10,	2,	8,	4,	7,	6,	1,	5,	15,	11,	9,	14,	3,	12,	13,	0	}
};

const uint32_t g_blake256_c[16] = {		// first digits of PI
	0x243F6A88, 0x85A308D3, 0x13198A2E, 0x03707344, 0xA4093822, 0x299F31D0, 0x082EFA98, 0xEC4E6C89,
	0x452821E6, 0x38D01377, 0xBE5466CF, 0x34E90C6C, 0xC0AC29B7, 0xC97C50DD, 0x3F84D5B5, 0xB5470917,
};

const uint64_t g_blake512_c[16] = {		// first digits of PI
	0x243F6A8885A308D3, 0x13198A2E03707344, 0xA4093822299F31D0, 0x082EFA98EC4E6C89,
	0x452821E638D01377, 0xBE5466CF34E90C6C, 0xC0AC29B7C97C50DD, 0x3F84D5B5B5470917,
	0x9216D5D98979FB1B, 0xD1310BA698DFB5AC, 0x2FFD72DBD01ADFB7, 0xB8E1AFED6A267E96,
	0xBA7C9045F12C7F99, 0x24A19947B3916CF7, 0x0801F2E2858EFC16, 0x636920D871574E69
};

template <typename W, int N0, int N1, int N2, int N3>
void CalcBlakeGImp(int sigma0, int sigma1, W& a, W& b, W& c, W& d, const W m[16], const W blakeC[]) {
	a += b + (m[sigma0] ^ blakeC[sigma1]);	
	c += (d = Rotr((d ^ a), N0));
	b = Rotr((b ^ c), N1);
	a += b + (m[sigma1] ^ blakeC[sigma0]);	
	c += (d = Rotr((d ^ a), N2));
	b = Rotr((b ^ c), N3);
}

void CalcBlakeG(int sigma0, int sigma1, uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d, const uint32_t m[16]) {
	CalcBlakeGImp<uint32_t, 16, 12, 8, 7>(sigma0, sigma1, a, b, c, d, m, g_blake256_c);
}

void CalcBlakeG(int sigma0, int sigma1, uint64_t& a, uint64_t& b, uint64_t& c, uint64_t& d, const uint64_t m[16]) {
#if UCFG_USE_MASM && defined(_M_IX86)
	Blake512Round(sigma0, sigma1, a, b, c, d, m, g_blake512_c);
#else
	CalcBlakeGImp<uint64_t, 32, 25, 16, 11>(sigma0, sigma1, a, b, c, d, m, g_blake512_c);
#endif
}

template <typename W, int R>
void CalcHashBlock(void *dst, const byte *src, uint64_t counter, const W salt[4], const W blakeC[], W t0, W t1) {
	const W *m = (const W*)src;
	W *h = (W*)dst;

	W w[16] = {
		h[0],					h[1],					h[2],					h[3],
		h[4],					h[5],					h[6],					h[7],
		salt[0] ^ blakeC[0],	salt[1] ^ blakeC[1],	salt[2] ^ blakeC[2],	salt[3] ^ blakeC[3],
		t0 ^ blakeC[4],			t0 ^ blakeC[5],			t1 ^ blakeC[6],			t1 ^ blakeC[7]
	};
	for (int r=0; r<R; ++r) {
		const byte *sigma = g_blake_sigma[r % 10];
		CalcBlakeG(sigma[0],  sigma[1],	 w[0], w[4], w[8],  w[12], m);
		CalcBlakeG(sigma[2],  sigma[3],  w[1], w[5], w[9],  w[13], m);
		CalcBlakeG(sigma[4],  sigma[5],  w[2], w[6], w[10], w[14], m);
		CalcBlakeG(sigma[6],  sigma[7],  w[3], w[7], w[11], w[15], m);
		CalcBlakeG(sigma[8],  sigma[9],  w[0], w[5], w[10], w[15], m);
		CalcBlakeG(sigma[10], sigma[11], w[1], w[6], w[11], w[12], m);
		CalcBlakeG(sigma[12], sigma[13], w[2], w[7], w[8],  w[13], m);
		CalcBlakeG(sigma[14], sigma[15], w[3], w[4], w[9],  w[14], m);
	}
	for (int i=0; i<8; ++i)
		h[i] ^= salt[i % 4] ^ w[i % 8] ^ w[8 + i % 8];
}

void Blake256::InitHash(void *dst) noexcept {
	memcpy(dst, g_sha256_hinit, sizeof(g_sha256_hinit));
}

void Blake256::HashBlock(void *dst, const byte *src, uint64_t counter) noexcept {
	CalcHashBlock<uint32_t, 14>(dst, src, counter, Salt, g_blake256_c, uint32_t(counter), counter >> 32);
}

void Blake512::InitHash(void *dst) noexcept {
	memcpy(dst, g_sha512_hinit, sizeof(g_sha512_hinit));
}

void Blake512::HashBlock(void *dst, const byte *src, uint64_t counter) noexcept {
	CalcHashBlock<uint64_t, 16>(dst, src, counter, Salt, g_blake512_c, counter, 0);
}

}} // Ext::Crypto::



/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "salsa20.h"


using namespace Ext;

extern "C" {

const byte g_salsaIndices[32][4] = {
	{ 4,  0,  12, 7 },
	{ 9,  5,  1,  7 },
	{ 14, 10, 6,  7 },
	{ 3,  15, 11, 7 },
	
	{ 8,  4,  0,  9 },
	{ 13, 9,  5,  9 },
	{ 2,  14, 10, 9 },
	{ 7,  3,  15, 9 },
	
	{ 12, 8,  4,  13 },
	{ 1,  13, 9,  13 },
	{ 6,  2,  14, 13 },
	{ 11, 7,  3,  13 },
	
	{ 0,  12, 8,  18 },
	{ 5,  1,  13, 18 },
	{ 10, 6,  2,  18 },
	{ 15, 11, 7,  18 },
 
	
	{ 1,  0,  3,  7 },
	{ 6,  5,  4,  7 },
	{ 11, 10, 9,  7 },
	{ 12, 15, 14, 7 },
	
	{ 2,  1,  0,  9 },
	{ 7,  6,  5,  9 },
	{ 8,  11, 10, 9 },
	{ 13, 12, 15, 9 },
	
	{ 3,  2,  1,  13 },
	{ 4,  7,  6,  13 },
	{ 9,  8,  11, 13 },
	{ 14, 13, 12, 13 },
	
	{ 0,  3,  2,  18 },
	{ 5,  4,  7,  18 },
	{ 10, 9,  8,  18 },
	{ 15, 14, 13, 18 },
};


} // "C"


namespace Ext { namespace Crypto {

static void SalsaQuarterRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
	c ^= _rotl((b ^= _rotl(a + d, 7))  + a, 9);
	a ^= _rotl((d ^= _rotl(c + b, 13)) + c, 18);
}

void Salsa20Core(uint32_t dst[16], int rounds) noexcept {
	uint32_t w[16];
	memcpy(w, dst, sizeof w);

	for (int i=0; i<rounds; ++i) {
		SalsaQuarterRound(w[0], w[4], w[8], w[12]);
		SalsaQuarterRound(w[5], w[9], w[13], w[1]);
		SalsaQuarterRound(w[10], w[14], w[2], w[6]);
		SalsaQuarterRound(w[15], w[3], w[7], w[11]);

		std::swap(w[1], w[4]);
		std::swap(w[2], w[8]);
		std::swap(w[3], w[12]);
		std::swap(w[6], w[9]);
		std::swap(w[7], w[13]);
		std::swap(w[11], w[14]);
	}

	for (size_t i=0; i<size(w); ++i)
		dst[i] += w[i];
}

static void ChaChaQuarterRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
	b = _rotl(b ^ (c += d = _rotl(d ^ (a += b), 16)), 12);
	b = _rotl(b ^ (c += d = _rotl(d ^ (a += b),  8)), 7);
}

void ChaCha20Core(uint32_t dst[16], int rounds) noexcept {	
	uint32_t w[16];
	memcpy(w, dst, sizeof w);

	for (; rounds; rounds-=2) {
		ChaChaQuarterRound(w[0], w[4], w[8], w[12]);
		ChaChaQuarterRound(w[1], w[5], w[9], w[13]);
		ChaChaQuarterRound(w[2], w[6], w[10], w[14]);
		ChaChaQuarterRound(w[3], w[7], w[11], w[15]);
		ChaChaQuarterRound(w[0], w[5], w[10], w[15]);
		ChaChaQuarterRound(w[1], w[6], w[11], w[12]);
		ChaChaQuarterRound(w[2], w[7], w[8], w[13]);
		ChaChaQuarterRound(w[3], w[4], w[9], w[14]);
	}

	for (size_t i=0; i<size(w); ++i)
		dst[i] += w[i];
}


void VectorMix(PFN_Salsa pfn, uint32_t x[][16], uint32_t tmp[][16], int r, int rounds) noexcept {
	for (int i=0; i<2*r; ++i) {
		VectorXor(x[i], x[(i-1 + 2*r) % (2*r)]);
		pfn(x[i], rounds);
		memcpy(tmp[(i & 1)*r + i/2], x[i], sizeof(x[i]));
	}
	memcpy(x, tmp, 2*r*sizeof(x[0]));
}




}} // Ext::Crypto::



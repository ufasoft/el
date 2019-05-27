/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

// RIPEMD-160 implementation

#include <el/ext.h>

#include "hash.h"

#if UCFG_USE_OPENSSL
#	include "ext-openssl.h"
#	include <openssl/ripemd.h>
#	include <openssl/sha.h>
#endif

using namespace Ext;

namespace Ext { namespace Crypto {

const uint32_t g_repemd160_hinit[5] = {
	0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
};

void RIPEMD160::InitHash(void *dst) noexcept {
	memcpy(dst, g_repemd160_hinit, sizeof(g_repemd160_hinit));
}

static const uint8_t SL[80] = {
	11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8,
	7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12,
	11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5,
	11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12,
	9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6
};

static const uint8_t SR[80] = {
	8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6,
	9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11,
	9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5,
	15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8,
	8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11
};

static const uint8_t RL[80] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8,
	3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12,
	1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2,
	4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13
};

static const uint8_t RR[80] = {
	5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12,
	6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2,
	15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13,
	8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14,
	12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11
};

static const uint32_t KL[5] = { 0,			0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xA953FD4E },
					KR[5] = { 0x50A28BE6,	0x5C4DD124, 0x6D703EF3, 0x7A6D76E9, 0 };

static uint32_t F1(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
static uint32_t F2(uint32_t x, uint32_t y, uint32_t z) { return x & y | ~x & z; }
static uint32_t F3(uint32_t x, uint32_t y, uint32_t z) { return (x | ~y) ^ z; }
static uint32_t F4(uint32_t x, uint32_t y, uint32_t z) { return z & x | ~z & y; }
static uint32_t F5(uint32_t x, uint32_t y, uint32_t z) { return (y | ~z) ^ x; }

typedef uint32_t (*PFN_F)(uint32_t, uint32_t, uint32_t);

static const PFN_F s_leftFs[] = { F1, F2, F3, F4, F5 },
	s_rightFs[] = { F5, F4, F3, F2, F1 };

struct Tuple5 {
	uint32_t A, B, C, D, E;
};

static Tuple5 CalcBranch(uint32_t *p, const uint32_t w[], const uint8_t tableR[], const uint8_t tableS[], const uint32_t tableK[], const PFN_F tableF[]) {
	uint32_t a = p[0], b = p[1], c = p[2], d = p[3], e = p[4];
	for (int i = 0; i < 80; ++i) {
		uint32_t t = e + _rotl(a + tableF[i>>4](b, c, d) + w[tableR[i]] + tableK[i>>4] , tableS[i]);
		a = exchange(e, exchange(d, _rotl(exchange(c, exchange(b, t)), 10)));
	}
	Tuple5 r = { a, b, c, d, e };
	return r;
}

void RIPEMD160::HashBlock(void *dst, uint8_t src[256], uint64_t counter) noexcept {
	uint32_t *p = (uint32_t*)dst;
	Tuple5  ll = CalcBranch(p, (const uint32_t*)src, RL, SL, KL, s_leftFs),
			rr = CalcBranch(p, (const uint32_t*)src, RR, SR, KR, s_rightFs);
	p[0] += ll.B + rr.C;
	p[1] += ll.C + rr.D;
	p[2] += ll.D + rr.E;
	p[3] += ll.E + rr.A;
	p[4] += ll.A + rr.B;
	p[0] = exchange(p[1], exchange(p[2], exchange(p[3], exchange(p[4], p[0]))));
}

#if UCFG_USE_OPENSSL

hashval RIPEMD160::ComputeHash(Stream& stm) {
	MemoryStream ms;
	stm.CopyTo(ms);
	return ComputeHash(ms);
}

hashval RIPEMD160::ComputeHash(RCSpan mb) {
	byte buf[20];
	SslCheck(::RIPEMD160(mb.P, mb.Size, buf));
	return hashval(buf, sizeof buf);
}

#endif // UCFG_USE_OPENSSL


}} // Ext::Crypto



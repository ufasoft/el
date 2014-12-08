#include <el/ext.h>

// This file should be included in each module, which uses it because problems with exporting constant data

#include "hash.h"

using namespace Ext;

namespace Ext { namespace Crypto {

const UInt32 g_sha1_hinit[5] ={
	0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};


void SHA1::InitHash(void *dst) noexcept {
	memcpy(dst, g_sha1_hinit, sizeof(g_sha1_hinit));
}

void SHA1::HashBlock(void *dst, const byte *src, UInt64 counter) noexcept {
	UInt32 *p = (UInt32*)dst;
	UInt32 w[16];
	memcpy(w, src, sizeof(w));
	UInt32 a = p[0], b = p[1], c = p[2], d = p[3], e = p[4];

	for (int i=0; i<80; ++i) {
		if (i >= 16)
			w[i & 15] = _rotl(w[(i-3) & 15] ^ w[(i-8) & 15] ^ w[(i-14) & 15] ^ w[i & 15], 1);
		UInt32 t = (i<20 ? (b & c | ~b & d) + 0x5A827999
			: i<40 ? (b ^ c ^ d) + 0x6ED9EBA1
			: i<60 ? (b & c | b & d | c & d) + 0x8F1BBCDC
			: (b ^ c ^ d) + 0xCA62C1D6)
			+ _rotl(a, 5) + e + w[i & 15];
		e = exchange(d, exchange(c, _rotl(exchange(b, exchange(a, t)), 30)));
	}
	p[0] += a; p[1] += b; p[2] += c; p[3] += d; p[4] += e;
}



}} // Ext::Crypto::



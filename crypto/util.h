/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com      ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once 

#ifndef UCFG_USE_OPENSSL
#	define UCFG_USE_OPENSSL 0
#endif


namespace Ext { namespace Crypto {
using namespace std;



template <typename A>
inline void VectorXor(A& d, const A& s) {
	for (size_t i=size(s); i--;)
		d[i] ^= s[i];
}

template <typename T>
inline void VectorXor(T *d, const T *s, size_t c) {
	for (; c--;)
		d[c] ^= s[c];
}

#if UCFG_CPU_X86_X64

void Transpose16x8bytes(__m128i to[8], __m128i from[8]);

inline void VectorXor8(__m128i *d, const __m128i *s) {
	d[0] = _mm_xor_si128(d[0], s[0]);
	d[1] = _mm_xor_si128(d[1], s[1]);
	d[2] = _mm_xor_si128(d[2], s[2]);
	d[3] = _mm_xor_si128(d[3], s[3]);
	d[4] = _mm_xor_si128(d[4], s[4]);
	d[5] = _mm_xor_si128(d[5], s[5]);
	d[6] = _mm_xor_si128(d[6], s[6]);
	d[7] = _mm_xor_si128(d[7], s[7]);
}

inline void VectorXor8(__m128i *d, const __m128i *a, const __m128i *b) {
	d[0] = _mm_xor_si128(a[0], b[0]);
	d[1] = _mm_xor_si128(a[1], b[1]);
	d[2] = _mm_xor_si128(a[2], b[2]);
	d[3] = _mm_xor_si128(a[3], b[3]);
	d[4] = _mm_xor_si128(a[4], b[4]);
	d[5] = _mm_xor_si128(a[5], b[5]);
	d[6] = _mm_xor_si128(a[6], b[6]);
	d[7] = _mm_xor_si128(a[7], b[7]);
}

#endif // UCFG_CPU_X86_X64


}} // Ext::Crypto::

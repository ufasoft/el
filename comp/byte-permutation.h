/*######   Copyright (c) 2017 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com      ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext {

template <int N>
class BytePermutation {
public:
	BytePermutation()
	{}

	BytePermutation(const uint8_t init[N]) {
		memcpy(m_perm, init, N);
	}

	static BytePermutation Identity() {
		BytePermutation r;
		for (size_t i = 0; i < N; ++i)
			r.m_perm[i] = (byte)i;
		return r;
	}

	bool operator==(const BytePermutation<N>& x) const
	{
		return !memcmp(m_perm, x.m_perm, N);
	}

	BytePermutation<N> operator!() const {
		BytePermutation<N> r;
		for (int i=0; i < N; ++i)
			r.m_perm[m_perm[i]] = (uint8_t)i;
		return r;
	}

	BytePermutation<N> operator*(const BytePermutation<N>& x) {
		BytePermutation<N> r;
		for (int i = 0; i < N; ++i)
			r.m_perm[i] = x.m_perm[m_perm[i]];
		return r;
	}

	void Apply(uint8_t to[N], const uint8_t from[N]) const {
		for (int i = 0; i < N; ++i)
			to[i] = from[m_perm[i]];
	}
private:
	uint8_t m_perm[N];
};


BytePermutation<128> Transposition128bytesBy8();

#if UCFG_CPU_X86_X64

void Transpose16x8bytes(__m128i to[8], __m128i from[8]);


#endif // UCFG_CPU_X86_X64

} // Ext::

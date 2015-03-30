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


}} // Ext::Crypto::

/*######   Copyright (c) 2015      Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

// Password-based Key Derivation Function
// RFC 2898 implementation

#include <el/ext.h>

#include "hash.h"


namespace Ext { namespace Crypto {

Blob PBKDF2(PseudoRandomFunction& prf, const ConstBuf& password, const ConstBuf& salt, uint32_t c, size_t dkLen) {
	size_t hlen = prf.HashSize();
	if (dkLen > uint64_t(hlen) * uint32_t(0xFFFFFFFF))
		Throw(ExtErr::DerivedKeyTooLong);
	uint32_t iBe = 0;
	Blob salt_i = salt + ConstBuf(&iBe, 4);
	Blob r(nullptr, dkLen);
	for (uint32_t i=1, n=uint32_t((dkLen + hlen - 1) / hlen); i<=n; ++i) {
		memcpy(salt_i.data()+salt.Size, &(iBe = htobe(i)), 4);
		hashval u = prf(password, salt_i), rh = u;
		for (uint32_t j=1; j<c; ++j)
			VectorXor(rh.data(), (u = prf(password, u)).data(), hlen);
		memcpy(r.data() + (i-1)*hlen, rh.data(), (min)(hlen, r.Size - (i-1)*hlen));
	}
	return r;
}



}} // Ext::Crypto::


#include <el/ext.h>

#include "hash.h"

#if UCFG_IMP_SHA3=='S'
#	include <sphlib/sph-config.h>			//!!!
#	include <sphlib/sph_keccak.h>
#endif


using namespace Ext;

namespace Ext { namespace Crypto {

hashval SHA3<256>::ComputeHash(RCSpan mb) {
#if UCFG_IMP_SHA3=='S'
	uint32_t hash[8];

    sph_keccak256_context ctx;
    sph_keccak256_init(&ctx);
    sph_keccak256(&ctx, mb.data(), mb.size());
    sph_keccak256_close(&ctx, hash);
	return hashval((const uint8_t*)hash, sizeof hash);
#else
	Throw(E_NOTIMPL);
#endif
}

hashval SHA3<256>::ComputeHash(Stream& stm) {
	MemoryStream ms;
	stm.CopyTo(ms);
	return ComputeHash(ms.AsSpan());
}




}} // Ext::Crypto::



#include <el/ext.h>

#include "hash.h"

#if UCFG_IMP_SHA3=='S'
#	include "sph-config.h"			//!!!
#	include <sphlib/sph_keccak.h>
#endif


using namespace Ext;

namespace Ext { namespace Crypto {

hashval SHA3<256>::ComputeHash(const ConstBuf& mb) {
#if UCFG_IMP_SHA3=='S'
	UInt32 hash[8];

    sph_keccak256_context ctx;
    sph_keccak256_init(&ctx);
    sph_keccak256(&ctx, mb.P, mb.Size);
    sph_keccak256_close(&ctx, hash);
	return hashval((const byte*)hash, sizeof hash);
#else
	Throw(E_NOTIMPL);
#endif
}

hashval SHA3<256>::ComputeHash(Stream& stm) {
	MemoryStream ms;
	stm.CopyTo(ms);
	return ComputeHash(ConstBuf(ms));
}




}} // Ext::Crypto::



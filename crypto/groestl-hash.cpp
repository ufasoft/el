/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

// Implementation if Grøstl Hash function.
// http://groestl.info

#include <el/ext.h>

#include "hash.h"

#define SPH_NO_64 0

#define SPH_BIG_ENDIAN 0
#define SPH_GROESTL_LITTLE_ENDIAN 1


#define SPH_I386_GCC 0
#define SPH_AMD64_GCC 0
#define SPH_SPARCV9_GCC 0

#define SPH_SMALL_FOOTPRINT 1

#if !UCFG_64
//#	define 0
#endif

#pragma warning(disable: 4242 4244)

extern "C" {
#include <sphlib/sph_groestl.h>
}

using namespace Ext;


namespace Ext { namespace Crypto {

hashval Groestl512Hash::ComputeHash(const ConstBuf& mb) {
	sph_groestl512_context ctx;
	sph_groestl512_init(&ctx);
	sph_groestl512(&ctx, mb.P, mb.Size);
	hashval r(64);
	sph_groestl512_close(&ctx, r.data());
	return r;
}


}} // Ext::Crypto::



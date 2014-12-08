#include <el/ext.h>

#include "ext-openssl.h"
#include "hash.h"

#include <openssl/ripemd.h>
#include <openssl/sha.h>


using namespace Ext;

namespace Ext { namespace Crypto {

hashval RIPEMD160::ComputeHash(Stream& stm) {
	MemoryStream ms;
	stm.CopyTo(ms);
	return ComputeHash(ms);
}

hashval RIPEMD160::ComputeHash(const ConstBuf& mb) {
	byte buf[20];
	SslCheck(::RIPEMD160(mb.P, mb.Size, buf));
	return hashval(buf, sizeof buf);
}



}} // Ext::Crypto



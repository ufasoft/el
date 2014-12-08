#include <el/ext.h>

#include "otp.h"
#include "hash.h"


namespace Ext { namespace Crypto {

int GoogleAuthenticatorCode(const ConstBuf& key, int tim) {
	uint64_t text = htobe64(tim);
	SHA1 sha1;
	hashval hv = HMAC(sha1, key, ConstBuf(&text, 8));
	int n = hv.constData()[hv.size()-1] & 0xF;
	UInt32 h = 0x7FFFFFFF & ((hv[n]<<24) | (hv[n+1]<<16) | (hv[n+2]<<8) | hv[n+3]);
	return h % 1000000;
}


bool GAuthVerify(RCString base32Key, int code) {
	int tim = int(time(0) / 30) + 1;
	Blob key = Convert::FromBase32String(base32Key);
	for (int i=0; i<10; ++i, --tim) {
		if (code == GoogleAuthenticatorCode(key, tim))
			return true;
	}
	return false;
}




}} // Ext::Crypto::



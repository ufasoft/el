#pragma once


namespace Ext { namespace Crypto {
using namespace std;

class DsaBase : public InterlockedObject {
public:
	virtual Blob SignHash(RCSpan hash) =0;
	virtual bool VerifyHash(RCSpan hash, RCSpan signature) =0;
};


}} // Ext::Crypto::


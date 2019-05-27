#pragma once

#include "sign.h"

namespace Ext { namespace Crypto {

ENUM_CLASS(CngKeyBlobFormat) {
	OSslEccPrivateBlob,
	OSslEccPublicBlob,
	OSslEccPrivateBignum,
	OSslEccPublicCompressedBlob,
	OSslEccPublicUncompressedBlob,
	OSslEccPrivateCompressedBlob
} END_ENUM_CLASS(CngKeyBlobFormat);

class CngKey {
public:
	void *m_pimpl;

	CngKey()
		:	m_pimpl(0)
	{}

	CngKey(const CngKey& key);
	~CngKey();
	CngKey& operator=(const CngKey& key);
	Blob Export(CngKeyBlobFormat format) const;
	static CngKey AFXAPI Import(RCSpan mb, CngKeyBlobFormat format);
protected:
	CngKey(void *pimpl)
		:	m_pimpl(pimpl)
	{}

	friend class Dsa;
	friend class ECDsa;
};

class Dsa : public DsaBase {
public:
	CngKey Key;
protected:
	Dsa(void *keyImpl)
		:	Key(keyImpl)
	{}

	Dsa(const CngKey& key)
		:	Key(key)
	{}
};

class ECDsa : public Dsa {
	typedef Dsa base;
public:
	ECDsa(int keySize = 521);

	ECDsa(const CngKey& key)
		:	base(key)
	{
	}

	Blob SignHash(RCSpan hash) override;
	bool VerifyHash(RCSpan hash, RCSpan signature) override;
};

ptr<Dsa> CreateECDsa();


}} // Ext::Crypto::



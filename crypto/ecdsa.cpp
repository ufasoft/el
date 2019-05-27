/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <el/num/num.h>

#include "ecdsa.h"

#ifdef _AFXDLL
#	pragma comment(lib, "openssl")
#endif


#define OPENSSL_NO_SCTP
#include <openssl/ec.h>
#include <openssl/rsa.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>

#include "ext-openssl.h"

using namespace Ext;
using namespace Ext::Num;

namespace Ext { namespace Crypto {


class EcKey {
	typedef EcKey class_type;
public:
	EcKey(int nid) {
		SslCheck(m_key = ::EC_KEY_new_by_curve_name(nid));
	}

	~EcKey() {
		::EC_KEY_free(m_key);
	}

	EcKey(const EcKey& ecKey) {
		SslCheck(::EC_KEY_up_ref(m_key = ecKey.m_key));
	}

	EcKey& operator=(const EcKey& ecKey) {
		::EC_KEY_free(m_key);
		SslCheck(::EC_KEY_up_ref(m_key = ecKey.m_key));
		return *this;
	}

	const EC_GROUP *get_Group() const {
		return ::EC_KEY_get0_group(m_key);
	}
	DEFPROP_GET(const EC_GROUP *, Group);
private:
	EC_KEY *m_key;
};

class EcPoint {
public:
	EcPoint(const EC_GROUP *group) {
		SslCheck(m_point = ::EC_POINT_new(m_group = group));
	}

	EcPoint(const EcPoint& ecPoint) {
		SslCheck(m_point = ::EC_POINT_dup(ecPoint.m_point, m_group = ecPoint.m_group));
	}

	~EcPoint() {
		::EC_POINT_free(m_point);
	}

	EcPoint& operator=(const EcPoint& ecPoint) {
		::EC_POINT_free(m_point);
		SslCheck(m_point = ::EC_POINT_dup(ecPoint.m_point, m_group = ecPoint.m_group));
		return *this;
	}

	operator const EC_POINT *() const {
		return m_point;
	}

	void Mul(const OpensslBn& bn) {
		BnCtx bnCtx;
		SslCheck(::EC_POINT_mul(m_group, m_point, bn, 0, 0, bnCtx));
	}
private:
	EC_POINT *m_point;
	const EC_GROUP *m_group;
};


static inline EVP_PKEY *ToKey(const CngKey& ck) {
	return (EVP_PKEY*)ck.m_pimpl;
}

static inline EC_KEY *ToEcKey(const CngKey &ck) {
	return EVP_PKEY_get0_EC_KEY((EVP_PKEY *) ck.m_pimpl);
}

/*!!!
CngKey::CngKey() {
	m_pimpl = ::EC_KEY_new_by_curve_name(NID_secp256k1);
}*/

CngKey::CngKey(const CngKey& key)
	:	m_pimpl(0)
{
	//!!!SslCheck(m_pimpl = ::EC_KEY_dup(ToKey(key)));
	operator=(key);
}

CngKey::~CngKey() {
	::EVP_PKEY_free(ToKey(_self));
}

CngKey& CngKey::operator=(const CngKey& key) {
	if (this != &key) {
		if (m_pimpl) {
			::EVP_PKEY_free(ToKey(_self));
			m_pimpl = 0;
		}
		auto pFromKey = ToKey(key);
		SslCheck(::EVP_PKEY_up_ref(pFromKey));
		m_pimpl = pFromKey;
	}
	/*!!!

	EVP_PKEY *pMyKey = ::EVP_PKEY_new(),
		*pKey = ToKey(key);
	SslCheck(m_pimpl = pMyKey);
	SslCheck(::EVP_PKEY_set_type(pMyKey, pKey->type));
	pMyKey->pkey.ec = ::EC_KEY_new_by_curve_name(NID_secp256k1);
	SslCheck(::EVP_PKEY_copy_parameters(pMyKey, pKey));
	*/

//	SslCheck(::EC_KEY_copy(ToKey(_self), ToKey(key)));
	return _self;
}

Blob CngKey::Export(CngKeyBlobFormat format) const {
	EVP_PKEY* key = ToKey(_self);
	Blob r;
	int size;
	uint8_t *buf;
	switch (format) {
	case CngKeyBlobFormat::OSslEccPrivateBlob:
		SslCheck((size = ::i2d_PrivateKey(key, 0)) >= 0);
		r.Size = size;
		SslCheck(::i2d_PrivateKey(key, &(buf = r.data())) == size);
		break;
	case CngKeyBlobFormat::OSslEccPrivateBignum:
		{
			const BIGNUM *bn = ::EC_KEY_get0_private_key(EVP_PKEY_get0_EC_KEY(key));
			SslCheck(bn);
			r.Size = BN_num_bytes(bn);
			if (::BN_bn2bin(bn, r.data()) != r.Size)
				Throw(E_FAIL);
		}
		break;
	case CngKeyBlobFormat::OSslEccPublicBlob:
//		SslCheck(size = ::i2o_ECPublicKey(key, 0));
		SslCheck((size = ::i2d_PublicKey(key, 0)) >= 0);
		r.Size = size;
//		SslCheck(::i2o_ECPublicKey(key, &(buf = r.data())) == size);
		SslCheck(::i2d_PublicKey(key, &(buf = r.data())) == size);
		break;
	case CngKeyBlobFormat::OSslEccPublicCompressedBlob:
		{
			CngKey tmp = _self;
			::EC_KEY_set_conv_form(ToEcKey(tmp), POINT_CONVERSION_COMPRESSED);
			r = tmp.Export(CngKeyBlobFormat::OSslEccPublicBlob);
		}
		break;
	case CngKeyBlobFormat::OSslEccPublicUncompressedBlob:
		{
			CngKey tmp = _self;
			::EC_KEY_set_conv_form(ToEcKey(tmp), POINT_CONVERSION_UNCOMPRESSED);
			r = tmp.Export(CngKeyBlobFormat::OSslEccPublicBlob);
		}
		break;
	case CngKeyBlobFormat::OSslEccPrivateCompressedBlob:
		{
			CngKey tmp = _self;
			::EC_KEY_set_conv_form(ToEcKey(tmp), POINT_CONVERSION_COMPRESSED);
			r = tmp.Export(CngKeyBlobFormat::OSslEccPrivateBlob);
		}
		break;
	default:
		Throw(errc::invalid_argument);
	}
	return r;
}

CngKey CngKey::Import(RCSpan mb, CngKeyBlobFormat format) {
	const uint8_t *p = mb.data();
	EVP_PKEY *impl = 0;
	EC_KEY *ec_key;
	CngKey r;
	switch (format) {
	case CngKeyBlobFormat::OSslEccPrivateBlob:
		//!!!SslCheck(::d2i_ECPrivateKey((EC_KEY**)&impl, &p, mb.m_len));
		impl = ::EVP_PKEY_new();
		SslCheck(ec_key = ::EC_KEY_new_by_curve_name(NID_secp256k1));
		ec_key = ::d2i_ECPrivateKey(&ec_key, &p, mb.size());
		SslCheck(ec_key);
		SslCheck(::EVP_PKEY_assign_EC_KEY(impl, ec_key));
		break;
	case CngKeyBlobFormat::OSslEccPrivateBignum:
		{
			r = CngKey(impl = ::EVP_PKEY_new());
			SslCheck(::EVP_PKEY_set_type(impl, EVP_PKEY_EC));
			SslCheck(ec_key = ::EC_KEY_new_by_curve_name(NID_secp256k1));
			OpensslBn bn = OpensslBn::FromBinary(mb);
			EcPoint ecPoint(::EC_KEY_get0_group(ec_key));
			ecPoint.Mul(bn);
			SslCheck(::EC_KEY_set_private_key(ec_key, bn));
			SslCheck(::EC_KEY_set_public_key(ec_key, ecPoint));
		}
		break;
	case CngKeyBlobFormat::OSslEccPublicBlob:
		//!!!SslCheck(impl = ::d2i_PublicKey(EVP_PKEY_EC, &impl, &p, mb.m_len));
		r = CngKey(impl = ::EVP_PKEY_new());
		SslCheck(ec_key = ::EC_KEY_new_by_curve_name(NID_secp256k1));
		SslCheck(::o2i_ECPublicKey(&ec_key, &p, mb.size()));
		SslCheck(::EVP_PKEY_assign_EC_KEY(impl, ec_key));
		break;
	case CngKeyBlobFormat::OSslEccPublicCompressedBlob:
		r = CngKey(impl = ::EVP_PKEY_new());
		SslCheck(ec_key = ::EC_KEY_new_by_curve_name(NID_secp256k1));
		SslCheck(::o2i_ECPublicKey(&ec_key, &p, mb.size()));
		SslCheck(::EVP_PKEY_assign_EC_KEY(impl, ec_key));
		::EC_KEY_set_conv_form(ec_key, POINT_CONVERSION_COMPRESSED);
		break;
	default:
		Throw(errc::invalid_argument);
	}
	return r;
}

ECDsa::ECDsa(int keySize)
	:	base(0)
{
	int nid = keySize == 256
				  ? NID_secp256k1
				  : keySize == 384 ? NID_secp384r1
								   : keySize == 521 ? NID_secp521r1 : 0;
	if (!nid)
		Throw(errc::invalid_argument);
	auto key = ::EC_KEY_new_by_curve_name(nid);
	SslCheck(key);
	EVP_PKEY *impl = ::EVP_PKEY_new();
	SslCheck(::EVP_PKEY_assign_EC_KEY(impl, key));

	SslCheck(Key.m_pimpl = impl);
	SslCheck(::EC_KEY_generate_key(key));
}

Blob ECDsa::SignHash(RCSpan hash) {
	EC_KEY *ecKey = ToEcKey(Key);
	int size = ::ECDSA_size(ecKey);
	SslCheck(size);
	Blob r(0, size);
	unsigned int nSize = 0;
	SslCheck(::ECDSA_sign(0, hash.data(), hash.size(), r.data(), &nSize, ecKey) > 0);
	r.Size = nSize;
	return r;
}

bool ECDsa::VerifyHash(RCSpan hash, RCSpan signature) {
	int rc = ::ECDSA_verify(0, hash.data(), hash.size(), signature.data(), signature.size(), ToEcKey(Key));
	SslCheck(rc >= 0);
	return rc;
}

ptr<Dsa> CreateECDsa() {
	return new ECDsa;
}



}} // Ext::Crypto



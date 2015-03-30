/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once 

#include <el/bignum.h>

#include <openssl/bn.h>

namespace Ext { namespace Crypto {
using namespace Ext;

void SslCheck(bool b);

class OpenSslMalloc {
public:
	OpenSslMalloc();
};

class OpenSslException : public CryptoException {
	typedef CryptoException base;
public:
	OpenSslException(HRESULT hr, RCString s)
		:	base(hr, s)
	{}
};

class BnCtx {
public:
	BnCtx() {
		SslCheck(m_ctx = ::BN_CTX_new());
	}

	~BnCtx() {
		::BN_CTX_free(m_ctx);
	}

	operator BN_CTX*() {
		return m_ctx;
	}
private:
	BN_CTX *m_ctx;

	BnCtx(const BnCtx& bnCtx);
	BnCtx& operator=(const BnCtx& bnCtx);
};

class OpensslBn {
public:
	OpensslBn() {
		SslCheck(m_bn = ::BN_new());
	}

	explicit OpensslBn(const BigInteger& bn);

	OpensslBn(const OpensslBn& bn) {
		SslCheck(::BN_dup(bn.m_bn));
	}


	~OpensslBn() {
		::BN_clear_free(m_bn);
	}

	OpensslBn& operator=(const OpensslBn& bn) {
		::BN_clear_free(m_bn);
		SslCheck(::BN_dup(bn.m_bn));
		return *this;
	}

	operator const BIGNUM *() const {
		return m_bn;
	}
	BIGNUM *Ref() { return m_bn; }	

private:
	BIGNUM *m_bn;

	explicit OpensslBn(BIGNUM *bn)
		:	m_bn(bn)
	{}
public:
	BigInteger ToBigInteger() const;
	static OpensslBn FromBinary(const ConstBuf& cbuf);
	void ToBinary(byte *p, size_t n) const;
};

BigInteger sqrt_mod(const BigInteger& x, const BigInteger& mod);

}} // Ext::Crypto::


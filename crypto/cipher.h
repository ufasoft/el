/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once 

#ifndef UCFG_USE_OPENSSL
#	define UCFG_USE_OPENSSL 1
#endif

#include "util.h"

namespace Ext { namespace Crypto {
using namespace std;

ENUM_CLASS(CipherMode) {
	CBC, CFB, CTS, ECB, OFB
} END_ENUM_CLASS(CipherMode);

ENUM_CLASS(PaddingMode) {
	ANSIX923, ISO10126, None, PKCS7, Zeros
} END_ENUM_CLASS(PaddingMode);

class BlockCipher {
	typedef BlockCipher class_type;
public:	
	Blob IV;
	int BlockSize, KeySize; 	// bits
	int Rounds;
	CipherMode Mode;
	PaddingMode Padding;

	BlockCipher()
		:	BlockSize(128)
		,	KeySize(256)
		,	Rounds(128)
		,	Mode(CipherMode::CBC)
		,	Padding(PaddingMode::PKCS7)
	{}

	ConstBuf get_Key() const { return m_key; }
	void put_Key(const ConstBuf& key) {
		if (key.Size != KeySize/8)
			Throw(errc::invalid_argument);
		m_key = key;
	}
	DEFPROP(ConstBuf, Key);

	virtual Blob Encrypt(const ConstBuf& cbuf);
	virtual Blob Decrypt(const ConstBuf& cbuf);
protected:
	Blob m_key;

	void Pad(byte *tdata, size_t cbPad) const;
	virtual void InitParams() {}
	virtual Blob CalcExpandedKey() const { Throw(E_NOTIMPL); }
	virtual Blob CalcInvExpandedKey() const { Throw(E_NOTIMPL); }

	virtual void EncryptBlock(const ConstBuf& ekey, byte *data) { Throw(E_NOTIMPL); }
	virtual void DecryptBlock(const ConstBuf& ekey, byte *data) { Throw(E_NOTIMPL); }
};

class ExpandedKey {
public:
	explicit ExpandedKey(const ConstBuf& key);
	virtual uint32_t Next();
protected:
	size_t m_i;
	vector<uint32_t> m_key;
private:
	byte m_rcon;
};

class InvExpandedKey : public ExpandedKey {
	typedef ExpandedKey base;
public:
	InvExpandedKey(const ConstBuf& key, int ekeylen, int nb);
	
	uint32_t Next() override { return m_invkey.at(m_invkey.size() - ++m_i); }
private:
	vector<uint32_t> m_invkey;
};

class Aes : public BlockCipher {
	typedef BlockCipher base;
public:
	Aes();

	static std::pair<Blob, Blob> GetKeyAndIVFromPassword(RCString password, const byte salt[8], int nRounds);		// don't set default value for nRound
#if UCFG_USE_OPENSSL
	Blob Encrypt(const ConstBuf& cbuf) override;
	Blob Decrypt(const ConstBuf& cbuf) override;
#endif
protected:
	void InitParams() override {
		switch (KeySize) {
		case 128: Rounds = 10; break;
		case 192: Rounds = 12; break;
		case 256: Rounds = 14; break;
		}
	}

#if !UCFG_USE_OPENSSL
	Blob CalcExpandedKey() const override;
	Blob CalcInvExpandedKey() const override;
#endif // !UCFG_USE_OPENSSL
	void EncryptBlock(const ConstBuf& ekey, byte *data) override;
	void DecryptBlock(const ConstBuf& ekey, byte *data) override;
private:
	void EncDec(const ConstBuf& ekey, uint32_t *data, const byte subTable[256], uint32_t(*pfnMixColumn)(uint32_t));
};


}} // Ext::Crypto::


/*######   Copyright (c) 2014-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

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
protected:
	Blob m_key;
public:
	Blob IV;
	int BlockSize, KeySize; 	// bits
	int Rounds;
	CipherMode Mode;
	PaddingMode Padding;

	BlockCipher()
		: BlockSize(128)
		, KeySize(256)
		, Rounds(128)
		, Mode(CipherMode::CBC)
		, Padding(PaddingMode::PKCS7)
	{}

	Span get_Key() const { return m_key; }
	void put_Key(RCSpan key) {
		if (key.size() != KeySize/8)
			Throw(errc::invalid_argument);
		m_key = key;
	}
	DEFPROP(Span, Key);

	virtual Blob Encrypt(RCSpan cbuf);
	virtual Blob Decrypt(RCSpan cbuf);

	void Pad(uint8_t* tdata, size_t cbPad) const;
	virtual void InitParams() {}
	virtual Blob CalcExpandedKey() const { Throw(E_NOTIMPL); }
	virtual Blob CalcInvExpandedKey() const { Throw(E_NOTIMPL); }

	virtual void EncryptBlock(RCSpan ekey, uint8_t* data) { Throw(E_NOTIMPL); }
	virtual void DecryptBlock(RCSpan ekey, uint8_t* data) { Throw(E_NOTIMPL); }
};

class ExpandedKey {
public:
	explicit ExpandedKey(RCSpan key);
	virtual uint32_t Next();
protected:
	size_t m_i;
	vector<uint32_t> m_key;
private:
	uint8_t m_rcon;
};

class InvExpandedKey : public ExpandedKey {
	typedef ExpandedKey base;
public:
	InvExpandedKey(RCSpan key, int ekeylen, int nb);

	uint32_t Next() override { return m_invkey.at(m_invkey.size() - ++m_i); }
private:
	vector<uint32_t> m_invkey;
};

class Aes : public BlockCipher {
	typedef BlockCipher base;
public:
	Aes();

	static std::pair<Blob, Blob> GetKeyAndIVFromPassword(RCString password, const uint8_t salt[8], int nRounds); // don't set default value for nRound
#if UCFG_USE_OPENSSL
	Blob Encrypt(RCSpan cbuf) override;
	Blob Decrypt(RCSpan cbuf) override;
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
	void EncryptBlock(RCSpan ekey, uint8_t* data) override;
	void DecryptBlock(RCSpan ekey, uint8_t* data) override;
private:
	void EncDec(RCSpan ekey, uint32_t* data, const uint8_t subTable[256], uint32_t (*pfnMixColumn)(uint32_t));
};


}} // Ext::Crypto::


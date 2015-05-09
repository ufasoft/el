/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

// Rijndael / AES implementation

#include <el/ext.h>

#include "cipher.h"
#include "hash.h"
#include "aes-imp.h"

#if UCFG_USE_OPENSSL
#	define OPENSSL_NO_SCTP
#	include <openssl/aes.h>
#	include <openssl/evp.h>

#	include <el/crypto/ext-openssl.h>
#endif


extern "C" {

const byte g_aesSubByte[256] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

const byte *g_aesInvSubByte,
		*g_aesPowers;
const uint16_t *g_aesLogs;


} // "C"

using namespace Ext;

namespace Ext { namespace Crypto {
using namespace std;

void BlockCipher::Pad(byte *tdata, size_t cbPad) const {
	switch (Padding) {
	case PaddingMode::Zeros:
		memset(tdata+BlockSize/8-cbPad, 0, cbPad);
		break;
	case PaddingMode::PKCS7:
		memset(tdata+BlockSize/8-cbPad, cbPad, cbPad);
		break;
	default:
		Throw(E_NOTIMPL);
	}
}

Blob BlockCipher::Encrypt(const ConstBuf& cbuf) {
	InitParams();
	const int cbBlock = BlockSize/8;
	MemoryStream ms;
	Blob block(0, cbBlock);
	byte *tdata = (byte*)alloca(cbBlock);
	if (Mode != CipherMode::ECB) {
		if (IV.Size != cbBlock)
			Throw(errc::invalid_argument);
		block = IV;
	}
	if (Padding == PaddingMode::None) {
		if (cbuf.Size % cbBlock)
			Throw(errc::invalid_argument);
	}		
	Blob ekey = CalcExpandedKey();
	for (size_t pos=0; ; pos+=block.Size) {
		size_t size = (min)(cbuf.Size-pos, size_t(cbBlock));
		if (0 == size && (Padding==PaddingMode::None || Padding==PaddingMode::Zeros))
			break;
		memcpy(tdata, cbuf.P+pos, size);
		if (size < cbBlock)
			Pad(tdata, cbBlock-size);

		switch (Mode) {
		case CipherMode::ECB:
			EncryptBlock(ekey, block.data());
			ms.WriteBuf(block);
			break;
		case CipherMode::OFB:
			EncryptBlock(ekey, block.data());
			VectorXor(tdata, block.constData(), size);
			ms.WriteBuffer(tdata, size);
			break;
		case CipherMode::CBC:
			VectorXor(tdata, block.constData(), size);
			EncryptBlock(ekey, block.data());
			ms.WriteBuf(block);
			break;
		default:
			Throw(E_NOTIMPL);
		}
		if (size < cbBlock)
			break;
	}
	return ms;
}

Blob BlockCipher::Decrypt(const ConstBuf& cbuf) {
	InitParams();
	const int cbBlock = BlockSize/8;
	if (Mode == CipherMode::OFB)
		return Encrypt(cbuf);
	if (cbuf.Size % cbBlock)
		Throw(errc::invalid_argument);
	MemoryStream ms;
	Blob block(0, cbBlock);
	if (Mode != CipherMode::ECB) {
		if (IV.Size != cbBlock)
			Throw(errc::invalid_argument);
	}
	ConstBuf state = IV;
	Blob ekey = CalcInvExpandedKey();
	for (size_t pos=0;  pos<cbuf.Size; pos+=cbBlock) {
		memcpy(block.data(), cbuf.P+pos, cbBlock);
		DecryptBlock(ekey, block.data());
		switch (Mode) {
		case CipherMode::CBC:
			VectorXor(block.data(), state.P, cbBlock);
			state = ConstBuf(cbuf.P+pos, cbBlock);
		case CipherMode::ECB:
			break;
		default:
			Throw(E_NOTIMPL);
		}
		if (Padding!=PaddingMode::None && pos+cbBlock==cbuf.Size) {
			switch (Padding) {
			case PaddingMode::PKCS7:
				{
					byte nPad = block.constData()[cbBlock-1];
					for (int i=0; i<nPad; ++i)
						if (block.constData()[cbBlock-1-i] != nPad)
							Throw(ExtErr::Crypto);									//!!!TODO Must be EXT_Crypto_DecryptFailed
					ms.WriteBuffer(block.constData(), cbBlock-nPad);
				}
				break;
			default:
				Throw(E_NOTIMPL);
			}
		} else
			ms.WriteBuf(block);
	}
	return ms;
}


void InitAesTables() {
	static bool s_b;
	if (s_b)
		return;
	byte *invSubByte = new byte[256],						// possible Memory Leak, but it is small amount of RAM
		*powers = new byte[1024];
	uint16_t *logs = new uint16_t[256];
	memset(powers, 0, 1024);								// elements after 510 are for multiplication by zero
	byte n = 0;
	for (int i=0; i<256; ++i) {
		invSubByte[g_aesSubByte[i]] = (byte)i;
		logs[powers[i] = n = (!i ? 1 : n ^ (n<<1) ^ (n & 0x80 ? 0x1B : 0))] = (byte)i;
	}
	memcpy(powers+255, powers, 255);
	logs[1] = 0;
	logs[0] = 511;			// means -INFINITY

	g_aesInvSubByte = invSubByte;
	g_aesPowers = powers;
	g_aesLogs = logs;
	s_b = true;
}

static byte MulRow(uint32_t a, uint32_t b) {
	byte *pa = (byte*)&a, *pb = (byte*)&b;
	return Mul(pa[0], pb[0]) ^ Mul(pa[1], pb[1]) ^ Mul(pa[2], pb[2]) ^ Mul(pa[3], pb[3]);
}

static uint32_t MulPolynom(uint32_t col, uint32_t p) {
	return MulRow(col, p) | (MulRow(col, _rotl(p, 8)) << 8) | (MulRow(col, _rotl(p, 16)) << 16) | (MulRow(col, _rotl(p, 24)) << 24);	
}

static uint32_t MixColumn(uint32_t col) {
	return MulPolynom(col, 0x01010302);
}

static uint32_t InvMixColumn(uint32_t col) {
	return MulPolynom(col, 0x090D0B0E);
}


ExpandedKey::ExpandedKey(const ConstBuf& key)
	:	m_i(0)
	,	m_rcon(1)
{
	m_key.resize(key.Size / 4);
	for (int i=0; i<key.Size/4; ++i)
		m_key[i] = letoh(*(uint32_t*)(key.P + i*4));
}

uint32_t ExpandedKey::Next() {
	uint32_t t = m_key[(m_i+m_key.size()-1) % m_key.size()];
	if (0 == m_i)
		t = exchange(m_rcon, byte((m_rcon << 1) ^ (m_rcon & 0x80 ? 0x1B : 0)))
			^ (g_aesSubByte[(t >> 8) & 255] |
					  (g_aesSubByte[(t >> 16) & 255] << 8) |
			          (g_aesSubByte[(t >> 24) & 255] << 16) |
			          (g_aesSubByte[t & 255] << 24));
	else if (m_key.size()>6 && m_i==4) {
		uint32_t a = t;
		byte *pt = (byte*)&t, *pa = (byte*)&a;
		pt[0] = g_aesSubByte[pa[0]]; pt[1] = g_aesSubByte[pa[1]]; pt[2] = g_aesSubByte[pa[2]]; pt[3] = g_aesSubByte[pa[3]];
	}
	uint32_t& cur = m_key[exchange(m_i, (m_i+1) % m_key.size())];
	return exchange(cur, cur ^ t);
}

InvExpandedKey::InvExpandedKey(const ConstBuf& key, int ekeylen, int nb)
	:	base(key)
	,	m_invkey(ekeylen)
{
	for (int i=0; i<ekeylen; ++i)
		m_invkey[i] = i>=nb && i<ekeylen-nb ? InvMixColumn(base::Next()) : base::Next();
	m_i = 0;
}

Aes::Aes() {
	Rounds = 14;
	BlockSize = 128;
	InitAesTables();
}

void Aes::EncDec(const ConstBuf& ekey, uint32_t *data, const byte subTable[256], uint32_t(*pfnMixColumn)(uint32_t)) {
	byte *p = (byte*)data;
	const int cbBlock = BlockSize/8;
	const int nb = cbBlock / 4;
	VectorXor(data, (const uint32_t*)ekey.P, nb);
	for (int i=0; i<Rounds; ++i) {
		for (int j=nb*4; j--;)
			p[j] = subTable[p[j]];
		switch (nb) {
		case 4:
			p[1] = exchange(p[5], exchange(p[9], exchange(p[13], p[1])));
			std::swap(p[2], p[10]);	std::swap(p[6], p[14]);
			p[15] = exchange(p[11], exchange(p[7], exchange(p[3], p[15])));
			break;
		case 6:
			p[1] = exchange(p[5], exchange(p[9], exchange(p[13], exchange(p[17], exchange(p[21], p[1])))));
			p[2] = exchange(p[10], exchange(p[18], p[2]));	p[6] = exchange(p[14], exchange(p[22], p[6]));
			p[23] = exchange(p[19], exchange(p[15], exchange(p[11], exchange(p[7], exchange(p[3], p[15])))));
			break;
		default:
			Throw(E_NOTIMPL);
		}

		if (i<Rounds-1)
			for (int j=0; j<nb; ++j)
				data[j] = pfnMixColumn(data[j]);
		VectorXor(data, (const uint32_t*)ekey.P + (i+1)*nb, nb);
	}
}

void Aes::EncryptBlock(const ConstBuf& ekey, byte *data) {
	EncDec(ekey, (uint32_t*)data, g_aesSubByte, MixColumn);
}

void Aes::DecryptBlock(const ConstBuf& ekey, byte *data) {
	uint32_t *b = (uint32_t*)data,
		*e = (uint32_t*)(data + BlockSize/8);
	std::reverse(b, e);
	EncDec(ekey, b, g_aesInvSubByte, InvMixColumn);
	std::reverse(b, e);
}

#if UCFG_USE_OPENSSL

class CipherCtx {
public:
	CipherCtx() {
		::EVP_CIPHER_CTX_init(&m_ctx);
	}

	~CipherCtx() {
	    SslCheck(::EVP_CIPHER_CTX_cleanup(&m_ctx));
	}

	operator EVP_CIPHER_CTX*() {
		return &m_ctx;
	}
private:
	EVP_CIPHER_CTX m_ctx;
};

#else

Blob Aes::CalcExpandedKey() const {
	Blob r(0, BlockSize/8*(Rounds+1));
	uint32_t *p = (uint32_t*)r.data();
	ExpandedKey ekey(Key);
	for (int i=0, n=r.Size/4; i<n; ++i)
		p[i] = ekey.Next(); 
	return r;
}

Blob Aes::CalcInvExpandedKey() const {
	Blob r(0, BlockSize/8*(Rounds+1));
	uint32_t *p = (uint32_t*)r.data();
	InvExpandedKey ekey(Key, r.Size/4, BlockSize/8/4);
	for (int i=0, n=r.Size/4; i<n; ++i)
		p[i] = ekey.Next();
	return r;
}


#endif // UCFG_USE_OPENSSL

pair<Blob, Blob> Aes::GetKeyAndIVFromPassword(RCString password, const byte salt[8], int nRounds) {
	pair<Blob, Blob> r(Blob(0, 32), Blob(0, 16));
	const unsigned char *psz = (const unsigned char*)(const char*)password;

#if UCFG_USE_OPENSSL
	int rc = ::EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha512(), salt, psz, strlen(password), nRounds, r.first.data(), r.second.data());
	if (rc != 32)
		Throw(E_FAIL);
#else
	SHA512 sha;
	Blob data = ConstBuf(psz, strlen(password)) + ConstBuf(salt, 8);
	while (nRounds--)
		data = sha.ComputeHash(data);
	memcpy(r.first.data(), data.constData(), r.first.Size);
	memcpy(r.second.data(), data.constData()+r.first.Size, r.second.Size);
#endif
	return r;
}

#if UCFG_USE_OPENSSL

Blob Aes::Encrypt(const ConstBuf& cbuf) {
	int rlen = cbuf.Size+AES_BLOCK_SIZE, flen = 0;
	Blob r(0, rlen);
    
	CipherCtx ctx;
    SslCheck(::EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), 0, m_key.constData(), IV.constData()));
    SslCheck(::EVP_EncryptUpdate(ctx, r.data(), &rlen, cbuf.P, cbuf.Size));
    SslCheck(::EVP_EncryptFinal_ex(ctx, r.data()+rlen, &flen));
	r.Size = rlen+flen;
	return r;
}

Blob Aes::Decrypt(const ConstBuf& cbuf) {
	int rlen = cbuf.Size, flen = 0;
	Blob r(0, rlen);

	CipherCtx ctx;
    SslCheck(::EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), 0, m_key.constData(), IV.constData()));
    SslCheck(::EVP_DecryptUpdate(ctx, r.data(), &rlen, cbuf.P, cbuf.Size));
    SslCheck(::EVP_DecryptFinal_ex(ctx, r.data()+rlen, &flen));
	r.Size = rlen+flen;
	return r;
}

#endif // UCFG_USE_OPENSSL



}} // Ext::Crypto



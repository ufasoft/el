/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

// Rijndael / AES implementation

#include <el/ext.h>

#include "cipher.h"


using namespace Ext;

namespace Ext { namespace Crypto {
using namespace std;

void BlockCipher::Pad(uint8_t *tdata, size_t cbPad) const {
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

Blob BlockCipher::Encrypt(RCSpan cbuf) {
	InitParams();
	const int cbBlock = BlockSize/8;
	MemoryStream ms;
	uint8_t *tdata = (uint8_t*)alloca(cbBlock), *block = (uint8_t*)alloca(cbBlock);
	if (Mode != CipherMode::ECB) {
		if (IV.Size != cbBlock)
			Throw(errc::invalid_argument);
		memcpy(block, IV.constData(), cbBlock);
	}
	if (Padding == PaddingMode::None) {
		if (cbuf.size() % cbBlock)
			Throw(errc::invalid_argument);
	}
	Blob ekey = CalcExpandedKey();
	for (size_t pos=0; ; pos+=cbBlock) {
		size_t size = (min)(cbuf.size() - pos, size_t(cbBlock));
		if (0 == size && (Padding==PaddingMode::None || Padding==PaddingMode::Zeros))
			break;
		memcpy(tdata, cbuf.data()+pos, size);
		if (size < cbBlock)
			Pad(tdata, cbBlock-size);

		switch (Mode) {
		case CipherMode::ECB:
			EncryptBlock(ekey, tdata);
			ms.WriteBuffer(tdata, cbBlock);
			break;
		case CipherMode::OFB:
			EncryptBlock(ekey, block);
			VectorXor(block, tdata, cbBlock);
			ms.WriteBuffer(block, cbBlock);
			break;
		case CipherMode::CBC:
			VectorXor(tdata, block, cbBlock);
			EncryptBlock(ekey, tdata);
			memcpy(block, tdata, cbBlock);
			ms.WriteBuffer(block, cbBlock);
			break;
		case CipherMode::CFB:
			EncryptBlock(ekey, block);
			VectorXor(block, tdata, cbBlock);
			ms.WriteBuffer(block, cbBlock);
			break;
		default:
			Throw(E_NOTIMPL);
		}
		if (size < cbBlock)
			break;
	}
	return ms.AsSpan();
}

Blob BlockCipher::Decrypt(RCSpan cbuf) {
	InitParams();
	const int cbBlock = BlockSize/8;
	if (Mode == CipherMode::OFB)
		return Encrypt(cbuf);
	if (cbuf.size() % cbBlock)
		Throw(errc::invalid_argument);
	MemoryStream ms;
	uint8_t *tdata = (uint8_t *)alloca(cbBlock),
		*block = (uint8_t*)alloca(cbBlock);
	if (Mode != CipherMode::ECB) {
		if (IV.Size != cbBlock)
			Throw(errc::invalid_argument);
		memcpy(tdata, IV.constData(), cbBlock);
	}
	Span state = IV;
	Blob ekey = Mode==CipherMode::CFB ? CalcExpandedKey() : CalcInvExpandedKey();
	for (size_t pos = 0; pos < cbuf.size(); pos += cbBlock) {
		memcpy(block, cbuf.data() + pos, cbBlock);
		switch (Mode) {
		case CipherMode::ECB:
			DecryptBlock(ekey, block);
			break;
		case CipherMode::CBC:
			DecryptBlock(ekey, block);
			VectorXor(block, state.data(), cbBlock);
			state = Span(cbuf.data() + pos, cbBlock);
			break;
		case CipherMode::CFB:
			EncryptBlock(ekey, tdata);
			VectorXor(block, tdata, cbBlock);
			memcpy(tdata, cbuf.data() + pos, cbBlock);
			break;
		default:
			Throw(E_NOTIMPL);
		}
		if (Padding != PaddingMode::None && pos + cbBlock == cbuf.size()) {
			switch (Padding) {
			case PaddingMode::PKCS7:
                {
				    uint8_t nPad = block[cbBlock - 1];
				    for (int i = 0; i < nPad; ++i)
					    if (block[cbBlock - 1 - i] != nPad)
						    Throw(ExtErr::Crypto); //!!!TODO Must be EXT_Crypto_DecryptFailed
				    ms.WriteBuffer(block, cbBlock - nPad);
			    }
                break;
			default:
				Throw(E_NOTIMPL);
			}
		} else
			ms.WriteBuffer(block, cbBlock);
	}
	return ms.AsSpan();
}




}} // Ext::Crypto



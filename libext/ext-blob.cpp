/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


namespace Ext {
using namespace std;

#if UCFG_BLOB_POLYMORPHIC
CStringBlobBuf *CBlobBufBase::AsStringBlobBuf() {
	Throw(ExtErr::BlobIsNotString);
}

void CBlobBufBase::Attach(CBlobBufBase::Char *bstr) {
	Throw(ExtErr::BlobIsNotString);
}

CBlobBufBase::Char *CBlobBufBase::Detach() {
	Throw(ExtErr::BlobIsNotString);
}
#endif

CStringBlobBuf::CStringBlobBuf(size_t len)
	:	m_apChar(0)
#ifdef _WIN64
	,	m_pad(0)
#endif
	,	m_size(len)
{
	*(UNALIGNED String::value_type *)((uint8_t *)(this + 1) + len) = 0;
}

CStringBlobBuf::CStringBlobBuf(const void *p, size_t len)
	:	m_apChar(0)
#ifdef _WIN64
	,	m_pad(0)
#endif
	,	m_size(len)
{
	if (p)
		memcpy(this+1, p, len);
	else
		memset(this+1, 0, len);
	*(UNALIGNED String::value_type *)((uint8_t *)(this + 1) + len) = 0;
}

CStringBlobBuf::CStringBlobBuf(size_t len, const void *buf, size_t copyLen)
	:	m_apChar(0)
#ifdef _WIN64
	,	m_pad(0)
#endif
	,	m_size(len)
{
	memcpy(this+1, buf, copyLen);
	memset((uint8_t *)(this + 1) + copyLen, 0, len - copyLen + sizeof(String::value_type));
}

void * AFXAPI CStringBlobBuf::operator new(size_t sz) {
	return Malloc(sz + sizeof(String::value_type));
}

void * AFXAPI CStringBlobBuf::operator new(size_t sz, size_t len, bool) {
	if ((ssize_t)len < 0)
		Throw(errc::invalid_argument);
	return Malloc(sz + len + sizeof(String::value_type));
}

CStringBlobBuf *CStringBlobBuf::Clone() {
	return new(m_size, false) CStringBlobBuf(this+1, m_size);
}

CStringBlobBuf *CStringBlobBuf::SetSize(size_t size) {
	CStringBlobBuf *d;
	size_t copyLen = min(size, size_t(m_size));
	if (m_aRef == 1) {
		if (m_apChar.load())
			free(m_apChar.exchange(nullptr));
		size_t cbNew = size + sizeof(CStringBlobBuf) + sizeof(String::value_type);
#if UCFG_HAS_REALLOC
		d = (CStringBlobBuf*)Realloc(this, cbNew);
#else
		memcpy((d = (CStringBlobBuf*)Malloc(cbNew)), this, std::min((size_t)m_size+sizeof(CStringBlobBuf)+sizeof(String::value_type), cbNew));
		free(this);
#endif
		d->m_size = size;	
		memset((uint8_t *)(d + 1) + copyLen, 0, size - copyLen + sizeof(String::value_type));
		return d;
	} else {
		d = new (size, false) CStringBlobBuf(size, this + 1, copyLen);
		Release(this);
		return d;
	}
}

static CStringBlobBuf *s_emptyStringBlobBuf;

static CStringBlobBuf *CreateStringBlobBuf() {
	return s_emptyStringBlobBuf = new CStringBlobBuf();
}

CStringBlobBuf *CStringBlobBuf::RefEmptyBlobBuf() {
	CStringBlobBuf *r = s_emptyStringBlobBuf;
	if (!r)
		r = CreateStringBlobBuf();		
	r->AddRef();
	return r;
}

Blob::Blob(const Blob& blob) noexcept
	:	m_pData(blob.m_pData)
{
	if (m_pData)
		m_pData->AddRef();
}

Blob::Blob(const void *buf, size_t len) {
	m_pData = new(len, false) CStringBlobBuf(buf, len);
}

Blob::Blob(const ConstBuf& mb) {
	m_pData = new(mb.Size, false) CStringBlobBuf(mb.P, mb.Size);
}

Blob::Blob(const Buf& mb) {
	m_pData = new(mb.Size, false) CStringBlobBuf(mb.P, mb.Size);
}

Blob::~Blob() {
	Release(m_pData);
}

void Blob::AssignIfNull(const Blob& val) {
	impl_class *pData = val.m_pData;
	if (!Interlocked::CompareExchange(m_pData, pData, (impl_class*)0) && pData)
		pData->AddRef();
}

Blob& Blob::operator=(const Blob& val) {
	if (m_pData != val.m_pData) {
		Release(m_pData);
		if (m_pData = val.m_pData)
			m_pData->AddRef();
	}
	return _self;
}

Blob& Blob::operator+=(const ConstBuf& mb) {
	size_t prevSize = Size;
	if (mb.P == constData()) {
		Size = prevSize+mb.Size;
		memcpy(data()+prevSize, data(), mb.Size);
	} else {
		Size = prevSize+mb.Size;
		memcpy(data()+prevSize, mb.P, mb.Size);
	}
	return *this;
}


#if defined(_MSC_VER) && defined(_M_IX86)

inline bool mem2equal(const void *x, const void *y, size_t siz) {
	__asm {
		mov esi, x
		mov edi, y
		mov ecx, siz
		xor  eax, eax
		rep cmpsw 
		setz al
	}
}

#endif


bool Blob::operator==(const Blob& blob) const noexcept {
	if (m_pData == blob.m_pData)
		return true;
	if (m_pData) {
		size_t size = Size;
		if (!blob.m_pData || size!=blob.Size)
			return false;
#if defined(_MSC_VER) && defined(_M_IX86)
		return mem2equal((const wchar_t*)constData(), (const wchar_t*)blob.constData(), (size+1)/2);		// because trailing 2 zeros allow compare by 2-bytes
#else
		return !memcmp(constData(), blob.constData(), size);
#endif
	}
	return false;
}

#undef memcmp
#pragma intrinsic(memcmp)	//!!!

bool Blob::operator<(const Blob& blob) const noexcept {
	if (!m_pData)
		return blob.m_pData;
	if (!blob.m_pData)
		return false;
	int n = memcmp(constData(), blob.constData(), min(Size, blob.Size));
	return n ? n<0 : Size<blob.Size;
}

void Blob::Cow() {
	if (m_pData->m_aRef > 1)
		Release(exchange(m_pData, m_pData->Clone()));
}

uint8_t *Blob::data() {
	Cow();
	return (uint8_t*)m_pData->GetBSTR();
}

Blob Blob::FromHexString(RCString s) {
	size_t len = s.length() / 2;
	Blob blob(0, len);
	uint8_t *p = blob.data();
	for (size_t i=0; i<len; ++i)
		p[i] = (uint8_t)stoi(s.substr(i * 2, 2), 0, 16);
	return blob;
}

void Blob::put_Size(size_t size) {
	m_pData = m_pData ? m_pData->SetSize(size) : new(size, false) CStringBlobBuf(size);
}

void Blob::Replace(size_t offset, size_t size, const ConstBuf& mb) {
	Cow();
	uint8_t *data;
	uint8_t newSize = get_Size() + mb.Size - size;
	if (mb.Size >= size) {		
		put_Size(newSize);
		data = (uint8_t*)m_pData->GetBSTR();
		memmove(data + offset + mb.Size, data + offset + size, newSize - offset - mb.Size);
	} else {
		data = (uint8_t*)m_pData->GetBSTR();
		memmove(data + offset + mb.Size, data + offset + size, Size - offset - size);
		put_Size(newSize);
		data = (uint8_t*)m_pData->GetBSTR();
	}
	memcpy(data + offset, mb.P, mb.Size);
}


ostream& __stdcall operator<<(ostream& os, const ConstBuf& cbuf) {
	static const char s_upperHexDigits[] = "0123456789ABCDEF",
 					  s_lowerHexDigits[] = "0123456789abcdef";
	if (!cbuf.P)
		return os << "<#nullptr>";
	const char *digits = os.flags() & ios::uppercase ? s_upperHexDigits : s_lowerHexDigits;
	for (size_t i=0, size=cbuf.Size; i<size; ++i) {
		uint8_t n = cbuf.P[i];
		os.put(digits[n >> 4]).put(digits[n & 15]);
	}
	return os;
}



} // Ext::



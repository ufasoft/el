/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <shlwapi.h>
#endif

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
	: m_apChar(0)
#ifdef _WIN64
	, m_pad(0)
#endif
	, m_size(len)
{
	*(UNALIGNED String::value_type *)((uint8_t *)(this + 1) + len) = 0;
}

CStringBlobBuf::CStringBlobBuf(const void *p, size_t len, bool bZeroContent)
	: m_apChar(0)
#ifdef _WIN64
	, m_pad(0)
#endif
	, m_size(len)
{
	if (p)
		memcpy(this + 1, p, len);
	else if (bZeroContent)
		memset(this + 1, 0, len);
	*(UNALIGNED String::value_type *)((uint8_t *)(this + 1) + len) = 0;
}

CStringBlobBuf::CStringBlobBuf(size_t len, const void *buf, size_t copyLen)
	: m_apChar(0)
#ifdef _WIN64
	, m_pad(0)
#endif
	, m_size(len)
{
	memcpy(this + 1, buf, copyLen);
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
	return new (m_size, false) CStringBlobBuf(this + 1, m_size);
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
	: m_pData(blob.m_pData)
{
	if (m_pData)
		m_pData->AddRef();
}

Blob::Blob(const void *buf, size_t len) {
	m_pData = new(len, false) CStringBlobBuf(buf, len);
}

Blob::Blob(size_t len, nullptr_t) {
	m_pData = new(len, false) CStringBlobBuf(0, len, false);
}

Blob::Blob(RCSpan mb) {
	m_pData = new(mb.size(), false) CStringBlobBuf(mb.data(), mb.size());
}

Blob::Blob(const span<uint8_t>& mb) {
	m_pData = new(mb.size(), false) CStringBlobBuf(mb.data(), mb.size());
}

Blob::~Blob() {
	Release(m_pData);
}

Blob::operator Span() const noexcept {
	if (impl_class *p = m_pData)
		return Span((const uint8_t*)p->GetBSTR(), p->GetSize());
	return Span();
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

Blob& Blob::operator+=(RCSpan mb) {
	size_t prevSize = size();
	if (mb.data() == constData()) {
		resize(prevSize + mb.size());
		memcpy(data() + prevSize, data(), mb.size());
	} else {
		resize(prevSize + mb.size());
		memcpy(data() + prevSize, mb.data(), mb.size());
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
		size_t sz = size();
		if (!blob.m_pData || sz != blob.size())
			return false;
#if defined(_MSC_VER) && defined(_M_IX86)
		return mem2equal((const wchar_t *)constData(), (const wchar_t *)blob.constData(), (sz + 1) / 2); // because trailing 2 zeros allow compare by 2-bytes
#else
		return !memcmp(constData(), blob.constData(), sz);
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
	int n = memcmp(constData(), blob.constData(), min(size(), blob.size()));
	return n ? n < 0 : size() < blob.size();
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
	Blob blob(len, nullptr);
	uint8_t *p = blob.data();
	for (size_t i = 0; i < len; ++i)
		p[i] = (uint8_t)stoi(s.substr(i * 2, 2), 0, 16);
	return blob;
}

void Blob::resize(size_t size) {
	m_pData = m_pData ? m_pData->SetSize(size) : new(size, false) CStringBlobBuf(size);
}

void Blob::Replace(size_t offset, size_t sz, RCSpan mb) {
	Cow();
	uint8_t *data;
	uint8_t newSize = size() + mb.size() - sz;
	if (mb.size() >= sz) {
		resize(newSize);
		data = (uint8_t*)m_pData->GetBSTR();
		memmove(data + offset + mb.size(), data + offset + sz, newSize - offset - mb.size());
	} else {
		data = (uint8_t*)m_pData->GetBSTR();
		memmove(data + offset + mb.size(), data + offset + sz, size() - offset - sz);
		resize(newSize);
		data = (uint8_t*)m_pData->GetBSTR();
	}
	memcpy(data + offset, mb.data(), mb.size());
}

AutoBlobBase::AutoBlobBase(const AutoBlobBase& x, size_t szSpace) {
	if (x.IsInHeap(szSpace)) {
		m_p = (uint8_t *)Malloc(m_size = x.m_size);
		memcpy(m_p, x.m_p, x.m_size);
	} else {
		size_t sz = x.m_p - x.m_space;
		memcpy(m_space, x.m_space, sz);
		m_p = m_space + sz;
	}
}

AutoBlobBase::AutoBlobBase(EXT_RV_REF(AutoBlobBase) rv, size_t szSpace) noexcept {
	m_p = 0;
	if (rv.IsInHeap(szSpace)) {
		m_p = exchange(rv.m_p, nullptr);
		m_size = rv.m_size;
	} else if (rv.m_p) {
		size_t sz = rv.Size(szSpace);
		memcpy(m_space, rv.m_space, sz);
		m_p = m_space + sz;
	}
}

AutoBlobBase::AutoBlobBase(RCSpan s, size_t szSpace) {
	size_t sz = s.size();
	uint8_t* p;
	m_p = sz <= szSpace
		? (p = m_space) + sz
		: p = (uint8_t*)Malloc(m_size = sz);
	memcpy(p, s.data(), sz);
}

void AutoBlobBase::DoAssign(EXT_RV_REF(AutoBlobBase) rv, size_t szSpace) noexcept {
	bool isInHeap = IsInHeap(szSpace);
	if (rv.IsInHeap(szSpace)) {
		if (isInHeap) {
			std::swap(m_p, rv.m_p);
			std::swap(m_size, rv.m_size);
		} else {
			m_p = exchange(rv.m_p, nullptr);
			m_size = rv.m_size;
		}
	} else {
		size_t sz = rv.Size(szSpace);
		size_t mySize = Size(szSpace);
		memcpy(m_space, rv.m_space, sz);
		if (isInHeap) {
			rv.m_size = mySize;
			rv.m_p = m_p;
		}
		m_p = m_space + sz;
	}
}

void AutoBlobBase::DoAssign(RCSpan s, size_t szSpace) {
	const uint8_t* data = s.data();
	if (m_p != data) {
		if (IsInHeap(szSpace))
			free(exchange(m_p, (uint8_t*)0));
		size_t sz = s.size();
		uint8_t* p;
		m_p = sz <= szSpace
			? (p = m_space) + sz
			: p = (uint8_t*)Malloc(m_size = sz);
		memcpy(p, data, sz);
	}
}

void AutoBlobBase::DoAssignIfNull(RCSpan s, size_t szSpace) {
	if (m_p)
		return;
	size_t sz = s.size();
	if (sz <= szSpace) {
		memcpy(m_space, s.data(), sz);
		m_p = m_space + sz;
	} else {
		uint8_t* p = (uint8_t*)memcpy(Malloc(sz), s.data(), m_size = sz);
		if (Interlocked::CompareExchange(m_p, p, (uint8_t*)0))
			free(p);
	}
}

void AutoBlobBase::DoResize(size_t sz, bool bZeroContent, size_t szSpace) {
	size_t szCur = Size(szSpace);
	bool isInHeap = IsInHeap(szSpace);
	if (sz <= szCur) {
		if (isInHeap)
			m_size = sz;
		else
			m_p = m_space + sz;
	} else if (sz <= szSpace) {
		if (isInHeap) {
			memcpy(m_space, m_p, szCur);
			free(m_p);
		}
		if (bZeroContent)
			memset(m_space + szCur, 0, sz - szCur);
		m_p = m_space + sz;
	} else {
		uint8_t* p = (uint8_t*)memcpy(Malloc(sz), Data(szSpace), szCur);
		if (isInHeap)
			free(m_p);
		m_p = p;
		m_size = sz;
		if (bZeroContent)
			memset(p + szCur, 0, sz - szCur);
	}
}


size_t AFXAPI hash_value(const void* key, size_t len) {
#if UCFG_OPT_SIZ && UCFG_WIN32
	size_t r;
	Win32Check(::HashData((BYTE*)key, len, (BYTE*)&r, sizeof(r)));
	return r;
#else
	return MurmurHashAligned2(Span((const uint8_t*)key, len), _HASH_SEED);
#endif
}

} // Ext::


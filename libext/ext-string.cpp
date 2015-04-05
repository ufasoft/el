/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <comutil.h>
#endif

#if UCFG_STDSTL && !UCFG_WCE
#	include <locale>
#endif

namespace Ext {
using namespace std;

String::String(char ch, ssize_t nRepeat) {
	value_type wch;
#ifdef WDM_DRIVER
	wch = ch;
#else
	Encoding& enc = Encoding::Default();
	enc.GetChars(ConstBuf(&ch, 1), &wch, 1);
#endif
	m_blob.Size = nRepeat*sizeof(String::value_type);
	fill_n((value_type*)m_blob.data(), nRepeat, wch);
}

void String::SetAt(size_t nIndex, char ch) {
	value_type wch;
#ifdef WDM_DRIVER
	wch = ch;
#else
	Encoding& enc = Encoding::Default();
	enc.GetChars(ConstBuf(&ch, 1), &wch, 1);
#endif
	SetAt(nIndex, wch);
}

String::String(const char *lpch, ssize_t nLength)
	:	m_blob(nullptr)
{
#ifdef WDM_DRIVER
	Init(nullptr, lpch, nLength);
#else
	Init(&Encoding::Default(), lpch, nLength);
#endif
}

String::String(const char *lpsz)
	:	m_blob(nullptr)
{
#ifdef WDM_DRIVER
	size_t len = lpsz ? strlen(lpsz) : -1;
	Init((const wchar_t*)nullptr, len);
	if (lpsz)
		std::copy(lpsz, lpsz+len, (wchar_t*)m_blob.data());
#else
	Init(&Encoding::Default(), lpsz, lpsz ? strlen(lpsz) : -1);  //!!! May be multibyte
#endif
}

#if defined(_NATIVE_WCHAR_T_DEFINED) && UCFG_STRING_CHAR/8 == __SIZEOF_WCHAR_T__

String::String(const uint16_t *lpch, ssize_t nLength)
	:	m_blob(nullptr)
{
	Init(lpch, nLength);
}

String::String(const uint16_t *lpsz)
	:	m_blob(nullptr)
{
	int len = -1;
	if (lpsz) {
		len = 0;
		for (const uint16_t *p=lpsz; *p; ++p)
			++len;
	}
	Init((const value_type*)nullptr, len);
	if (lpsz)
		std::copy(lpsz, lpsz+len, (value_type*)m_blob.data());
}

void String::Init(unsigned short const *lpch, ssize_t nLength) {
	Init((const value_type*)nullptr, nLength);
	std::copy(lpch, lpch+nLength, (value_type*)m_blob.data());
}
#endif

void String::Init(const value_type *lpch, ssize_t nLength) {
	if (!lpch && nLength == -1)
		m_blob.m_pData = 0; //!!! = new CStringBlobBuf;
	else {
		size_t bytes = nLength * sizeof(value_type);
		m_blob.m_pData = new(bytes) CStringBlobBuf(bytes);
		if (lpch)
			memcpy(m_blob.data(), lpch, bytes);				//!!! was wcsncpy((BSTR)m_blob.get_Data(), lpch, nLength);
	}
}

#if UCFG_STRING_CHAR/8 != __SIZEOF_WCHAR_T__
String::String(const wchar_t *lpsz) {
	size_t len = wcslen(lpsz);
	Init((const value_type*)0, len);				//!!! not correct for UTF-32
	value_type *p = (value_type*)m_blob.data();
	for (int i=0; i<len; ++i)
		p[i] = value_type(lpsz[i]);
}
#endif

String::String(const std::string& s)
	:	m_blob(nullptr)
{
#ifdef WDM_DRIVER
	Init(nullptr, s.c_str(), s.size());
#else
	Init(&Encoding::Default(), s.c_str(), s.size());
#endif
}

String::String(const std::wstring& s)
	:	m_blob(nullptr)
{
	Init((const value_type*)0, s.size());				//!!! not correct for UTF-32
	value_type *p = (value_type*)m_blob.data();
	for (size_t i=0; i<s.size(); ++i)
		p[i] = value_type(s[i]);
}

#if UCFG_COM

String::String(const _bstr_t& bstr)
	:	m_blob(nullptr)
{
	const wchar_t *p = bstr;
	Init(p, p ? wcslen(p) : -1);
}

#endif // UCFG_COM

void String::Init(Encoding *enc, const char *lpch, ssize_t nLength) {
	if (!lpch)
		m_blob.m_pData = 0; //!!!new CStringBlobBuf;
	else {
		int len = 0;
		if (nLength) {
			if (enc) {
				ConstBuf mb(lpch, nLength);
				len = enc->GetCharCount(mb);
				size_t bytes = len * sizeof(value_type);
				m_blob.m_pData = new(bytes) CStringBlobBuf(bytes);
				enc->GetChars(mb, (value_type*)m_blob.data(), len);
			} else {
				Init((const value_type *)nullptr, nLength);
				std::copy(lpch, lpch+nLength, (value_type*)m_blob.data());
			}
		} else {
			size_t bytes = len * sizeof(value_type);
			m_blob.m_pData = new(bytes) CStringBlobBuf(bytes);
		}
	}
}

const char *String::c_str() const {  //!!! optimize
	if (Blob::impl_class *pData = m_blob.m_pData) {
		atomic<char*> &apChar = pData->AsStringBlobBuf()->m_apChar;
		if (!apChar.load()) {
			Encoding& enc = Encoding::Default();
			for (size_t n=(pData->GetSize()/sizeof(value_type))+1, len=n+1;; len<<=1) {
				Array<char> p(len);
				size_t r;
				if ((r=enc.GetBytes((const value_type*)pData->GetBSTR(), n, (byte*)p.get(), len)) < len) {
					char *pch = p.release();
					pch[r] = 0; //!!!? R
					for (char *prev=0; !apChar.compare_exchange_weak(prev, pch);)
						if (prev) {
							free(pch);
							break;
						}						
					break;
				}
			}
		}
		return apChar;
	}
	return 0;
}

String::operator explicit_cast<std::string>() const {
	Blob bytes = Encoding::Default().GetBytes(_self);
	return std::string((const char*)bytes.constData(), bytes.Size);
}

void String::CopyTo(char *ar, size_t size) const {
#ifdef WDM_DRIVER
	Throw(E_NOTIMPL);
#else
	const char *p = _self;
	size_t len = std::min(size-1, strlen(p)-1);
	memcpy(ar, p, len);
	ar[len] = 0;
#endif
}

int String::FindOneOf(RCString sCharSet) const {
	value_type ch;
	for (const value_type *pbeg=_self, *p=pbeg, *c=sCharSet; (ch=*p); ++p)
		if (StrChr(c, ch))
			return int(p-pbeg);  //!!! shoul be ssize_t
	return -1;
}

int String::Replace(RCString sOld, RCString sNew) {
	const value_type *p = _self,
		*q = p + length(),
		*r;
	int nCount = 0,
		nOldLen = (int)sOld.length(),
		nReplLen = (int)sNew.length();
	for (; p<q; p += traits_type::length(p)+1) {
		while (r = StrStr<value_type>(p, sOld)) {
			nCount++;
			p = r+nOldLen;
		}
	}
	if (nCount) {
		int nLen = int(length() + (nReplLen-nOldLen)*nCount);
		value_type *pTarg = (value_type*)alloca(nLen * sizeof(value_type)),
			*z = pTarg;
		for (p = _self; p<q;) {
			if (StrNCmp<value_type>(p, sOld, nOldLen))
				*z++ = *p++;
			else {
				memcpy(z, (const value_type*)sNew, nReplLen * sizeof(value_type));
				p += nOldLen;
				z += nReplLen;
			}
		}
		_self = String(pTarg, nLen);
	}
	return nCount;
}


String::String(value_type ch, ssize_t nRepeat) {
	m_blob.Size = nRepeat * sizeof(value_type);
	fill_n((value_type*)m_blob.data(), nRepeat, ch);
}

String::String(const value_type *lpsz)
	:	m_blob(nullptr)
{
	Init(lpsz, lpsz ? traits_type::length(lpsz) : -1);
}

/*!!!
String::String(const string& s)
	: m_blob(nullptr)
{
const char *p = s.c_str();
Init(p, strlen(p));  //!!! May be multibyte
}*/

String::String(const std::vector<value_type>& vec)
	:	m_blob(nullptr)
{
	Init(vec.empty() ? 0 : &vec[0], vec.size());
}

void String::MakeDirty() noexcept {
	if (m_blob.m_pData)
		free(m_blob.m_pData->AsStringBlobBuf()->m_apChar.exchange(0));
}

#if UCFG_WDM
String::String(UNICODE_STRING *pus)
	:	m_blob(nullptr)
{
	if (pus)
		Init(pus->Buffer, pus->Length/sizeof(wchar_t));
	else
		Init((const wchar_t*)NULL, -1);
}

String::operator UNICODE_STRING*() const {
	if (CStringBlobBuf *pBuf = (CStringBlobBuf*)m_blob.m_pData) {
		UNICODE_STRING *us = &pBuf->m_us;
		us->Length = us->MaximumLength = (USHORT)m_blob.Size;
		us->Buffer = (WCHAR*)(const wchar_t*)_self;
		return us;
	} else
		return nullptr;
}
#endif

const String::value_type& String::operator[](size_type idx) const {
	return (operator const value_type*())[idx];
}

void String::SetAt(size_t nIndex, value_type ch) {
	MakeDirty();
	m_blob.m_pData->GetBSTR()[nIndex] = ch;
}

String& String::operator=(const char *lpsz) {
	return operator=(String(lpsz));
}

String& String::operator=(const value_type *lpsz) {
	MakeDirty();
	if (lpsz) {
		size_t len = traits_type::length(lpsz)*sizeof(value_type);
		if (!m_blob.m_pData)
			m_blob.m_pData = new(len) CStringBlobBuf(len);
		else
			m_blob.Size = len;
		memcpy(m_blob.data(), lpsz, len);
	} else
		Release(exchange(m_blob.m_pData, nullptr));
	return _self;
}

String& String::operator+=(const String& s) {
	MakeDirty();
	m_blob += s.m_blob;
	return _self;
}

void String::CopyTo(value_type *ar, size_t size) const {
	size_t len = std::min(size-1, (size_t)length()-1);
	memcpy(ar, m_blob.constData(), len*sizeof(value_type));
	ar[len] = 0;
}

int String::compare(size_type p1, size_type c1, const value_type *s, size_type c2) const {
	int r = traits_type::compare((const value_type*)m_blob.constData()+p1, s, (min)(c1, c2));
	return r ? r : c1==c2 ? 0 : c1<c2 ? -1 : 1;
}

int String::compare(const String& s) const noexcept {
	return compare(0, length(), (const value_type*)s.m_blob.constData(), s.length());
}

#if UCFG_WDM
static locale s_locale("");

static locale& UserLocale() {
	return s_locale; 
}

#else
static locale& UserLocale() {
	static locale s_locale("");
	return s_locale; 
}

#endif // UCFG_WDM

static locale& s_locale_not_used = UserLocale();	// initialize while one thread, don't use

int String::CompareNoCase(const String& s) const {
	value_type *s1 = (value_type*)m_blob.constData(),
		*s2 = (value_type*)s.m_blob.constData();
	for (size_t len1=length(), len2=s.length();; --len1, --len2) {
		value_type ch1=*s1++, ch2=*s2++;
#if UCFG_USE_POSIX
		ch1 = (value_type)tolower<wchar_t>(ch1, UserLocale());
		ch2 = (value_type)tolower<wchar_t>(ch2, UserLocale());
#else
		ch1 = (value_type)tolower(ch1, UserLocale());
		ch2 = (value_type)tolower(ch2, UserLocale());
#endif
		if (ch1 < ch2)
			return -1;
		if (ch1 > ch2)
			return 1;
		if (!len1)
			return len2 ? -1 : 0;
		else if (!len2)
			return 1;
	}
}

bool String::empty() const noexcept {
	return !m_blob.m_pData || m_blob.Size==0;
}

void String::clear() {		//  noexcept 
	m_blob.Size = 0;		//!!!TODO make this function noexcept
	MakeDirty();
}

String::size_type String::find(value_type ch, size_type pos) const noexcept {
	const value_type *p = _self;
	for (size_t i=pos, e=size(); i<e; ++i)
		if (p[i] == ch)
			return i;
	return npos;
}

String::size_type String::find(const value_type *s, size_type pos, size_type count) const {
	size_t cbS = sizeof(value_type) * count;
	if (count <= size()) {
		const value_type *p = _self,
			*q = s;
		for (size_t i=pos, e=size()-count; i<=e; ++i)
			if (!memcmp(p+i, q, cbS))
				return i;
	}
	return npos;
}

String::size_type String::rfind(value_type ch, size_type pos) const noexcept {
	const value_type *p = _self;
	for (size_t i = pos<length() ? pos+1 : length(); i--;)
		if (p[i] == ch)
			return i;
	return npos;
}

String String::substr(size_type pos, size_type count) const {
	if (pos > size())
		Throw(E_EXT_IndexOutOfRange);
	return !pos && count>=size() ? _self : String((const value_type*)_self + pos, min(count, size()-pos));
}

String String::Right(ssize_t nCount) const {
	if (nCount < 0)
		nCount = 0;
	if (nCount >= length())
		return _self;
	String dest;
	dest.m_blob.Size = nCount * sizeof(value_type);
	memcpy(dest.m_blob.data(), m_blob.constData()+(length() - nCount)*sizeof(value_type), nCount*sizeof(value_type));
	return dest;
}

String String::Left(ssize_t nCount) const {
	if (nCount < 0)
		nCount = 0;
	if (nCount >= length())
		return _self;
	String dest;
	dest.m_blob.Size = nCount * sizeof(value_type);
	memcpy(dest.m_blob.data(), m_blob.constData(), nCount*sizeof(value_type));
	return dest;
}

String String::TrimStart(RCString trimChars) const {
	size_t i;
	for (i=0; i<length(); i++) {
		value_type ch = _self[i];
		if (!trimChars) {
			if (!iswspace(ch)) //!!! must test wchar_t
				break;
		} else if (trimChars.find(ch) == npos)
			break;
	}
	return Right(int(length() - i));
}

String String::TrimEnd(RCString trimChars) const {
	ssize_t i;
	for (i=length(); i--;) {
		value_type ch = _self[(size_t)i];
		if (!trimChars) {
			if (!iswspace(ch)) //!!! must test wchar_t
				break;
		} else if (trimChars.find(ch) == npos)
			break;
	}
	return Left(int(i+1));
}

vector<String> String::Split(RCString separator, size_t count) const {
	String sep = separator.empty() ? " \t\n\r\v\f\b" : separator;
	vector<String> ar;
	for (const value_type *p = _self; count-- && *p;) {
		if (count) {
			size_t n = StrCSpn<value_type>(p, sep);
			ar.push_back(String(p, n));
			if (*(p += n) && !*++p)
				ar.push_back("");
		} else
			ar.push_back(p);
	}
	return ar;
}

String String::Join(RCString separator, const std::vector<String>& value) {
	String r;
	for (size_t i=0; i<value.size(); ++i)
		r += (i ? separator : "") + value[i];
	return r;
}

void String::MakeUpper() {
	MakeDirty();
	for (value_type *p=(value_type*)m_blob.data(); *p; ++p) {
#if UCFG_USE_POSIX
		*p = (value_type)toupper<wchar_t>(*p, UserLocale());
#else
		*p = (value_type)toupper(*p, UserLocale());
#endif
	}
}

void String::MakeLower() {
	MakeDirty();
	for (value_type *p=(value_type*)m_blob.data(); *p; ++p) {
#if UCFG_USE_POSIX
		*p = (value_type)tolower<wchar_t>(*p, UserLocale());
#else
		*p = (value_type)tolower(*p, UserLocale());
#endif
	}
}

void String::Replace(int offset, int size, const String& s) {
	MakeDirty();
	m_blob.Replace(offset*sizeof(value_type), size*sizeof(value_type), s.m_blob);
}

#if UCFG_COM
BSTR String::AllocSysString() const {
	return ::SysAllocString(Bstr);
}

LPOLESTR String::AllocOleString() const {
	LPOLESTR p = (LPOLESTR)CoTaskMemAlloc(m_blob.Size+sizeof(wchar_t));
	memcpy(p, Bstr, m_blob.Size+sizeof(wchar_t));
	return p;
}
#endif

String AFXAPI operator+(const String& string1, const String& string2) {
	size_t len1 = string1.length() * sizeof(String::value_type),
		len2 = string2.length() * sizeof(String::value_type);
	String s;
	s.m_blob.Size = len1+len2;
	memcpy(s.m_blob.data(), string1.m_blob.constData(), len1);
	memcpy(s.m_blob.data()+len1, string2.m_blob.constData(), len2);
	return s;
}

String AFXAPI operator+(const String& string, const char *lpsz) {
	return operator+(string, String(lpsz));
}

String AFXAPI operator+(const String& string, const String::value_type *lpsz) {
	return operator+(string, String(lpsz));
}

String AFXAPI operator+(const String& string, char ch) {
	return operator+(string, String(ch));
}

String AFXAPI operator+(const String& string, String::value_type ch) {
	return operator+(string, String(ch));
}

/*!!!
bool __fastcall operator<(const String& s1, const String& s2)
{
return wcscmp((wchar_t*)s1.m_blob.Data, (wchar_t*)s2.m_blob.Data) < 0;
}*/

bool AFXAPI operator==(const String& s1, const char * s2) { return s1 == String(s2); }
bool AFXAPI operator!=(const String& s1, const String& s2) noexcept { return !(s1==s2); }
bool AFXAPI operator>(const String& s1, const String& s2) noexcept { return s1.compare(s2) > 0; }
bool AFXAPI operator<=(const String& s1, const String& s2) noexcept { return s1.compare(s2) <= 0; }
bool AFXAPI operator>=(const String& s1, const String& s2) noexcept { return s1.compare(s2) >= 0; }

bool AFXAPI operator==(const char * s1, const String& s2) { return String(s1)==s2; }
bool AFXAPI operator!=(const String& s1, const char * s2) { return !(s1==s2); }
bool AFXAPI operator!=(const char * s1, const String& s2) { return !(s1==s2); }
bool AFXAPI operator<(const String& s1, const char *s2) { return s1.compare(s2) < 0; }
bool AFXAPI operator<(const char * s1, const String& s2) { return s2.compare(s1) > 0; }
bool AFXAPI operator>(const String& s1, const char * s2) { return s1.compare(s2) > 0; }
bool AFXAPI operator>(const char * s1, const String& s2) { return s2.compare(s1) < 0; }
bool AFXAPI operator<=(const String& s1, const char * s2) { return s1.compare(s2) <= 0; }
bool AFXAPI operator<=(const char * s1, const String& s2) { return s2.compare(s1) >= 0; }
bool AFXAPI operator>=(const String& s1, const char * s2) { return s1.compare(s2) >= 0; }
bool AFXAPI operator>=(const char * s1, const String& s2) { return s2.compare(s1) <= 0; }

bool AFXAPI operator==(const String& s1, const String::value_type *s2) { return s1==String(s2); }
bool AFXAPI operator==(const String::value_type * s1, const String& s2) { return String(s1)==s2; }
bool AFXAPI operator!=(const String& s1, const String::value_type *s2) { return !(s1==s2); }
bool AFXAPI operator!=(const String::value_type * s1, const String& s2) { return !(s1==s2); }
bool AFXAPI operator<(const String& s1, const String::value_type *s2) { return s1.compare(s2) < 0; }
bool AFXAPI operator<(const String::value_type * s1, const String& s2) { return s2.compare(s1) > 0; }
bool AFXAPI operator>(const String& s1, const String::value_type *s2) { return s1.compare(s2) > 0; }
bool AFXAPI operator>(const String::value_type * s1, const String& s2) { return s2.compare(s1) < 0; }
bool AFXAPI operator<=(const String& s1, const String::value_type *s2) { return s1.compare(s2) <= 0; }
bool AFXAPI operator<=(const String::value_type * s1, const String& s2) { return s2.compare(s1) >= 0; }
bool AFXAPI operator>=(const String& s1, const String::value_type *s2) { return s1.compare(s2) >= 0; }
bool AFXAPI operator>=(const String::value_type * s1, const String& s2) { return s2.compare(s1) <= 0; }



} // Ext::

#if !UCFG_STDSTL || UCFG_WCE

namespace std {

wchar_t AFXAPI toupper(wchar_t ch, const locale& loc) {
#if UCFG_WDM
	return RtlUpcaseUnicodeChar(ch);
#else
	return (wchar_t)(DWORD_PTR)::CharUpperW(LPWSTR(ch));
#endif
}

wchar_t AFXAPI tolower(wchar_t ch, const locale& loc) {
#if UCFG_WDM
	WCHAR dch;
	UNICODE_STRING dest = { 2, 2, &dch },
		src = { 2, 2, &ch };
	NTSTATUS st = RtlDowncaseUnicodeString(&dest, &src, FALSE);
	if (!NT_SUCCESS(st))
		Throw(st);
	return dch;
	//!!!	return RtlDowncaseUnicodeChar(ch);  // not supported in XP ntoskrnl.exe
#else
	return (wchar_t)(DWORD_PTR)::CharLowerW(LPWSTR(ch));
#endif
}

#ifdef WIN32
bool AFXAPI islower(wchar_t ch, const locale& loc) {
	return IsCharLowerW(ch);
}

bool AFXAPI isupper(wchar_t ch, const locale& loc) {
	return IsCharUpperW(ch);
}

bool AFXAPI isalpha(wchar_t ch, const locale& loc) {
	return IsCharAlphaW(ch);
}
#endif // WIN32

} // std::

#endif


#if UCFG_WIN32

class ThreadStrings {
public:
	std::mutex Mtx;

	typedef std::unordered_map<DWORD, Ext::String> CThreadMap;
	CThreadMap Map;

	ThreadStrings() {

	}
};

static ThreadStrings *s_threadStrings;

static bool InitTreadStrings() {
	s_threadStrings = new ThreadStrings;
	return true;
}

extern "C" const unsigned short * AFXAPI Utf8ToUtf16String(const char *utf8) {
	static std::once_flag once;
	std::call_once(once, &InitTreadStrings);

	const unsigned short *r = 0;
	EXT_LOCK (s_threadStrings->Mtx) {
		if (s_threadStrings->Map.size() > 256) {
			for (ThreadStrings::CThreadMap::iterator it=s_threadStrings->Map.begin(), e=s_threadStrings->Map.end(); it!=e;) {
				if (HANDLE h = ::OpenThread(0, FALSE, it->first)) {
					::CloseHandle(h);
					++it;
				} else {
					s_threadStrings->Map.erase(it++);
				}
			}
		}
		r = (const unsigned short*)(const wchar_t*)(s_threadStrings->Map[::GetCurrentThreadId()] = Ext::String(utf8));	
	}
	return r;
}



#endif // UCFG_WIN32

 
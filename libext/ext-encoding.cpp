/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	undef NONLS
#	include <mlang.h>

#	include <el/libext/win32/ext-win.h>
#	if UCFG_COM
#		include <el/libext/win32/ext-com.h>
#	endif
#endif

#pragma warning(disable: 4073)
#pragma init_seg(lib)				// to initialize UTF8 early

/*!!!R
#if UCFG_WIN32
extern "C" {
	const GUID CLSID_CMultiLanguage = __uuidof(CMultiLanguage);
}
#endif */

namespace Ext {
using namespace std;



#if UCFG_WDM
class AnsiPageEncoding : public Encoding {
public:
	size_t GetBytes(const wchar_t *chars, size_t charCount, uint8_t *bytes, size_t byteCount) {
		ANSI_STRING as = { (USHORT)byteCount, (USHORT)byteCount, (PCHAR)bytes };
		UNICODE_STRING us = { USHORT(charCount*sizeof(wchar_t)), USHORT(charCount*sizeof(wchar_t)), (PWCH)chars };
		NTSTATUS st = RtlUnicodeStringToAnsiString(&as, &us, FALSE);
		if (NT_SUCCESS(st))
			return as.Length;
		return 0;
	}
};

#else
ASSERT_CDECL;
#endif

Encoding *Encoding::s_Default;
UTF8Encoding Encoding::UTF8;

#if UCFG_WDM
bool Encoding::t_IgnoreIncorrectChars;
#else
EXT_THREAD_PTR(Encoding) Encoding::t_IgnoreIncorrectChars;
#endif

mutex m_csEncodingMap;
Encoding::CEncodingMap Encoding::s_encodingMap;

Encoding& AFXAPI Encoding::Default() {
	if (!s_Default) {
#if UCFG_WDM
		s_Default = new UTF8Encoding;
#else
#	if UCFG_CODEPAGE_UTF8
		s_Default = new UTF8Encoding;			// never destroy because used in static dtors
#	elif UCFG_WDM
		s_Default = new AnsiPageEncoding;
#	else
		s_Default = new CodePageEncoding(CP_ACP);
#	endif
#endif
	}
	return *s_Default;
}

Encoding::Encoding(int codePage)
	:	CodePage(codePage)
#if UCFG_USE_POSIX
	,	m_iconvTo((iconv_t)-1)
	,	m_iconvFrom((iconv_t)-1)
#endif
{
#if UCFG_USE_POSIX
	if (CodePage) {
		char name[100];
		switch (CodePage) {
			case CP_UTF8:	strcpy(name, "UTF-8"); break;
			case CP_UTF7:	strcpy(name, "UTF-7"); break;
			default:		sprintf(name, "CP%d", (int)CodePage);
		}
		Name = String(name, 0, strlen(name), nullptr);

		const char *unicodeCP = sizeof(String::value_type)==4 ? "UTF-32" : "UTF-16";
		m_iconvTo = ::iconv_open(name, unicodeCP);
		CCheck((intptr_t)m_iconvTo == -1 ? -1 : 0);
		m_iconvFrom = ::iconv_open(unicodeCP, name);
		CCheck((intptr_t)m_iconvFrom == -1 ? -1 : 0);

		GetCharCount(ConstBuf("ABC", 3));			// to skip encoding prefixes;
	}
#endif
}

bool Encoding::SetThreadIgnoreIncorrectChars(bool v) {
	bool prev = t_IgnoreIncorrectChars;
	t_IgnoreIncorrectChars = v ? &UTF8 : nullptr;
	return prev;
}

Encoding *Encoding::GetEncoding(RCString name) {
	String upper = name.ToUpper();
	ptr<Encoding> r;

	CScopedLock<mutex> lck = ScopedLock(m_csEncodingMap);

	if (optional<ptr<Encoding>> o = Lookup(s_encodingMap, upper))
		r = o.value();
	else {
		if (upper == "UTF-8")
			r = new UTF8Encoding;
		else if (upper == "ASCII")
			r = new ASCIIEncoding;
		else if (upper.StartsWith("CP")) {
			int codePage = atoi(upper.substr(2));
			r = new CodePageEncoding(codePage);
		} else {
#if UCFG_COM
			CUsingCOM usingCOM;
			MIMECSETINFO mi;
			OleCheck(CComPtr<IMultiLanguage>(CreateComObject(__uuidof(CMultiLanguage)))->GetCharsetInfo(name.Bstr, &mi));
			r = new CodePageEncoding(mi.uiCodePage);
#else
			Throw(ExtErr::EncodingNotSupported);
#endif
		}
		s_encodingMap[upper] = r;
	}
	return r.get();
}

Encoding *Encoding::GetEncoding(int codepage) {
	return GetEncoding("CP" + Convert::ToString(codepage));
}

#ifdef __FreeBSD__
	typedef const char *ICONV_SECOND_TYPE;
#else
	typedef char *ICONV_SECOND_TYPE;
#endif

size_t Encoding::GetCharCount(RCSpan mb) {
	if (0 == mb.size())
		return 0;
#if UCFG_USE_POSIX
	const char *sp = (char*)mb.P;
	int r = 0;
	for (size_t len = mb.size(); len;) {
		char buf[40] = {0};
		char *dp = buf;
		size_t slen = std::min(len, (size_t)6);
		size_t dlen = sizeof(buf);
		CCheck(::iconv(m_iconvFrom, (ICONV_SECOND_TYPE *)&sp, &slen, &dp, &dlen), E2BIG);
		len = mb.Size - (sp - (const char *)mb.data());
		r += (sizeof(buf) - dlen) / sizeof(String::value_type);
	}
	return r;
#elif !UCFG_WDM
	::SetLastError(0);
	int r = ::MultiByteToWideChar(CodePage, 0, (LPCSTR)mb.data(), mb.size(), 0, 0);
	Win32Check(r, 0);
	return r;
#else
	return 0; //!!!!
#endif
}

size_t Encoding::GetChars(RCSpan mb, String::value_type *chars, size_t charCount) {
#if UCFG_USE_POSIX
	const char *sp = (char*)mb.data();
	size_t len = mb.size();
	char *dp = (char*)chars;
	size_t dlen = charCount*sizeof(String::value_type);
	CCheck(::iconv(m_iconvFrom, (ICONV_SECOND_TYPE*)&sp, &len, &dp, &dlen));
	return (charCount*sizeof(String::value_type)-dlen)/sizeof(String::value_type);
#elif defined(WIN32)
	::SetLastError(0);
	int r = ::MultiByteToWideChar(CodePage, 0, (LPCSTR)mb.data(), mb.size(), chars, charCount);
	Win32Check(r, 0);
	return r;
#else
	return 0; //!!!
#endif
}

vector<String::value_type> Encoding::GetChars(RCSpan mb) {
	vector<String::value_type> vec(GetCharCount(mb));
	if (vec.size()) {
		size_t n = GetChars(mb, &vec[0], vec.size());
		ASSERT(n == vec.size());
	}
	return vec;
}

size_t Encoding::GetByteCount(const String::value_type *chars, size_t charCount) {
	if (0 == charCount)
		return 0;
#if UCFG_USE_POSIX
	const char *sp = (char*)chars;
	int r = 0;
	for (size_t len=charCount*sizeof(String::value_type); len;) {
		char buf[40];
		char *dp = buf;
		size_t dlen = sizeof(buf);
		CCheck(::iconv(m_iconvTo, (ICONV_SECOND_TYPE*)&sp, &len, &dp, &dlen), E2BIG);
		r += (sizeof(buf)-dlen);
	}
	return r;
#elif defined (WIN32)
	::SetLastError(0);
	int r = ::WideCharToMultiByte(CodePage, 0, chars, charCount, 0, 0, 0, 0);
	Win32Check(r, 0);
	return r;
#else
	return 0; //!!!
#endif
}

size_t Encoding::GetBytes(const String::value_type *chars, size_t charCount, uint8_t *bytes, size_t byteCount) {
#if UCFG_USE_POSIX
	const char *sp = (char*)chars;
	size_t len = charCount*sizeof(String::value_type);
	char *dp = (char*)bytes;
	size_t dlen = byteCount;
	CCheck(::iconv(m_iconvTo, (ICONV_SECOND_TYPE*)&sp, &len, &dp, &dlen));
	return byteCount-dlen;
#elif defined(WIN32)
	::SetLastError(0);
	int r = ::WideCharToMultiByte(CodePage, 0, chars, charCount, (LPSTR)bytes, byteCount, 0, 0);
	Win32Check(r, ERROR_INSUFFICIENT_BUFFER);
	return r;
#else
	return 0;
#endif
}

Blob Encoding::GetBytes(RCString s) {
	Blob blob(0, GetByteCount(s));
	size_t n = GetBytes(s, s.length(), blob.data(), blob.size());
	ASSERT(n == blob.size());
	return blob;
}

void UTF8Encoding::Pass(RCSpan mb, UnaryFunction<String::value_type, bool>& visitor) {
	size_t len = mb.size();
	for (const uint8_t *p = mb.data(); len--;) {
		uint8_t b = *p++;
		int n = 0;
		if (b >= 0xFE) {
			if (!t_IgnoreIncorrectChars)
				Throw(ExtErr::InvalidUTF8String);
			n = 1;
			b = '?';
		}
		else if (b >= 0xFC)
			n = 5;
		else if (b >= 0xF8)
			n = 4;
		else if (b >= 0xF0)
			n = 3;
		else if (b >= 0xE0)
			n = 2;
		else if (b >= 0xC0)
			n = 1;
		else if (b >= 0x80) {
			if (!t_IgnoreIncorrectChars)
				Throw(ExtErr::InvalidUTF8String);
			n = 1;
			b = '?';
		}
		String::value_type wc = String::value_type(b & ((1<<(7-n))-1));
		while (n--) {
			if (!len--) {
				if (!t_IgnoreIncorrectChars)
					Throw(ExtErr::InvalidUTF8String);
				++len;
				wc = '?';
				break;
			}
			b = *p++;
			if ((b & 0xC0) != 0x80) {
				if (!t_IgnoreIncorrectChars)
					Throw(ExtErr::InvalidUTF8String);
				wc = '?';
				break;
			} else
				wc = (wc<<6) | (b&0x3F);
		}
		if (!visitor(wc))
			break;
	}
}

void UTF8Encoding::PassToBytes(const String::value_type *pch, size_t nCh, UnaryFunction<uint8_t, bool> &visitor) {
	for (size_t i=0; i<nCh; i++) {
		String::value_type wch = pch[i];
		uint32_t ch = wch; 					// may be 32-bit chars in the future
		if (ch < 0x80) {
			if (!visitor(uint8_t(ch)))
				break;
		} else {
			int n = 5;
			if (ch >= 0x4000000) {
				if (!visitor(uint8_t(0xFC | (ch >> 30))))
					break;
			} else if (ch >= 0x200000) {
				if (!visitor(uint8_t(0xF8 | (ch >> 24))))
					break;
				n = 4;
			} else if (ch >= 0x10000) {
				if (!visitor(uint8_t(0xF0 | (ch >> 18))))
					break;
				n = 3;
			} else if (ch >= 0x800) {
				if (!visitor(uint8_t(0xE0 | (ch >> 12))))
					break;
				n = 2;
			} else {
				if (!visitor(uint8_t(0xC0 | (ch >> 6))))
					break;
				n = 1;
			}
			while (n--)
				if (!visitor(uint8_t(0x80 | ((ch >> (n * 6)) & 0x3F))))
					return;
		}
	}
}

Blob UTF8Encoding::GetBytes(RCString s) {
	struct Visitor : UnaryFunction<uint8_t, bool> {
		MemoryStream ms;
		BinaryWriter wr;

		Visitor()
			:	wr(ms)
		{}

		bool operator()(uint8_t b) {
			wr << b;
			return true;
		}
	} v;
	PassToBytes(s, s.length(), v);
	return v.ms.AsSpan();
}

size_t UTF8Encoding::GetBytes(const String::value_type *chars, size_t charCount, uint8_t *bytes, size_t byteCount) {
	struct Visitor : UnaryFunction<uint8_t, bool> {
		size_t m_count;
		uint8_t *m_p;

		bool operator()(uint8_t ch) {
			if (m_count <= 0)
				return false;
			*m_p++ = ch;
			--m_count;
			return true;
		}
	} v;
	v.m_count = byteCount;
	v.m_p = bytes;
	PassToBytes(chars, charCount, v);
	return v.m_p-bytes;
}

size_t UTF8Encoding::GetCharCount(RCSpan mb) {
	ASSERT(mb.size() <= INT_MAX);

	Cvt::state_type s = Cvt::state_type();
	return (size_t)m_cvt.length(s, (const char *)mb.data(), (const char *)mb.data() + mb.size(), INT_MAX);
}

std::vector<String::value_type> UTF8Encoding::GetChars(RCSpan mb) {
	struct Visitor : UnaryFunction<String::value_type, bool> {
		vector<String::value_type> ar;

		bool operator()(String::value_type ch) {
			ar.push_back(ch);
			return true;
		}
	} v;
	Pass(mb, v);
	return v.ar;
}

size_t UTF8Encoding::GetChars(RCSpan mb, String::value_type *chars, size_t charCount) {
	struct Visitor : UnaryFunction<String::value_type, bool> {
		size_t m_count;
		String::value_type *m_p;

		bool operator()(String::value_type ch) {
			if (m_count <= 0)
				return false;
			*m_p++ = ch;
			--m_count;
			return true;
		}
	} v;
	v.m_count = charCount;
	v.m_p = chars;
	Pass(mb, v);
	return v.m_p-chars;
}


Blob ASCIIEncoding::GetBytes(RCString s) {
	Blob blob(nullptr, s.length());
	for (size_t i=0; i<s.length(); ++i)
		blob.data()[i] = (uint8_t)s[i];
	return blob;
}

size_t ASCIIEncoding::GetBytes(const String::value_type *chars, size_t charCount, uint8_t *bytes, size_t byteCount) {
	size_t r = std::min(charCount, byteCount);
	for (size_t i=0; i<r; ++i)
		bytes[i] = (uint8_t)chars[i];
	return r;
}

size_t ASCIIEncoding::GetCharCount(RCSpan mb) {
	return mb.size();
}

std::vector<String::value_type> ASCIIEncoding::GetChars(RCSpan mb) {
	vector<String::value_type> r(mb.size());
	for (size_t i = 0; i < mb.size(); ++i)
		r[i] = mb[i];
	return r;
}

size_t ASCIIEncoding::GetChars(RCSpan mb, String::value_type *chars, size_t charCount) {
	size_t r = std::min(charCount, mb.size());
	for (size_t i = 0; i < r; ++i)
		chars[i] = mb[i];
	return r;
}


CodePageEncoding::CodePageEncoding(int codePage)
	:	Encoding(codePage)
{
}



} // Ext::


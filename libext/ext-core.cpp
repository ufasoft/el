/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <windows.h>
#endif

#include EXT_HEADER_DYNAMIC_BITSET

using namespace std;
using namespace Ext;

namespace Ext {

int __cdecl PopCount(uint32_t v) {
	return BitOps::PopCount(v);
}

int __cdecl PopCount(uint64_t v) {
	return BitOps::PopCount(uint64_t(v));
}

const uint8_t *Find(RCSpan a, RCSpan b) {
	for (const unsigned char *p = a.data(), *e = p + (a.size() - b.size()), *q; (p <= e) && (q = (const unsigned char *)memchr(p, b[0], e - p + 1)); p = q + 1)
		if (b.size() == 1 || !memcmp(q + 1, b.data() + 1, b.size() - 1))
			return q;
	return 0;
}


#ifdef WIN32

Blob Convert::ToAnsiBytes(wchar_t ch) {
	int n = WideCharToMultiByte(CP_ACP, 0, &ch, 1, 0, 0, 0, 0);
	Blob blob(0, n);
	WideCharToMultiByte(CP_ACP, 0, &ch, 1, (LPSTR)blob.data(), n, 0, 0);
	return blob;
}

#endif

static void ImpIntToStr(char *buf, uint64_t v, int base) {
	char *q = buf;
	do {
		int d = int(v % base);
		*q++ = char(d<=9 ? d+'0' : d-10+'a');
	}
	while (v /= base);
	*q = 0;
	reverse(buf, q);
}

String Convert::ToString(int64_t v, int base) {
	char buf[66];
	char *p = buf;
	if (v < 0) {
		*p++ = '-';
		v = -v;
	}
	ImpIntToStr(p, v, base);
	return buf;
}

String Convert::ToString(uint64_t v, int base) {
	char buf[66];
	ImpIntToStr(buf, v, base);
	return buf;
}

String Convert::ToString(int64_t v, const char *format) {
	char buf[100];
	char sformat[100];
	char typ = *format;
	switch (typ) {
	case 'D':
		typ = 'd';
	case 'd':
	case 'x':
	case 'X':
		if (format[1])
			sprintf(sformat, "%%%s.%s" EXT_LL_PREFIX "%c", format+1, format+1, typ);
		else
			sprintf(sformat, "%%" EXT_LL_PREFIX "%c", typ);
		sprintf(buf, sformat, v);
		return buf;
	default:
		Throw(E_FAIL);
	}
}

uint64_t Convert::ToUInt64(RCString s, int fromBase) {
	uint64_t r = 0;
	const String::value_type *p=s;
	for (String::value_type ch; (ch = *p); ++p) {
		int n;
		if (ch>='0' && ch<='9')
			n = ch-'0';
		else if (ch>='A' && ch<='Z')
			n = ch-'A'+10;
		else if (ch>='a' && ch<='z')
			n = ch-'a'+10;
		else
			Throw(E_FAIL);
		if (n >= fromBase)
			Throw(E_FAIL);
		r = (r * fromBase) + n;
	}
	return r;
}

int64_t Convert::ToInt64(RCString s, int fromBase) {
	size_t siz;
	int64_t r = stoll(s, &siz, fromBase);
	if (siz != s.length())
		Throw(errc::invalid_argument);
	return r;
}

template <typename T>
T CheckBounds(int64_t v) {
	if (v > numeric_limits<T>::max() || v < numeric_limits<T>::min())
		Throw(ExtErr::Overflow);
	return (T)v;
}

template <typename T>
T CheckBounds(uint64_t v) {
#if _HAS_EXCEPTIONS
	if (v > numeric_limits<T>::max())
		throw out_of_range("integer bounds violated");
#endif
	return (T)v;
}

template <typename T>
T CheckBounds(int v) {
#if _HAS_EXCEPTIONS
	if (v > numeric_limits<T>::max())
		throw out_of_range("integer bounds violated");
#endif
	return (T)v;
}

uint32_t Convert::ToUInt32(RCString s, int fromBase) {
	return CheckBounds<uint32_t>(ToUInt64(s, fromBase));
}

uint16_t Convert::ToUInt16(RCString s, int fromBase) {
	return CheckBounds<uint16_t>(stoi(s, 0, fromBase));
}

uint8_t Convert::ToByte(RCString s, int fromBase) {
	return CheckBounds<uint8_t>(stoi(s, 0, fromBase));
}

#if !UCFG_WDM
String Convert::ToString(double d) {
	char buf[40];
	sprintf(buf, "%g", d);
	return buf;
}
#endif

String Convert::MulticharToString(int n) {
	uint64_t ar[2] = { htole((unsigned)n), 0 };
	return strrev((char*)ar);
}

int Convert::ToMultiChar(const char* s) {
	size_t len = strlen(s);
	if (len > sizeof(int))
		return -1;
	int r = 0;
	for (int i = 0; i < len; ++i)
		r = (r << 8) | (unsigned char)s[i];
	return r;
}

MacAddress::MacAddress(RCString s)
	:	m_n64(0)
{
	vector<String> ar = s.Split(":-");
	if (ar.size() != 6)
		Throw(E_FAIL);
	uint8_t *p = (uint8_t*)&m_n64;
	for (size_t i=0; i<ar.size(); i++)
		*p++ = (uint8_t)Convert::ToUInt32(ar[i], 16);
}

void MacAddress::Print(ostream& os) const {
	Span s = AsSpan();
	char buf[20];
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", s[0], s[1], s[2], s[3], s[4], s[5]);
	os << buf;
}

int StreamReader::ReadChar() {
	if (m_prevChar == -1)
		return BaseStream.ReadByte();
	return exchange(m_prevChar, -1);
}

String StreamReader::ReadToEnd() {
	vector<char> v;
	for (int b; (b = BaseStream.ReadByte()) != -1;)
		v.push_back((char)b);
	if (v.empty())
		return String();
	return String(&v[0], 0, v.size(), &Encoding);
}

pair<String, bool> StreamReader::ReadLineEx() {
	pair<String, bool> r(nullptr, false);
	vector<char> v;
	bool bEmpty = true;
	for (int b; (b=ReadChar())!=-1;) {
		bEmpty = false;
		switch (b) {
		case '\r':
			r.second = true;
			if ((m_prevChar=ReadChar()) == '\n')
				m_prevChar = -1;
		case '\n':
			r.second = true;
			goto LAB_OUT;
		default:
			v.push_back((char)b);
		}
	}
LAB_OUT:
	r.first = bEmpty ? nullptr
		: v.empty() ? String()
		: String(&v[0], 0, v.size(), &Encoding);
	return r;
}

void StreamWriter::WriteLine(RCString line) {
	BaseStream.Write(Encoding.GetBytes(line + NewLine));
}

uint64_t ToUInt64AtBytePos(const dynamic_bitset<uint8_t> &bs, size_t pos) {
	ASSERT(!(pos & 7));

	uint64_t r = 0;
#if UCFG_STDSTL
	for (size_t i = 0, e = min(size_t(64), size_t(bs.size() - pos)); i < e; ++i)
		r |= uint64_t(bs[pos + i]) << i;
#else
	size_t idx = pos / bs.bits_per_block;
	const uint8_t *pb = (const uint8_t *)&bs.m_data[idx];
	ssize_t outRangeBits = max(ssize_t(pos + 64 - bs.size()), (ssize_t)0);
	if (0 == outRangeBits)
		return *(uint64_t *)pb;
	size_t n = std::min(size_t(8), (bs.num_blocks() - idx) * bs.bits_per_block / 8);
	for (size_t i = 0; i < n; ++i)
		((uint8_t *)&r)[i] = pb[i];
	r &= uint64_t(-1) >> outRangeBits;
#endif
	return r;
}



} // Ext::


static vector<PFNAtExit> *s_atexits;

static vector<PFNAtExit>& GetAtExit() {
	if (!s_atexits)
		s_atexits = new vector<PFNAtExit>;
	return *s_atexits;
}

extern "C" {

int AFXAPI RegisterAtExit(void (_cdecl*pfn)()) {
	GetAtExit().push_back(pfn);
	return 0;
}

void AFXAPI UnregisterAtExit(void (_cdecl*pfn)()) {
	Remove(GetAtExit(), pfn);
}

void __cdecl MainOnExit() {
	vector<PFNAtExit>& ar = GetAtExit();
	while (!ar.empty()) {
		ar.back()();
		ar.pop_back();
	}
}


#if UCFG_WDM
unsigned int __cdecl bus_space_read_4(int bus, void *addr, int off) {
	if (bus == BUS_SPACE_IO)
		return READ_PORT_ULONG(PULONG((byte*)addr + off));
	else
		return *(volatile ULONG *)((byte*)addr+off);
}

void __cdecl bus_space_write_4(int bus, void *addr, int off, unsigned int val) {
	if (bus == BUS_SPACE_IO)
		WRITE_PORT_ULONG(PULONG((byte*)addr + off), val);
	else
		*(volatile ULONG *)((byte*)addr+off) = val;
}
#endif



} // extern "C"

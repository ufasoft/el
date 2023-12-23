#pragma once

#if UCFG_USE_POSIX
#	ifdef __FreeBSD__
#		include <sys/endian.h>
#	else
#		include <byteswap.h>
#	endif

#	include <unistd.h>
#	include <iconv.h>
#	include <dlfcn.h>
#endif

#if UCFG_USE_PTHREADS

#	if WIN32
#		define HAVE_CONFIG_H 0
#		define HAVE_SIGNAL_H 1
#		define pthread_create STDCALL_pthread_create
#	endif

#	include <sched.h>
#	include <pthread.h>

#	if WIN32
#		undef pthread_create

PTW32_DLLPORT int PTW32_CDECL pthread_create(pthread_t* tid, const pthread_attr_t* attr, void*(__cdecl* start)(void*), void* arg);

#	endif

#endif

#include EXT_HEADER_CODECVT

namespace Ext {

//!!!R ENUM_CLASS(Endian){Big, Little} END_ENUM_CLASS(Endian);

__forceinline uint16_t htole(uint16_t v) { return htole16(v); }
__forceinline int16_t htole(int16_t v) { return int16_t(htole16(uint16_t(v))); }
__forceinline uint32_t htole(uint32_t v) { return htole32(v); }
__forceinline int32_t htole(int32_t v) { return int32_t(htole32(uint32_t(v))); }
__forceinline uint64_t htole(uint64_t v) { return htole64(v); }
__forceinline int64_t htole(int64_t v) { return int64_t(htole64(uint64_t(v))); }
__forceinline uint16_t letoh(uint16_t v) { return le16toh(v); }
__forceinline int16_t letoh(int16_t v) { return int16_t(le16toh(uint16_t(v))); }
__forceinline uint32_t letoh(uint32_t v) { return le32toh(v); }
__forceinline int32_t letoh(int32_t v) { return int32_t(le32toh(uint32_t(v))); }
__forceinline uint64_t letoh(uint64_t v) { return le64toh(v); }
__forceinline int64_t letoh(int64_t v) { return int64_t(le64toh(uint64_t(v))); }

#if UCFG_SEPARATE_LONG_TYPE
__forceinline long letoh(long v) { return letoh(int_presentation<sizeof(long)>::type(v)); }
__forceinline unsigned long letoh(unsigned long v) { return letoh(int_presentation<sizeof(unsigned long)>::type(v)); }
__forceinline long htole(long v) { return htole(int_presentation<sizeof(long)>::type(v)); }
__forceinline unsigned long htole(unsigned long v) { return htole(int_presentation<sizeof(unsigned long)>::type(v)); }
#endif

inline float htole(float v) {
	int_presentation<sizeof(float)>::type vp = htole((int_presentation<sizeof(float)>::type&)(v));
	return (float&)(vp);
}

inline double htole(double v) {
	int_presentation<sizeof(double)>::type vp = htole((int_presentation<sizeof(double)>::type&)(v));
	return (double&)(vp);
}

inline float letoh(float v) {
	int_presentation<sizeof(float)>::type vp = letoh((int_presentation<sizeof(float)>::type&)(v));
	return (float&)(vp);
}

inline double letoh(double v) {
	int_presentation<sizeof(double)>::type vp = letoh((int_presentation<sizeof(double)>::type&)(v));
	return (double&)(vp);
}

inline uint32_t htobe(uint32_t v) { return htobe32(v); }
inline uint16_t htobe(uint16_t v) { return htobe16(v); }
inline uint32_t betoh(uint32_t v) { return be32toh(v); }
inline uint16_t betoh(uint16_t v) { return be16toh(v); }
inline uint64_t htobe(uint64_t v) { return htobe64(v); }
inline uint64_t betoh(uint64_t v) { return be64toh(v); }

inline uint16_t load_little_u16(const uint8_t* p) noexcept {
#if UCFG_CPU_X86_X64
	return *(const uint16_t*)p;
#else
	return p[0] | ((uint16_t)p[1] << 8);
#endif
}

inline void store_little_u16(uint8_t* p, uint16_t v) noexcept {
#if UCFG_CPU_X86_X64
	*(uint16_t*)p = v;
#else
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
#endif
}

inline uint32_t load_little_u24(const uint8_t* p) noexcept {
	return load_little_u16(p) | ((uint32_t)p[2] << 16);
}

inline void store_little_u24(uint8_t* p, uint32_t v) noexcept {
	store_little_u16(p, (uint16_t)v);
	p[2] = (uint8_t)(v >> 16);
}

inline uint32_t load_little_u32(const uint8_t* p) noexcept {
#if UCFG_CPU_X86_X64
	return *(const uint32_t*)p;
#else
	return p[0] | ((uint16_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
#endif
}

inline void store_little_u32(uint8_t* p, uint32_t v) noexcept {
#if UCFG_CPU_X86_X64
	*(uint32_t*)p = v;
#else
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
#endif
}

inline uint64_t load_little_u64(const uint8_t* p) noexcept {
#if UCFG_CPU_X86_X64
	return *(const uint64_t*)p;
#else
	return load_little_u32(p) | ((uint64_t)load_little_u32(p + 4) << 32);
#endif
}

inline void store_little_u64(uint8_t* p, uint64_t v) noexcept {
#if UCFG_CPU_X86_X64
	*(uint64_t*)p = v;
#else
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
	p[4] = (uint8_t)(v >> 32);
	p[5] = (uint8_t)(v >> 40);
	p[6] = (uint8_t)(v >> 48);
	p[7] = (uint8_t)(v >> 56);
#endif
}

template <typename T> class BeInt {
	T m_val;
public:
	BeInt(T v = 0)
		: m_val(htobe(v)) {}

	operator T() const { return betoh(m_val); }

	BeInt& operator=(T v) {
		m_val = htobe(v);
		return *this;
	}
};

typedef BeInt<uint16_t> BeUInt16;
typedef BeInt<uint32_t> BeUInt32;
typedef BeInt<uint64_t> BeUInt64;

#define EXT_NULLPTR_TRACE_LITERAL "#<nullptr>"

inline std::ostream& AFXAPI operator<<(std::ostream& os, const String& s) {
	const char* p = (const char*)s;
	return os << (p ? p : EXT_NULLPTR_TRACE_LITERAL);
}

inline std::wostream& AFXAPI operator<<(std::wostream& os, const String& s) {
	return s == nullptr
		? os << EXT_NULLPTR_TRACE_LITERAL
		: os << (std::wstring)explicit_cast<std::wstring>(s);
}

inline std::ostream& AFXAPI operator<<(std::ostream& os, const wchar_t* s) {
	return os << String(s);
}

inline std::istream& AFXAPI getline(std::istream& is, String& s, char delim = '\n') {
	std::string bs;
	getline(is, bs, delim);
	s = bs;
	return is;
}

} // namespace Ext

__BEGIN_DECLS

inline uint16_t AFXAPI GetLeUInt16(const void* p) { return le16toh(*(const uint16_t UNALIGNED*)p); }
inline uint32_t AFXAPI GetLeUInt32(const void* p) { return le32toh(*(const uint32_t UNALIGNED*)p); }
inline uint64_t AFXAPI GetLeUInt64(const void* p) { return le64toh(*(const uint64_t UNALIGNED*)p); }
inline uint64_t AFXAPI GetBeUInt64(const void* p) { return be64toh(*(const uint64_t UNALIGNED*)p); }

inline void AFXAPI PutLeUInt16(void* p, uint16_t v) { *(uint16_t UNALIGNED*)p = htole16(v); }
inline void AFXAPI PutLeUInt32(void* p, uint32_t v) { *(uint32_t UNALIGNED*)p = htole32(v); }
inline void AFXAPI PutLeUInt64(void* p, uint64_t v) { *(uint64_t UNALIGNED*)p = htole64(v); }

#ifdef _MSC_VER
#	define strrev _strrev
#else
char * __cdecl strrev(char *s);
#endif

__END_DECLS

namespace Ext {
using namespace std;

uint64_t AFXAPI Read7BitEncoded(const uint8_t*& p);
void AFXAPI Write7BitEncoded(uint8_t*& p, uint64_t v);

class Convert {
public:
	static AFX_API Blob AFXAPI FromBase64String(RCString s);
	static AFX_API String AFXAPI ToBase64String(RCSpan mb);
	static AFX_API Blob AFXAPI FromBase32String(RCString s, const uint8_t *table = 0);
	static AFX_API String AFXAPI ToBase32String(RCSpan mb, const char* table = 0);
	static AFX_API uint32_t AFXAPI ToUInt32(RCString s, int fromBase = 10);
	static AFX_API uint64_t AFXAPI ToUInt64(RCString s, int fromBase = 10);
	static AFX_API int64_t AFXAPI ToInt64(RCString s, int fromBase = 10);
	static AFX_API uint16_t AFXAPI ToUInt16(RCString s, int fromBase = 10);
	static AFX_API uint8_t AFXAPI ToByte(RCString s, int fromBase = 10);
	//!!!R	static AFX_API int32_t AFXAPI ToInt32(RCString s, int fromBase = 10);
	static AFX_API String AFXAPI ToString(int64_t v, int base = 10);
	static AFX_API String AFXAPI ToString(uint64_t v, int base = 10);
	static AFX_API String ToIecSuffixedString(uint64_t v, RCString unit = nullptr);
	static AFX_API String AFXAPI ToString(int64_t v, const char* format);
	//!!!	static String AFXAPI ToString(size_t v, int base = 10) { return ToString(uint64_t(v), base); }
	static String AFXAPI ToString(int32_t v, int base = 10) { return ToString(int64_t(v), base); }
	static String AFXAPI ToString(uint32_t v, int base = 10) { return ToString(uint64_t(v), base); }
#if UCFG_SEPARATE_INT_TYPE
	static String AFXAPI ToString(int v, int base = 10) { return ToString(int64_t(v), base); }
	static String AFXAPI ToString(unsigned int v, int base = 10) { return ToString(uint64_t(v), base); }
#endif
#if UCFG_SEPARATE_LONG_TYPE
	static String AFXAPI ToString(long v, int base = 10) { return ToString(int64_t(v), base); }
	static String AFXAPI ToString(unsigned long v, int base = 10) { return ToString(uint64_t(v), base); }
#endif
	static String AFXAPI ToString(int16_t v, int base = 10) { return ToString(int32_t(v), base); }
	static String AFXAPI ToString(uint16_t v, int base = 10) { return ToString(uint32_t(v), base); }
	static String AFXAPI ToString(double d);

	static String AFXAPI MulticharToString(int n);
	static int AFXAPI ToMultiChar(const char *s);
#ifdef WIN32
	static AFX_API Blob AFXAPI ToAnsiBytes(wchar_t ch);
#endif

#if UCFG_COM
	static AFX_API int32_t AFXAPI ToInt32(const VARIANT& v);
	static AFX_API int64_t AFXAPI ToInt64(const VARIANT& v);
	static AFX_API double AFXAPI ToDouble(const VARIANT& v);
	static AFX_API String AFXAPI ToString(const VARIANT& v);
	static AFX_API bool AFXAPI ToBoolean(const VARIANT& v);
#endif
};

class MemStreamWithPosition : public Stream {
protected:
	mutable size_t m_pos;
public:
	MemStreamWithPosition()
		: m_pos(0) {}

	uint64_t get_Position() const override { return m_pos; }
	void put_Position(uint64_t pos) const override { m_pos = (size_t)pos; }
	int64_t Seek(int64_t offset, SeekOrigin origin) const override;
};

class CMemReadStream : public MemStreamWithPosition {
public:
	Span m_mb;

	CMemReadStream(RCSpan mb)
		: m_mb(mb) {}

	uint64_t get_Length() const override { return m_mb.size(); }
	bool Eof() const override { return m_pos == m_mb.size(); }

	void put_Position(uint64_t pos) const override {
		if (pos > m_mb.size())
			Throw(E_FAIL);
		m_pos = (size_t)pos;
	}

	void ReadBufferAhead(void* buf, size_t count) const override;
	size_t Read(void* buf, size_t count) const override;
	void ReadExactly(void* buf, size_t count) const override;
	int ReadByte() const override;
};

class CBlobReadStream : public CMemReadStream {
	typedef CMemReadStream base;

	const Blob m_blob;
public:
	CBlobReadStream(const Blob& blob)
		: base(blob)
		, m_blob(blob) // m_mb now points to the same buffer as m_blob
	{}
};

class CMemWriteStream : public MemStreamWithPosition {
public:
	span<uint8_t> m_mb;

	CMemWriteStream(const span<uint8_t>& mb)
		: m_mb(mb) {}

	uint64_t get_Length() const override { return m_mb.size(); }
	bool Eof() const override { return m_pos == m_mb.size(); }
	void WriteBuffer(const void* buf, size_t count) override;

	void put_Position(uint64_t pos) const override {
		if (pos > m_mb.size())
			Throw(E_FAIL);
		m_pos = (size_t)pos;
	}
};

class EXTAPI MemoryStream : public MemStreamWithPosition {
	typedef MemStreamWithPosition base;
public:
	typedef MemoryStream class_type;

	static const size_t DEFAULT_CAPACITY = 256;
private:
	typedef AutoBlob<DEFAULT_CAPACITY> CBlob;
	CBlob m_blob;
	size_t m_size = 0;
public:
	MemoryStream(size_t capacity = DEFAULT_CAPACITY);
	uint64_t get_Length() const override { return m_size; }
    size_t size() const { return m_size; }
    const uint8_t *data() const { return m_blob.data(); }
	size_t Read(void* buf, size_t count) const override;
	void WriteBuffer(const void* buf, size_t count) override;
	bool Eof() const override;
	void Reset(size_t capacity = DEFAULT_CAPACITY);

	Span AsSpan() const { return Span(m_blob.data(), m_size); }    //!!!O obsolete

	size_t get_Capacity() const { return m_blob.size(); }
	DEFPROP_GET(size_t, Capacity);
};

AFX_API String AFXAPI operator+(const String& s, const char* lpsz); // friend declaration in the "class String" is not enough

inline String AFXAPI operator+(const String& s, unsigned char ch) { return s + String((char)ch); }

inline std::ostream& operator<<(std::ostream& os, const CStringVector& ar) {
	for (size_t i = 0; i < ar.size(); i++)
		os << ar[i] << '\n';
	return os;
}

inline String AFXAPI operator+(const char* p, const String& s) { return String(p) + s; }

inline String AFXAPI operator+(const String::value_type* p, const String& s) { return String(p) + s; }

class CIosStream : public Stream {
	typedef CIosStream class_type;
protected:
	std::istream* m_pis = 0;
	std::ostream* m_pos = 0;
public:
	CIosStream(std::istream& ifs)
		: m_pis(&ifs)
		{}

	CIosStream(std::ostream& ofs)
		: m_pos(&ofs) {}

	size_t Read(void* buf, size_t count) const override;
	void ReadExactly(void* buf, size_t count) const override;
	void WriteBuffer(const void* buf, size_t count) override;
	uint64_t get_Position() const override;
	void put_Position(uint64_t pos) const override;

	bool Eof() const override { return m_pis->eof(); }

	void Flush() override {
		if (m_pos)
			m_pos->flush();
	}

protected:
	CIosStream() {}
};

class StringInputStream : public CIosStream {
	String m_s;
	std::istringstream m_is;
public:
	StringInputStream(RCString s)
		: m_s(s)
		, m_is(m_s.c_str())
	{
		m_pis = &m_is;
	}
};

template <typename EL, typename TR> inline std::basic_ostream<EL, TR>& operator<<(std::basic_ostream<EL, TR>& os, const CPrintable& ob) { return os << ob.ToString(); }

inline std::ostream& operator<<(std::ostream& os, const CPrintable& ob) {
#if UCFG_TRC
	ob.Print(os);
#endif
	return os;
}

class MacAddress : totally_ordered<MacAddress>, public CPrintable {
public:
	uint64_t m_n64;

	MacAddress(const MacAddress& mac)
		: m_n64(mac.m_n64) {}

	explicit MacAddress(int64_t n64 = 0)
		: m_n64(n64) {}

	explicit MacAddress(RCSpan mb) {
		if (mb.size() != 6)
			Throw(E_FAIL);
		m_n64 = 0;
		memcpy(&m_n64, mb.data(), 6);
	}

	explicit MacAddress(RCString s);

	Span AsSpan() const { return Span((uint8_t*)&m_n64, 6); }

	void CopyTo(void* p) const {
		memcpy(p, &m_n64, 6);
	}

	bool operator<(MacAddress mac) const { return m_n64 < mac.m_n64; }
	bool operator==(MacAddress mac) const { return m_n64 == mac.m_n64; }

	static MacAddress __stdcall Null() { return MacAddress(); }
	static MacAddress __stdcall Broadcast() { return MacAddress(0xFFFFFFFFFFFFLL); }

#if UCFG_TRC
	void Print(ostream& os) const override;
#endif
};

#if !UCFG_WCE
} // Ext::

namespace EXT_HASH_VALUE_NS {
inline size_t hash_value(const Ext::MacAddress& mac) { return std::hash<uint64_t>()(mac.m_n64); }
} // namespace EXT_HASH_VALUE_NS

EXT_DEF_HASH(Ext::MacAddress)

namespace Ext {
#endif

class UTF8Encoding;

class Encoding : public InterlockedObject {
protected:
#if UCFG_WDM
	static bool t_IgnoreIncorrectChars;
#else
	static EXT_THREAD_PTR(Encoding) t_IgnoreIncorrectChars;
#endif

	typedef std::unordered_map<String, ptr<Encoding>> CEncodingMap;
	static CEncodingMap s_encodingMap;

#if UCFG_USE_POSIX
	iconv_t m_iconvTo;
	iconv_t m_iconvFrom;
#endif
public:
	EXT_DATA static Encoding* s_Default;
	EXT_DATA static UTF8Encoding UTF8;

	String Name;
	int CodePage;

	Encoding(int codePage = 0);

	static Encoding& AFXAPI Default();

	virtual ~Encoding() {
#if UCFG_USE_POSIX
		if ((intptr_t)m_iconvTo != -1)
			CCheck(::iconv_close(m_iconvTo));
		if ((intptr_t)m_iconvFrom != -1)
			CCheck(::iconv_close(m_iconvFrom));
#endif
	}
	static bool AFXAPI SetThreadIgnoreIncorrectChars(bool v);
	static Encoding* AFXAPI GetEncoding(RCString name);
	static Encoding* AFXAPI GetEncoding(int codepage);
	virtual size_t GetCharCount(RCSpan mb) const;
	virtual size_t GetChars(RCSpan mb, String::value_type* chars, size_t charCount) const;
	String GetString(RCSpan s) const;
	EXT_API virtual std::vector<String::value_type> GetChars(RCSpan mb) const;
	virtual size_t GetByteCount(const String::value_type* chars, size_t charCount) const;
	virtual size_t GetByteCount(RCString s) const { return GetByteCount(s, s.length()); }
	virtual size_t GetBytes(const String::value_type* chars, size_t charCount, uint8_t* bytes, size_t byteCount) const;
	virtual Blob GetBytes(RCString s) const;

	class CIgnoreIncorrectChars {
		bool m_prev;
	public:
		CIgnoreIncorrectChars(bool v = true)
			: m_prev(SetThreadIgnoreIncorrectChars(v)) {}

		~CIgnoreIncorrectChars() { SetThreadIgnoreIncorrectChars(m_prev); }
	};
};

class UTF8Encoding : public Encoding {
	typedef Encoding base;

	typedef std::codecvt_utf8_utf16<wchar_t> Cvt;
	Cvt m_cvt;
public:
	UTF8Encoding()
		: base(CP_UTF8) {}

	Blob GetBytes(RCString s) const;
	size_t GetBytes(const String::value_type* chars, size_t charCount, uint8_t* bytes, size_t byteCount) const override;
	size_t GetCharCount(RCSpan mb) const override;
	EXT_API std::vector<String::value_type> GetChars(RCSpan mb) const override;
	size_t GetChars(RCSpan mb, String::value_type* chars, size_t charCount) const override;
protected:
	void Pass(RCSpan mb, UnaryFunction<String::value_type, bool>& visitor) const;
	void PassToBytes(const String::value_type* pch, size_t nCh, UnaryFunction<uint8_t, bool>& visitor) const;
};

class ASCIIEncoding : public Encoding {
public:
	Blob GetBytes(RCString s) const override;
	size_t GetBytes(const String::value_type* chars, size_t charCount, uint8_t* bytes, size_t byteCount) const override;
	size_t GetCharCount(RCSpan mb) const override;
	EXT_API std::vector<String::value_type> GetChars(RCSpan mb) const override;
	size_t GetChars(RCSpan mb, String::value_type* chars, size_t charCount) const override;
};

class CodePageEncoding : public Encoding {
public:
	CodePageEncoding(int codePage);
};

#ifndef _MSC_VER
__forceinline uint32_t _byteswap_ulong(uint32_t v) {
#	ifdef __FreeBSD__
	return __bswap32(v);
#	else
	return __bswap_32(v);
#	endif
}

__forceinline uint16_t _byteswap_ushort(uint16_t v) {
#	ifdef __FreeBSD__
	return __bswap16(v);
#	else
	return __bswap_16(v);
#	endif
}
#endif

inline size_t RotlSizeT(size_t v, int shift) {
#if defined(_MSC_VER) && !UCFG_WCE
#	ifdef _WIN64
	return _rotl64(v, shift);
#	else
	return _rotl((uint32_t)v, shift);
#	endif
#else
	return shift == 0 ? v : (v << shift) | (v >> (sizeof(v) * 8 - shift));
#endif
}

int __cdecl PopCount(uint32_t v);
int __cdecl PopCount(uint64_t v);

class BitOps {
public:
	static inline bool BitTest(const void* p, int idx) {
#if defined(_MSC_VER) && !UCFG_WCE
#	ifdef _WIN64
		return _bittest64((int64_t*)p, idx);
#	else
		return ::_bittest((long*)p, idx);
#	endif
#else
		return ((const uint8_t*)p)[idx >> 3] & (1 << (idx & 7));
#endif
	}

	static inline bool BitTestAndSet(void* p, int idx) {
#if defined(_MSC_VER) && !UCFG_WCE
#	ifdef _WIN64
		return _bittestandset64((int64_t*)p, idx);
#	else
		return ::_bittestandset((long*)p, idx);
#	endif
#else
		uint8_t* pb = (uint8_t*)p + (idx >> 3);
		uint8_t mask = uint8_t(1 << (idx & 7));
		uint8_t v = *pb;
		*pb = v | mask;
		return v & mask;
#endif
	}

#if !UCFG_WCE
	static inline int PopCount(uint32_t v) {
#	ifdef _MSC_VER
		int r = 0; // 	__popcnt() is AMD-specific
		for (int i = 0; i < 32; ++i)
			r += (v >> i) & 1;
		return r;
#	else
		return __builtin_popcount(v);
#	endif
	}

	static inline int PopCount(uint64_t v) {
#	ifdef _MSC_VER
		int r = 0; // 	__popcnt() is AMD-specific
		for (int i = 0; i < 64; ++i)
			r += (v >> i) & 1;
		return r;
#	else
		return __builtin_popcountll(v);
#	endif
	}

	static inline int Scan(uint32_t mask) {
#	ifdef _MSC_VER
		unsigned long index;
		return _BitScanForward(&index, mask) ? index + 1 : 0;
#	else
		return __builtin_ffs(mask);
#	endif
	}

	static inline int Scan(uint64_t mask) {
#	ifdef _MSC_VER
#		ifdef _M_X64
		unsigned long index;
		return _BitScanForward64(&index, mask) ? index + 1 : 0;
#		else
		int r;
		return !mask ? 0 : (r = Scan(uint32_t(mask))) ? r : 32 + Scan(uint32_t(mask >> 32));
#		endif
#	else
		return __builtin_ffsll(mask);
#	endif
	}
#endif // !UCFG_WCE

	static inline int ScanReverse(uint32_t mask) {
#ifdef _MSC_VER
		unsigned long index;
		return _BitScanReverse(&index, mask) ? index + 1 : 0;
#else
		return mask == 0 ? 0 : 32 - __builtin_clz(mask);
#endif
	}

	static inline int ScanReverse(uint64_t mask) {
#ifdef _MSC_VER
#	ifdef _M_X64
		unsigned long index;
		return _BitScanReverse64(&index, mask) ? index + 1 : 0;
#	else
		int r;
		return !mask ? 0 : (r = ScanReverse(uint32_t(mask >> 32))) ? r + 32 : ScanReverse(uint32_t(mask));
#	endif
#else
		return mask == 0 ? 0 : 64 - __builtin_clzll(mask);
#endif
	}
};

typedef void(_cdecl* PFNAtExit)();

class AtExitRegistration {
	PFNAtExit m_pfn;
public:
	AtExitRegistration(PFNAtExit pfn)
		: m_pfn(pfn) {
		RegisterAtExit(m_pfn);
	}

	~AtExitRegistration() { UnregisterAtExit(m_pfn); }
};

class StreamReader {
public:
	Stream& BaseStream;
	Ext::Encoding& Encoding;
private:
	int m_prevChar;
public:
	StreamReader(Stream& stm, Ext::Encoding& enc = Ext::Encoding::Default())
		: BaseStream(stm)
		, Encoding(enc)
		, m_prevChar(-1) {}

	String ReadToEnd();
	EXT_API std::pair<String, bool> ReadLineEx();

	String ReadLine() { return ReadLineEx().first; }
private:
	int ReadChar();
};

class StreamWriter {
public:
	Ext::Encoding& Encoding;
	String NewLine;
	Stream& BaseStream;

	StreamWriter(Stream& stm, Ext::Encoding& enc = Ext::Encoding::Default())
		: Encoding(enc)
		, NewLine("\r\n")
		, BaseStream(stm) {}

	void WriteLine(RCString line);
};

unsigned int MurmurHashAligned2(RCSpan cbuf, uint32_t seed);
uint32_t MurmurHash3_32(RCSpan cbuf, uint32_t seed);

} // namespace Ext

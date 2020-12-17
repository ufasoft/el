/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


typedef uintptr_t BASEWORD;
typedef intptr_t S_BASEWORD;

__BEGIN_DECLS

#if UCFG_BIGNUM=='A'

void __stdcall ImpAddBignums(const BASEWORD *a, const BASEWORD *b, BASEWORD *c, size_t siz);
void __stdcall ImpSubBignums(const BASEWORD *a, const BASEWORD *b, BASEWORD *c, size_t siz);
void __stdcall ImpNegBignum(const BASEWORD *a, size_t siz, BASEWORD *c);
void __stdcall ImpMulBignums(const BASEWORD *a, size_t m, const BASEWORD *b, size_t n, BASEWORD *c);
void __stdcall ImpDivBignums(BASEWORD *a, const BASEWORD *b, BASEWORD *c, size_t n);
void __stdcall ImpShld(const BASEWORD s[], BASEWORD *d, size_t siz, size_t amt); // siz>=1, s[siz], d[siz+1],  amt<32x64
void __stdcall ImpShrd(const BASEWORD s[], BASEWORD *d, size_t siz, size_t amt, BASEWORD prev); // siz>=1, s[siz], d[siz],  amt<32x64

void __stdcall ImpAddBignumsEx(BASEWORD *u, const BASEWORD *v, size_t siz);
BASEWORD __cdecl ImpMulAddBignums(BASEWORD *r, const BASEWORD *a, size_t n, BASEWORD w);

BASEWORD __stdcall ImpShortDiv(const BASEWORD s[], BASEWORD d[], size_t siz, BASEWORD q);



#endif // UCFG_BIGNUM

void __stdcall ImpAndBignums(const BASEWORD *a, const BASEWORD *b, BASEWORD *c, size_t siz);
void __stdcall ImpOrBignums(const BASEWORD *a, const BASEWORD *b, BASEWORD *c, size_t siz);
void __stdcall ImpXorBignums(const BASEWORD *a, const BASEWORD *b, BASEWORD *c, size_t siz);

#ifdef _M_IX86
	int __cdecl MontgomeryMul32_SSE(uint32_t *rp, const uint32_t *ap, const uint32_t *bp, const uint32_t *np, const uint32_t *n0, uint32_t num);
#endif

__END_DECLS

#ifdef __cplusplus

#include <random>

const size_t BASEWORD_BITS = sizeof(BASEWORD) * 8;


typedef void (__stdcall *PFNImpBinary)(const BASEWORD *a, const BASEWORD *b, BASEWORD *c, size_t siz);



#if UCFG_BIGNUM=='G' || UCFG_BIGNUM=='N'
	using std::swap;
#	if UCFG_BIGNUM=='G'
#		pragma warning(push)
#		pragma warning(disable: 4146)
#		include <gmpxx.h>
#		pragma warning(pop)
#	elif UCFG_BIGNUM=='N'
#		include <NTL/ZZ.h>
#	endif

namespace Ext {

#else


#	ifdef _WIN64
	#define BitScanReverseBaseword BitScanReverse64
	#define BitTestAndSetBaseword _bittestandset64

	#define BASEWORD_MIN LLONG_MIN

#	else
	#define BitScanReverseBaseword BitScanReverse
	#define BitTestAndSetBaseword _bittestandset

	#define BASEWORD_MIN INT_MIN
#	endif

extern "C" bool __stdcall ImpRcl(size_t siz, BASEWORD p[]); //!!! there is no bool type in the C language
extern "C" bool __stdcall ImpMulSubBignums(const BASEWORD *u, const BASEWORD *v, size_t n, BASEWORD q);


namespace Ext {

template <typename T, typename U> T int_cast(const U& x) { Throw(E_NOTIMPL); };
template <> inline uint32_t int_cast<uint32_t, uint64_t>(const uint64_t& x) { return (uint32_t)x; }


class BigInteger;


#if UCFG_PLATFORM_X64

	struct CUInt128 {
		uint64_t Low;
		uint64_t High;

		CUInt128(uint64_t low = 0, uint64_t high = 0)
			:	Low(low)
			,	High(high)
		{}

		CUInt128 operator<<(int off) const {
			uint64_t a[3], r[3];
			ImpShld(&Low, a, 2, off>>1);
			ImpShld(a, r, 2, (off>>1) | (off&1));
			return CUInt128(r[0], r[1]);
		}		

		CUInt128 operator|(const CUInt128 v) const {
			return CUInt128(Low|v.Low, High|v.High);
		}

		CUInt128 operator+(uint64_t x) const {
			CUInt128 x128(x), r;
			BASEWORD d[3];
			ImpAddBignums(&Low, &x128.Low, d, 2);
			return CUInt128(d[0], d[1]);			// d[2] can contain significant bits.
		}

		uint64_t operator/(uint64_t q) const {		//!!! only when q>High
			uint64_t u[2] = { Low, High };
			uint64_t r;
			ImpShortDiv(u, &r, 1, q);
			return r;
		}

		uint64_t operator%(uint64_t q) const {			//!!! only when q>High
			uint64_t u[2] = { Low, High };
			uint64_t r;
			return ImpShortDiv(u, &r, 1, q);
		}

		CUInt128 operator*(uint64_t x) const {		//!!! only if rusult id 128-bit
			uint64_t d[3];
			ImpMulBignums(&Low, 2, &x, 1, d);
			return CUInt128(d[0], d[1]);
		}

		bool operator>(const CUInt128& x) const {
			return High>x.High || High==x.High && Low>x.Low;
		}

		bool operator>=(const CUInt128& x) const {
			return High>x.High || High==x.High && Low>=x.Low;
		}
	};

	template <> inline uint64_t int_cast<uint64_t, CUInt128>(const CUInt128& u128) { return u128.Low; }


#	if defined(_M_IA64)
	typedef unsigned __int128 D_BASEWORD;
#	else
	typedef CUInt128 D_BASEWORD;
#	endif
#else
	typedef uint64_t D_BASEWORD;
#endif




inline int ImpBSR(BASEWORD w) {
	return BitScanReverseBaseword(w);
}

inline BigInteger operator-(const BigInteger& x, const BigInteger& y);

#endif

class BigInteger {
	typedef BigInteger class_type;
public:
	BigInteger();

/*!!!
	BigInteger(const BigInteger& v)
#if #if UCFG_BIGNUM=='N'
		:	m_zz(v.m_zz)
#endif

	:	m_blob((CBlobBufBase*)0)
	{
	m_p = 0;
	operator=(v);
	}*/

	explicit BigInteger(uint32_t n);
	BigInteger(int64_t n);

	explicit BigInteger(RCString s, int bas = 10);
	BigInteger(const uint8_t* p, size_t count);

	size_t ToBytes(uint8_t* p, size_t size) const;
	Blob ToBytes() const;
	bool operator!() const;


	String ToString(int bas = 10) const;

	BASEWORD *ExtendTo(BASEWORD *p, size_t size) const;

#if UCFG_BIGNUM=='G'
	BigInteger(const mpz_class& zz)
		:	m_zz(zz)
	{}
#endif


#if UCFG_BIGNUM!='A'
	BigInteger(int n);
#	if LONG_MAX==0x7fffffff
	BigInteger(long n);
#	endif
	BigInteger(const BigInteger& bi);
	~BigInteger();
	BigInteger& operator=(const BigInteger& bi);
#else
	BigInteger(const BigInteger& bi)
		: m_blob(bi.m_blob)
		, m_count(bi.m_count)
	{
		m_data[0] = bi.m_data[0];
	}

	BigInteger& operator=(const BigInteger& bi) {
		m_count = bi.m_count;
		m_blob = bi.m_blob;
		m_data[0] = bi.m_data[0];
		return *this;
	}

#	if INTPTR_MAX > 0x7fffffff
	BigInteger(int n);
#	else
	BigInteger(S_BASEWORD n);
#	endif

#	if UCFG_SEPARATE_LONG_TYPE && LONG_MAX==0x7fffffff
	explicit BigInteger(unsigned long n)
		:	m_blob(nullptr)
	{
		int64_t v = n;
		Init((uint8_t*)&v, sizeof(v));
	}
#	endif

	BigInteger(const BASEWORD *p, size_t count)
		:	m_blob(nullptr)
	{
		Init((uint8_t*)p, count * sizeof(BASEWORD));
	}

	BigInteger(EXT_RV_REF(BigInteger) rv)
		:	m_blob(nullptr)
		,	m_count(1)
	{
		m_data[0] = 0;
		swap(rv);
	}

	BigInteger& operator=(EXT_RV_REF(BigInteger) rv) {
		swap(rv);
		return *this;
	}

	const BASEWORD *get_Data() const { return m_count<=size(m_data) ? m_data : (const BASEWORD*)m_blob.constData(); }
	DEFPROP_GET(const BASEWORD *, Data);

#endif
	bool AsBaseWord(S_BASEWORD& n) const;

	bool AsInt64(int64_t& n) const;
	double AsDouble() const;

	operator explicit_cast<int64_t>() const {
		int64_t r;
		if (!AsInt64(r))
			Throw(E_FAIL);
		return r;
	}

	operator explicit_cast<int>() const {
		int64_t r;
		if (!AsInt64(r) || r<LONG_MIN || r>LONG_MAX)
			Throw(E_FAIL);
		return (long)r;
	}

	operator explicit_cast<unsigned int>() const {
		int64_t r;
		if (!AsInt64(r) || r<0 || r>UINT_MAX)
			Throw(E_FAIL);
		return (unsigned int)r;
	}

	operator explicit_cast<uint8_t>() const {
		int64_t r;
		if (!AsInt64(r) || r<0 || r>255)
			Throw(E_FAIL);
		return (uint8_t)r;
	}

	friend inline void swapHelper(BigInteger& x, BigInteger& y);

	void swap(BigInteger& x) {
		swapHelper(*this, x);
	}

	//!!!	int64_t ToInt64() const;
	//!!!BigInteger& operator=(const BigInteger& v);
	bool operator==(const BigInteger& v) const;

	size_t get_Length() const;
	DEFPROP_GET(size_t, Length);

	bool TestBit(size_t idx);

	BigInteger operator~() const;
	BigInteger operator-() const;


	friend int AFXAPI Sign(const BigInteger& v);
	friend EXT_API pair<BigInteger, BigInteger> AFXAPI div(const BigInteger& x, const BigInteger& y);
	uint32_t NMod(unsigned int d) const;
	friend BigInteger AFXAPI operator>>(const BigInteger& x, size_t v);
	friend BigInteger AFXAPI operator<<(const BigInteger& x, size_t v);
	friend BinaryWriter& AFXAPI operator<<(BinaryWriter& wr, const BigInteger& n);

	BigInteger& operator+=(const BigInteger& v) { return _self = Add(v); }
	BigInteger& operator-=(const BigInteger& v) { return _self = Sub(v); }
	BigInteger& operator*=(const BigInteger& v) { return _self = Mul(v); }
	BigInteger& operator/=(const BigInteger& v) { return _self = div(_self, v).first; }
	BigInteger& operator%=(const BigInteger& v) { return _self = div(_self, v).second; }
	BigInteger& operator&=(const BigInteger& v) { return _self = And(v); }
	BigInteger& operator|=(const BigInteger& v) { return _self = Or(v); }
	BigInteger& operator^=(const BigInteger& v) { return _self = Xor(v); }
	BigInteger& operator>>=(size_t v) { return _self = _self >> v; }
	BigInteger& operator<<=(size_t v) { return _self = _self << v; }

	BigInteger& operator++() { return _self += 1; }
	BigInteger operator++(int) { return exchange(_self, Add(1)); }
	BigInteger& operator--() { return _self -= 1; }
	BigInteger operator--(int) { return exchange(_self, Sub(1)); }
	
	template <class E>
	static inline BigInteger AFXAPI Random(const BigInteger& maxValue, E& rngeng) {
		size_t nbytes = maxValue.Length / 8 + 1;
		uint8_t* p = (uint8_t*)alloca(nbytes);
		uniform_int_distribution<int> dist(0, 255);
		for (size_t i = 0; i < nbytes; ++i)
			p[i] = (uint8_t)dist(rngeng);
#if UCFG_BIGNUM!='A'
		uint8_t* ar = (uint8_t*)alloca(nbytes);
		maxValue.ToBytes(ar, nbytes);
		uint8_t hiByte = ar[nbytes - 1];
#else
		uint8_t hiByte = ((uint8_t*)maxValue.get_Data())[nbytes - 1];
#endif
		p[nbytes - 1] = hiByte ? (uint8_t)uniform_int_distribution<int>(0, hiByte - 1)(rngeng) : 0;
		return BigInteger(p, nbytes);
	}

	int Compare(const BigInteger& x) const;
	BigInteger Add(const BigInteger& x) const;
	BigInteger Sub(const BigInteger& x) const;
	BigInteger Mul(const BigInteger& y) const;
	BigInteger And(const BigInteger& x) const;
	BigInteger Or(const BigInteger& x) const;
	BigInteger Xor(const BigInteger& x) const;

	size_t GetBaseWords() const
#if UCFG_BIGNUM!='A'
		;
#else
	{ return m_count; }
#endif

	size_t GetHashCode() const {
#if UCFG_BIGNUM!='A'
		size_t nbytes = size_t((Length+8)/8);
		uint8_t* p = (uint8_t*)alloca(nbytes);
		ToBytes(p, nbytes);
		return hash_value(ConstBuf(p, nbytes));
#else
		return hash_value(ConstBuf(Data, m_count));
#endif
	}

private:
#if UCFG_BIGNUM=='G'
	mpz_class m_zz;
#elif UCFG_BIGNUM=='N'
	NTL::ZZ m_zz;

	BigInteger(const NTL::ZZ& zz);

#else
	Blob m_blob; // GC requires to keep this field first (it can't be 0xFF)

	BASEWORD m_data[1];

public:
	size_t m_count; //!!!
private:
#endif
	void Init(const uint8_t* p, size_t count);
	BigInteger Div(const BigInteger& v);

	friend BigInteger BinaryBignums(PFNImpBinary pfn, const BigInteger& x, const BigInteger& y, int incsize);
//!!!R	friend inline size_t hash_value(const BigInteger& bi);
};

BigInteger BinaryBignums(PFNImpBinary pfn, const BigInteger& x, const BigInteger& y, int incsize = 0);


inline void swapHelper(BigInteger& x, BigInteger& y) {
#if UCFG_BIGNUM!='A'
	swap(x.m_zz, y.m_zz);
#else
	x.m_blob.swap(y.m_blob);
	std::swap(x.m_count, y.m_count);
	std::swap(*x.m_data, *y.m_data);
#endif
}

inline void swap(BigInteger& x, BigInteger& y) { swapHelper(x, y); }

inline BigInteger abs(const BigInteger& bi) {
	return Sign(bi) < 0 ? -bi : bi;
}

BigInteger AFXAPI sqrt(const BigInteger& bi);

/*!!!R
namespace EXT_HASH_VALUE_NS {

inline size_t hash_value(const BigInteger& bi) {
#if UCFG_BIGNUM!='A'
	DWORD nbytes = DWORD((bi.Length+8)/8);
	uint8_t *p = (uint8_t*)alloca(nbytes);
	bi.ToBytes(p, nbytes);
	return hash_value(ConstBuf(p, nbytes));
#else
	return hash_value(ConstBuf(bi.Data, bi.m_count));
#endif
}

}*/

EXT_DEF_HASH_NS(Ext, BigInteger) 


inline BigInteger operator+(const BigInteger& x, const BigInteger& y) { return x.Add(y); }
inline BigInteger operator-(const BigInteger& x, const BigInteger& y) { return x.Sub(y); }
inline BigInteger operator*(const BigInteger& x, const BigInteger& y) { return x.Mul(y); }
inline BigInteger operator/(const BigInteger& x, const BigInteger& y) { return div(x, y).first; }
inline BigInteger operator%(const BigInteger& x, const BigInteger& y) { return div(x, y).second; }
inline BigInteger operator&(const BigInteger& x, const BigInteger& y) { return x.And(y); }
inline BigInteger operator|(const BigInteger& x, const BigInteger& y) { return x.Or(y); }
inline BigInteger operator^(const BigInteger& x, const BigInteger& y) { return x.Xor(y); }


inline bool operator!=(const BigInteger& x, const BigInteger& y) {
	return !(x == y);
}

inline bool operator<(const BigInteger& x, const BigInteger& y) { return x.Compare(y) < 0; }
inline bool operator>(const BigInteger& x, const BigInteger& y) { return y < x; }
inline bool operator>=(const BigInteger& x, const BigInteger& y) { return !(x < y); }
inline bool operator<=(const BigInteger& x, const BigInteger& y) { return !(x > y); }

EXT_API std::ostream& AFXAPI operator<<(std::ostream& os, const BigInteger& n);
EXT_API std::istream& AFXAPI operator>>(std::istream& is, BigInteger& n);

EXT_API const BinaryReader& AFXAPI operator>>(const BinaryReader& rd, BigInteger& n);

BigInteger pow(const BigInteger& x, int n);

} // Ext::


#endif // __cpluplus




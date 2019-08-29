/*######   Copyright (c) 2014-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include EXT_HEADER_DYNAMIC_BITSET

#if UCFG_LIB_DECLS
#	ifdef UCFG_HASH_LIB
#		define EXT_HASH_API DECLSPEC_DLLEXPORT
#	else
#		define EXT_HASH_API DECLSPEC_DLLIMPORT
#	endif
#else
#	define EXT_HASH_API
#endif

#include "util.h"

namespace Ext { namespace Crypto {
using namespace std;

extern "C" {

	extern const uint8_t g_blake_sigma[10][16];
	extern const uint32_t g_blake256_c[16];
	extern const uint64_t g_blake512_c[16];

} // "C"

struct Sha256Constants {
	const uint32_t* Sha256_hinit;
	const uint32_t* Sha256_k;
	const uint32_t(*FourSha256_k)[4];
};

const Sha256Constants& GetSha256Constants();

__forceinline uint32_t Rotr32(uint32_t v, int n) {
	return _rotr(v, n);
}

__forceinline uint64_t Rotr64(uint64_t v, int n) {
	return _rotr64(v, n);
}


class SHA1 : public HashAlgorithm {
public:
	SHA1() {
		BlockSize = 64;
		HashSize = 20;
	}

protected:
	void InitHash(void *dst) noexcept override;
	void HashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept override;
};

class SHA256 : public HashAlgorithm {
	typedef HashAlgorithm base;
public:
	static void Init4Way(uint32_t state[8][4]);

	SHA256() {
		BlockSize = 64;
		HashSize = 32;
	}

	void InitHash(void *dst) noexcept override;
	void HashBlock(void* dst, uint8_t *src, uint64_t counter) noexcept override;
	void PrepareEndiannessAndHashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept override;
	hashval ComputeHash(RCSpan s) override;
};

class SHA512 : public HashAlgorithm {
public:
	SHA512() {
		BlockSize = 128;
		HashSize = 64;
		Is64Bit = true;
	}
protected:
	void InitHash(void *dst) noexcept override;
	void HashBlock(void *dst, uint8_t src[256], uint64_t counter) noexcept override;
};

class SipHash2_4 : public HashAlgorithm {
    typedef HashAlgorithm base;

	uint64_t m_key[2];
public:
	SipHash2_4(uint64_t key0, uint64_t key1)
	{
		m_key[0] = key0;
		m_key[1] = key1;
		BlockSize = 128;
		HashSize = 8;
		Is64Bit = true;
	}

    hashval ComputeHash(Stream& stm) override;
    hashval ComputeHash(RCSpan s) override { return base::ComputeHash(s); }
protected:
	void InitHash(void *dst) noexcept override;
private:
    static void Round(uint64_t v[4]);
    static void TwoRounds(uint64_t v[4]) { Round(v); Round(v); }
    static void FourRounds(uint64_t v[4]) { TwoRounds(v); TwoRounds(v); }
};

#ifndef UCFG_IMP_SHA3
#	define UCFG_IMP_SHA3 'S'		// shplib
#endif

#ifndef UCFG_IMP_GROESTL
#	define UCFG_IMP_GROESTL 'T'		// 'S' for shplib,  'T' for T-table
#endif


template <int n>
class SHA3 : public HashAlgorithm {
};

template<> class SHA3<256> : HashAlgorithm {
public:
	SHA3<256>() {
		BlockSize = 64;
		HashSize = 32;
	}

	hashval ComputeHash(RCSpan mb) override;
	hashval ComputeHash(Stream& stm) override;
};

template<> class SHA3<512> : HashAlgorithm {
	SHA3<512>() {
		BlockSize = 128;
		HashSize = 64;
	}

	hashval ComputeHash(RCSpan mb) override;
	hashval ComputeHash(Stream& stm) override;
};

class Blake256 : public HashAlgorithm {
public:
	uint32_t Salt[4];

	Blake256() {
		BlockSize = 128;	//!!!?
		HashSize = 64;	//!!!?
		IsHaifa = true;
		ZeroStruct(Salt);
	}
protected:
	void InitHash(void *dst) noexcept override;
	void HashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept override;
};

class GroestlHash : public HashAlgorithm {
protected:
	GroestlHash(size_t hashSize);
	void InitHash(void *dst) noexcept override;
};

class Groestl512Hash : public GroestlHash {
	typedef GroestlHash base;
public:
	Groestl512Hash()
		: base(64)
	{}

	hashval ComputeHash(RCSpan mb) override;

	hashval ComputeHash(Stream& stm) override {
#if UCFG_IMP_GROESTL=='S'
		Throw(E_NOTIMPL);
#else
		return base::ComputeHash(stm);
#endif
	}

protected:
	void InitHash(void *dst) noexcept override;
	void HashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept override;
	void OutTransform(void *dst) noexcept override;
};


class Blake512 : public HashAlgorithm {
public:
	uint64_t Salt[4];

	Blake512() {
		BlockSize = 128;	//!!!?
		HashSize = 64;	//!!!?
		IsHaifa = true;
		Is64Bit = true;
		ZeroStruct(Salt);
	}
protected:
	void InitHash(void *dst) noexcept override;
	void HashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept override;
	void OutTransform(void *dst) noexcept override;
};

class RIPEMD160 : public HashAlgorithm {
public:
	RIPEMD160() {
		BlockSize = 64;
		HashSize = 20;
		IsLenBigEndian = IsBigEndian = false;
	}

#if UCFG_USE_OPENSSL
	hashval ComputeHash(Stream& stm) override;
	hashval ComputeHash(RCSpan mb) override;
#endif
protected:
	void InitHash(void *dst) noexcept override;
	void HashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept override;
};


class Random : public Ext::Random {
public:
	Random();
	void NextBytes(const span<uint8_t>& mb) override;
};

class PseudoRandomFunction {
public:
	virtual size_t HashSize() const =0;
	virtual hashval operator()(RCSpan key, RCSpan text) =0;
};

template <class H>
class HmacPseudoRandomFunction : public PseudoRandomFunction {
public:
	H HAlgo;

	size_t HashSize() const override { return HAlgo.HashSize; }
	hashval operator()(RCSpan key, RCSpan text) override { return HMAC(HAlgo, key, text); }
};

Blob PBKDF2(PseudoRandomFunction& prf, RCSpan password, RCSpan salt, uint32_t c, size_t dkLen);


typedef std::array<uint32_t, 8> CArray8UInt32;
hashval CalcPbkdf2Hash(const uint32_t *pIhash, RCSpan data, int idx);
void CalcPbkdf2Hash_80_4way(uint32_t dst[32], const uint32_t *pIhash, RCSpan data);

void ShuffleForSalsa16(uint32_t w[16], const uint32_t src[16]);
void ShuffleForSalsa(uint32_t w[32], const uint32_t src[32]);
void UnShuffleForSalsa16(uint32_t dst[16], const uint32_t w[16]);
void UnShuffleForSalsa(uint32_t w[32], const uint32_t src[32]);

typedef uint32_t(*SalsaBlockPtr)[16];
void NeoScryptCore(uint32_t x[][16], uint32_t tmp[][16], uint32_t v[], int r, int rounds, int n, bool dblmix) noexcept;
void ScryptCore(uint32_t x[32], uint32_t alignedScratch[1024*32+32]) noexcept;
CArray8UInt32 CalcSCryptHash(RCSpan password);
std::array<CArray8UInt32, 3> CalcSCryptHash_80_3way(const uint32_t input[20]);
CArray8UInt32 CalcNeoSCryptHash(RCSpan password, int profile = 0);

Blob Scrypt(RCSpan password, RCSpan salt, int n, int r, int p, size_t dkLen);

#if UCFG_CPU_X86_X64
extern "C" void _cdecl ScryptCore_x86x64(uint32_t x[32], uint32_t alignedScratch[1024*32+32]);
#endif

template <class T, class H2>
class MerkleBranch {
public:
	std::vector<T> Vec;
	H2 m_h2;
	int Index;

	MerkleBranch()
		: Index(-1)
	{}

	T Apply(T hash) const {
		if (Index < 0)
			Throw(E_FAIL);
		for (int i = 0, idx = Index; i < Vec.size(); ++i, idx >>= 1)
			hash = idx & 1 ? m_h2(Vec[i], hash) : m_h2(hash, Vec[i]);
		return hash;
	}
};

template <class T, class H2>
class MerkleTree : public vector<T> {
	typedef vector<T> base;
public:
	H2 m_h2;
	int SourceSize;

	MerkleTree()
		: SourceSize(-1)
	{}

	template <class U, class H1>
	MerkleTree(const vector<U>& ar, H1 h1, H2 h2)
		: base(ar.size())
		, SourceSize(int(ar.size()))
		, m_h2(h2)
	{
		for (int i = 0; i < ar.size(); ++i)
			(*this)[i] = h1(ar[i], i);
		for (int j = 0, nSize = ar.size(); nSize > 1; j += exchange(nSize, (nSize + 1) / 2))
			for (int i = 0; i < nSize; i += 2)
				base::push_back(h2((*this)[j + i], (*this)[j + std::min(i + 1, nSize - 1)]));
	}

	MerkleBranch<T, H2> GetBranch(int idx) {
		MerkleBranch<T, H2> r;
		r.m_h2 = m_h2;
		r.Index = idx;
		for (int j = 0, n = SourceSize; n > 1; j += n, n = (n + 1) / 2, idx >>= 1)
			r.Vec.push_back((*this)[j + std::min(idx^1, n-1)]);
		return r;
	}
};

template <class T, class U, class H1, class H2>
MerkleTree<T, H2> BuildMerkleTree(const vector<U>& ar, H1 h1, H2 h2) {
	return MerkleTree<T, H2>(ar, h1, h2);
}

class PartialMerkleTreeBase {
public:
	dynamic_bitset<uint8_t> Bitset;
	size_t NItems;

    size_t CalcTreeWidth(int height) const {
        return (NItems + (size_t(1) << height) - 1) >> height;
    }

	int CalcTreeHeight() const;
	virtual void AddHash(int height, size_t pos, const void *ar) =0;
protected:
	void TraverseAndBuild(int height, size_t pos, const void* ar, const dynamic_bitset<uint8_t>& vMatch);
};

template <class T, class H2>
class PartialMerkleTree : public PartialMerkleTreeBase {
	typedef PartialMerkleTreeBase base;
public:
	H2 m_h2;
	vector<T> Items;

	PartialMerkleTree(H2 h2)
		: m_h2(h2)
	{}

	T CalcHash(int height, size_t pos, const T *ar) const {
		if (0 == height)
			return ar[pos];
		T left = CalcHash(height-1, pos*2, ar),
			right = pos*2+1 < CalcTreeWidth(height-1) ? CalcHash(height-1, pos*2+1, ar) : left;
		return m_h2(left, right);
	}

	void AddHash(int height, size_t pos, const void *ar) override {
		Items.push_back(CalcHash(height, pos, (const T*)ar));
	}

	T TraverseAndExtract(int height, size_t pos, size_t& nBitsUsed, int& nHashUsed, vector<T>& vMatch) const {
		if (nBitsUsed >= Bitset.size())
			Throw(E_FAIL);
		bool fParentOfMatch = Bitset[nBitsUsed++];
		if (height==0 || !fParentOfMatch) {
	        if (nHashUsed >= Items.size())
				Throw(E_FAIL);
	        const T &hash = Items[nHashUsed++];
	        if (height==0 && fParentOfMatch) // in case of height 0, we have a matched txid
    	        vMatch.push_back(hash);
 	       return hash;
    	} else {
			T left = TraverseAndExtract(height-1, pos*2, nBitsUsed, nHashUsed, vMatch),
			   right = pos*2+1 < CalcTreeWidth(height-1) ? TraverseAndExtract(height-1, pos*2+1, nBitsUsed, nHashUsed, vMatch) : left;
			return m_h2(left, right);
		}
	}
};



/*!!!R
template <class T, class U, class H1, class H2>
std::vector<T> BuildMerkleTree(const vector<U>& ar, H1 h1, H2 h2) {
	std::vector<T> r(ar.size());
	std::transform(ar.begin(), ar.end(), r.begin(), h1);
    for (int j=0, nSize = ar.size(); nSize > 1; j += exchange(nSize, (nSize + 1) / 2))
        for (int i = 0; i < nSize; i += 2)
			r.push_back(h2(r[j+i], r[j+std::min(i+1, nSize-1)]));
	return r;
}
*/


}} // Ext::Crypto::

extern "C" {


#if UCFG_CPU_X86_X64
	void _cdecl Sha256Update_4way_x86x64Sse2(uint32_t state[8][4], const uint32_t data[16][4]);
	void _cdecl Sha256Update_x86x64(uint32_t state[8], const uint32_t data[16]);
	void _cdecl Blake512Round(int sigma0, int sigma1, uint64_t& pa, uint64_t& pb, uint64_t& pc, uint64_t& pd, const uint64_t m[16], const uint64_t blakeC[]);
#endif

} // "C"

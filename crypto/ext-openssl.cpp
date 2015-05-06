/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#define OPENSSL_NO_SCTP
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "ext-openssl.h"
#include "hash.h"

#pragma warning(disable: 4073)
#pragma init_seg(user)

#pragma comment(lib, "openssl")


namespace Ext { namespace Crypto {

static class OpensslCategory : public ErrorCategoryBase {
	typedef ErrorCategoryBase base;
public:
	OpensslCategory()
		:	base("OpenSSL", FACILITY_OPENSSL)
	{}

	string message(int errval) const override {
		char buf[256];
		ERR_error_string_n(errval, buf, sizeof buf);
		return buf;
	}
} s_openssl_category;

const error_category& openssl_category() {
	return s_openssl_category;
}

void SslCheck(bool b) {
	if (!b) {
#ifdef OPENSSL_NO_ERR
		throw OpenSslException(1, "OpenSSL Error"); //!!! code?
#else
		ERR_load_crypto_strings();
		int rc = ::ERR_get_error();
		throw OpenSslException(ERR_GET_REASON(rc));
#endif
	}
}

static void *OpenSslMallocFun(size_t size) {
	return Malloc(size);
}

static void OpenSslFree(void *p) {
	free(p);
}

static void *OpenSslRealloc(void *p, size_t size) {
	return Realloc(p, size);
}

OpenSslMalloc::OpenSslMalloc() {
	SslCheck(::CRYPTO_set_mem_functions(&OpenSslMallocFun, &OpenSslRealloc, &OpenSslFree));
}

static mutex *s_pOpenSslMutexes;


static void OpenSslLockingCallback(int mode, int i, const char* file, int line) {
	mutex& m = s_pOpenSslMutexes[i];
    if (mode & CRYPTO_LOCK)
		m.lock();
	else
		m.unlock();
}

static void OpenSslThreadId(CRYPTO_THREADID *ti) {
#if UCFG_WIN32
	CRYPTO_THREADID_set_numeric(ti, ::GetCurrentThreadId());
#else
	CRYPTO_THREADID_set_numeric(ti, ::pthread_self());
#endif
}

struct CInitOpenSsl {
	CInitOpenSsl() {
		s_pOpenSslMutexes = new mutex[CRYPTO_num_locks()];
        CRYPTO_set_locking_callback(&OpenSslLockingCallback);
		CRYPTO_THREADID_set_callback(&OpenSslThreadId);
	}

	~CInitOpenSsl() {
        CRYPTO_set_locking_callback(nullptr);
		CRYPTO_THREADID_set_callback(nullptr);
		delete[] exchange(s_pOpenSslMutexes, nullptr);
	}
} g_initOpenSsl;



static OpenSslMalloc s_openSslMalloc;		// should be first global OpenSsl object

static struct RANDCleanup {
	bool Inited;

	~RANDCleanup() {
		if (Inited)
			::RAND_cleanup();
	}
} s_randCleanup;

Random::Random() {
	s_randCleanup.Inited = true;

	int64_t ticks = DateTime::UtcNow().Ticks;
	::RAND_add(&ticks, sizeof ticks, 2);

#if UCFG_CPU_X86_X64
	int64_t tsc = __rdtsc();
	::RAND_add(&tsc, sizeof tsc, 4);
#endif

#ifdef _WIN32
	int64_t cnt = System.PerformanceCounter;
	::RAND_add(&cnt, sizeof cnt, 4);
#endif

#ifdef WIN32
	typedef BOOLEAN (APIENTRY *PFN_PRNG)(void*, ULONG);	
	DlProcWrap<PFN_PRNG> pfnPrng("advapi32.dll", "SystemFunction036");		//!!! Undocumented
	if (pfnPrng) {
		byte buf[32];
		if (pfnPrng(buf, sizeof buf))
			::RAND_add(&buf, sizeof buf, sizeof buf/2);
	}
#endif
}

void Random::NextBytes(const Buf& mb) {
	SslCheck(::RAND_bytes(mb.P, mb.Size) > 0);
}


static Ext::Crypto::Random g_Random;

Random& RandomRef() {
	return g_Random;
}


OpensslBn::OpensslBn(const BigInteger& bn) {
	int n = (bn.Length+8)/8;
	byte *p = (byte*)alloca(n);
	bn.ToBytes(p, n);
	std::reverse(p, p+n);
	SslCheck(m_bn = ::BN_bin2bn(p, n, 0));
}

BigInteger OpensslBn::ToBigInteger() const {
	int n = BN_num_bytes(m_bn);
	byte *p = (byte*)alloca(n+1);
	if (::BN_bn2bin(m_bn, p) != n)
		Throw(E_FAIL);
	std::reverse(p, p+n);
	p[n] = 0;
	return BigInteger(p, n+1);
}

OpensslBn OpensslBn::FromBinary(const ConstBuf& cbuf) {
	BIGNUM *a = ::BN_bin2bn(cbuf.P, cbuf.Size, 0);
	SslCheck(a);
	return OpensslBn(a);
}

void OpensslBn::ToBinary(byte *p, size_t n) const {
	int a = BN_num_bytes(m_bn);
	if (a > n)
		Throw(E_INVALIDARG);
	memset(p, 0, n-a);
	SslCheck(::BN_bn2bin(m_bn, p+(n-a)));
}

BigInteger sqrt_mod(const BigInteger& x, const BigInteger& mod) {
	OpensslBn v(x),
		m(mod),
		r;
	BnCtx ctx;
	SslCheck(::BN_mod_sqrt(r.Ref(), v, m, ctx));
	return r.ToBigInteger();
}



}} // Ext::Crypto

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

static void *OpenSslMallocFun(size_t size, const char *file, int line) {
	return Malloc(size);
}

static void OpenSslFree(void *p, const char *file, int line) {
	free(p);
}

static void *OpenSslRealloc(void *p, size_t size, const char *file, int line) {
	return Realloc(p, size);
}

OpenSslMalloc::OpenSslMalloc() {
	SslCheck(::CRYPTO_set_mem_functions(&OpenSslMallocFun, &OpenSslRealloc, &OpenSslFree));
}

/*!!!R
//!!!Rstatic mutex *s_pOpenSslMutexes;

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
*/

static OpenSslMalloc s_openSslMalloc;		// should be first global OpenSsl object

Random::Random() {
	int64_t ticks = Clock::now().Ticks;
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
		uint8_t buf[32];
		if (pfnPrng(buf, sizeof buf))
			::RAND_add(&buf, sizeof buf, sizeof buf/2);
	}
#endif
}

void Random::NextBytes(const span<uint8_t>& mb) {
	SslCheck(::RAND_bytes(mb.data(), mb.size()) > 0);
}


static InterlockedSingleton<Ext::Crypto::Random> s_pRandom;

Random& RandomRef() {
	return *s_pRandom;
}


OpensslBn::OpensslBn(const BigInteger& bn) {
	int n = (bn.Length+8)/8;
	uint8_t *p = (uint8_t*)alloca(n);
	bn.ToBytes(p, n);
	std::reverse(p, p+n);
	SslCheck(m_bn = ::BN_bin2bn(p, n, 0));
}

BigInteger OpensslBn::ToBigInteger() const {
	int n = BN_num_bytes(m_bn);
	uint8_t *p = (uint8_t*)alloca(n+1);
	if (::BN_bn2bin(m_bn, p) != n)
		Throw(E_FAIL);
	std::reverse(p, p+n);
	p[n] = 0;
	return BigInteger(p, n+1);
}

OpensslBn OpensslBn::FromBinary(RCSpan cbuf) {
	BIGNUM *a = ::BN_bin2bn(cbuf.data(), cbuf.size(), 0);
	SslCheck(a);
	return OpensslBn(a);
}

void OpensslBn::ToBinary(uint8_t *p, size_t n) const {
	int a = BN_num_bytes(m_bn);
	if (a > n)
		Throw(errc::invalid_argument);
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

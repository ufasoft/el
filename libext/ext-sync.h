/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#if UCFG_WIN32
#	ifdef NTDDI_WIN8
#		include <synchapi.h >
#	else
#		include <winbase.h>
#	endif

#	if !UCFG_WCE
#		include <winternl.h>
#	endif
#endif


#if UCFG_USE_PTHREADS
#	include <semaphore.h>
#endif

#if !UCFG_STDSTL || !UCFG_STD_MUTEX
#	include <el/stl/mutex>

namespace std {
	using ExtSTL::mutex;
	using ExtSTL::recursive_mutex;
	using ExtSTL::lock_guard;
	using ExtSTL::once_flag;
	using ExtSTL::call_once;
}

#endif

namespace Ext {
	using std::try_to_lock_t;

class atomic_flag_lock : noncopyable {
	atomic_flag& m_aFlag;
	bool m_owns;
public:
	atomic_flag_lock(atomic_flag& aFlag, try_to_lock_t) noexcept
		: m_aFlag(aFlag)
		, m_owns(!m_aFlag.test_and_set())
	{
	}

	~atomic_flag_lock() {
		if (m_owns)
			m_aFlag.clear();
	}

	EXPLICIT_OPERATOR_BOOL() const { return m_owns ? EXT_CONVERTIBLE_TO_TRUE : 0; }
};

#if UCFG_USE_PTHREADS
int PthreadCheck(int code, int allowableError = 0);
#endif

#ifdef _WIN32
	NTSTATUS AFXAPI NtCheck(NTSTATUS status, NTSTATUS allowableError = 0);		// 0 == STATUS_SUCCESS
#endif

class AFX_CLASS CSyncObject : public SafeHandle {
	typedef CSyncObject class_type;
public:
	CBool m_bAlreadyExists;

	CSyncObject() {}
	CSyncObject(RCString pstrName);
	void AttachCreated(intptr_t h);
	virtual void unlock() =0;

	virtual bool lock(uint32_t dwTimeout = INFINITE);
	void Unlock(int32_t lCount, int32_t *lprevCount = 0);
#ifdef WDM_DRIVER
	bool LockEx(uint32_t dwTimeout, KPROCESSOR_MODE mode, bool bAlertable);
#endif
private:
	CSyncObject(const class_type&);
};

class AFX_CLASS CCriticalSection : public CSyncObject {
public:
#if UCFG_USE_POSIX
	typedef pthread_mutex_t *native_handle_type;
#elif UCFG_WIN32
	typedef CRITICAL_SECTION *native_handle_type;
#elif UCFG_WDM
	typedef KSPIN_LOCK *native_handle_type;
#endif

	CCriticalSection();
	//  CCriticalSection(DWORD dwSpinCount);
	~CCriticalSection() noexcept;

	void lock() {
#if UCFG_USE_PTHREADS
		PthreadCheck(::pthread_mutex_lock(&m_mutex));
#elif defined(WDM_DRIVER)
		KeAcquireSpinLock(&m_spinLock, &m_oldIrql);
#else
		::EnterCriticalSection(&m_sect);				// don't handle EXCEPTION_POSSIBLE_DEADLOCK and "OutOfMemory on Win2K"
#endif
	}

	bool try_lock() {
#if UCFG_USE_PTHREADS
		return !PthreadCheck(::pthread_mutex_trylock(&m_mutex), EBUSY);
#elif UCFG_WDM
		m_oldIrql = DISPATCH_LEVEL;					//!!!
		return KeTryToAcquireSpinLockAtDpcLevel(&m_spinLock);
#else
		return ::TryEnterCriticalSection(&m_sect);				// don't handle EXCEPTION_POSSIBLE_DEADLOCK and "OutOfMemory on Win2K"
#endif
	}

	void unlock() noexcept override {
#if UCFG_USE_PTHREADS
		PthreadCheck(::pthread_mutex_unlock(&m_mutex));
#elif defined(WDM_DRIVER)
		KeReleaseSpinLock(&m_spinLock, m_oldIrql);
#else
		::LeaveCriticalSection(&m_sect);
#endif
	}

	bool lock(uint32_t dwTimeout) override;
	bool TryLock();

	//!!!  DWORD SetSpinCount(DWORD dwSpinCount);

#if UCFG_USE_POSIX
	native_handle_type native_handle() { return &m_mutex; }
#elif UCFG_WIN32
	native_handle_type native_handle() { return &m_sect; }
#elif UCFG_WDM
	native_handle_type native_handle() { return &m_spinLock; }
#endif

protected:

#if UCFG_USE_PTHREADS
	pthread_mutex_t m_mutex;
#elif defined WDM_DRIVER
    KSPIN_LOCK m_spinLock;
	KIRQL m_oldIrql;
#else
	CRITICAL_SECTION m_sect;
#endif
	void Init();

friend class CEvent;
};

#if UCFG_WIN32
class AFX_CLASS CNonRecursiveCriticalSection : public CCriticalSection {
	typedef CCriticalSection base;
public:

	bool lock(uint32_t dwTimeout) override { return base::lock(dwTimeout); }

	void lock() {
		::EnterCriticalSection(&m_sect);				// don't handle EXCEPTION_POSSIBLE_DEADLOCK and "OutOfMemory on Win2K"
#if UCFG_DEBUG
		ASSERT(m_sect.RecursionCount == 1);
#endif
	}

	bool try_lock() {
		if (m_sect.RecursionCount > 0)
			return false;
		return ::TryEnterCriticalSection(&m_sect);				// don't handle EXCEPTION_POSSIBLE_DEADLOCK and "OutOfMemory on Win2K"
	}
};
#endif // UCFG_WIN32

class CScopedLockBase {
public:
	operator bool() const {
		return false;
	}
};

template <class T>
class CScopedLock : public CScopedLockBase {
public:
	mutable T *m_sync;

	CScopedLock(const CScopedLock& sl)
		:	m_sync(exchange(sl.m_sync, nullptr))
	{}

	CScopedLock(T& sync)
		:	m_sync(&sync)
	{
		m_sync->lock();
	}

	~CScopedLock() {
		if (m_sync)
			m_sync->unlock();
	}


/*!!!	void Unlock() const {
		Ext::Unlock(*m_sync);
		m_sync = 0;
	}*/
};


template <class T> CScopedLock<T> ScopedLock(T& sync) { return CScopedLock<T>(sync); }

#define EXT_LOCK(m) if (const Ext::CScopedLockBase& lck = Ext::ScopedLock(m)) Ext::ThrowImp(Ext::ExtErr::CodeNotReachable); else
//#define EXT_LOCK(m) if (const Ext::CScopedLockBase& lck = Ext::ScopedLock(m)) ; else

#if UCFG_CPP11
#	define EXT_LOCKED(mtx, expr) (std::lock_guard<decltype(mtx)>(mtx), (expr))			//!!!
#else
#	define EXT_LOCKED(mtx, expr) (Ext::ScopedLock(mtx), (expr))			//!!!
#endif

#if UCFG_SIMPLE_MACROS
#	define lock EXT_LOCK
#endif

} // Ext::






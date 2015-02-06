#include <el/ext.h>


#pragma warning(disable: 4073)
#pragma init_seg(lib)				// for AutoEventPool

#include <locale.h>

#if UCFG_WIN32
#	include <windows.h>
#endif

#ifdef __FreeBSD__
#	include <sys/sysctl.h>
#endif

namespace Ext {
using namespace std;

COperatingSystem System;

#if UCFG_WIN32 || UCFG_USE_POSIX

//!!! use locale class instead of global change, because it changes fstream locale  static char *s_initLocale = ::setlocale(LC_CTYPE, "");   // only LC_CTYPE because List requires standard float

path COperatingSystem::get_ExeFilePath() {
#ifdef __FreeBSD__
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	char szModule[1024];
	size_t cb = sizeof(szModule);
	sysctl(mib, 4, szModule, &cb, NULL, 0);
#elif UCFG_USE_POSIX
	TCHAR szModule[PATH_MAX];
	memset(szModule, 0, sizeof szModule);
	char szTmp[32];
	sprintf(szTmp, "/proc/%d/exe", getpid());
	CCheck(::readlink(szTmp, szModule, sizeof(szModule)));
#else	
	TCHAR szModule[_MAX_PATH];
	Win32Check(GetModuleFileName(0, szModule, _MAX_PATH));
#endif
	return szModule;
}

path COperatingSystem::GetExeDir() {
	return get_ExeFilePath().parent_path();
}

#endif // UCFG_WIN32 || UCFG_USE_POSIX

#ifdef _WIN32

int64_t COperatingSystem::get_PerformanceCounter() {
	LARGE_INTEGER li;
#if UCFG_WDM
	li = ::KeQueryPerformanceCounter(nullptr);
#else
	Win32Check(::QueryPerformanceCounter(&li));
#endif
	return li.QuadPart;
}

int64_t COperatingSystem::get_PerformanceFrequency() {
	LARGE_INTEGER li;
#if UCFG_WDM
	::KeQueryPerformanceCounter(&li);
#else
	Win32Check(::QueryPerformanceFrequency(&li));
#endif
	return li.QuadPart;
}

#endif


CSyncObject::CSyncObject(RCString pstrName)
	:	m_bAlreadyExists(false)
{
}

#ifdef WDM_DRIVER
bool CSyncObject::LockEx(uint32_t dwTimeout, KPROCESSOR_MODE mode, bool bAlertable) {
	LARGE_INTEGER timeout, *pto = dwTimeout==INFINITE ? NULL : &timeout;
	timeout.QuadPart = -(int)dwTimeout*10000;
	return STATUS_SUCCESS == KeWaitForSingleObject(m_pObject, Executive, mode, bAlertable, pto);
}
#endif

bool CSyncObject::lock(uint32_t dwTimeout) {
#if UCFG_USE_POSIX
	Throw(E_NOTIMPL);
#endif
#ifdef WIN32
	DWORD r = ::WaitForSingleObject(HandleAccess(_self), dwTimeout);
	switch (r) {
		case WAIT_OBJECT_0: return true;
		case WAIT_TIMEOUT: break;
		case WAIT_ABANDONED: Throw(HRESULT_FROM_WIN32(ERROR_ABANDONED_WAIT_0));
		case WAIT_FAILED:
			Win32Check(false);
			break;
		default:
			Throw(E_FAIL);
	}
	if (INFINITE == dwTimeout)
		Throw(E_FAIL);
	return false;
#elif defined(WDM_DRIVER)
	return LockEx(dwTimeout, KernelMode, TRUE);
#endif
}

void CSyncObject::Unlock(int32_t lCount, int32_t *lprevCount) {
}


CCriticalSection::CCriticalSection()
	:	CSyncObject(nullptr)
{
	Init();
}

/*CCriticalSection::CCriticalSection(DWORD dwSpinCount)
:CSyncObject(0)
{
Win32Check(InitializeCriticalSectionAndSpinCount(&m_sect, dwSpinCount));
}*/

CCriticalSection::~CCriticalSection() noexcept {
#if UCFG_USE_PTHREADS
	PthreadCheck(::pthread_mutex_destroy(&m_mutex));
#elif defined(WIN32)
	::DeleteCriticalSection(&m_sect);
#endif
}

#if UCFG_USE_PTHREADS

class CPthreadMutexAttr {
public:
	typedef CPthreadMutexAttr class_type;

	pthread_mutexattr_t m_attr;

	CPthreadMutexAttr() {
		PthreadCheck(::pthread_mutexattr_init(&m_attr));
	}
		
	~CPthreadMutexAttr() {
		PthreadCheck(::pthread_mutexattr_destroy(&m_attr));
	}

	int get_Type() {
		int kind;
		PthreadCheck(::pthread_mutexattr_gettype(&m_attr, &kind));
		return kind;
	}

	void put_Type(int kind) {
		PthreadCheck(::pthread_mutexattr_settype(&m_attr, kind));
	}
	DEFPROP(int, Type);
};



#endif

void CCriticalSection::Init() {
#if UCFG_USE_PTHREADS
	CPthreadMutexAttr a;
	a.Type = PTHREAD_MUTEX_RECURSIVE;
	PthreadCheck(::pthread_mutex_init(&m_mutex, &a.m_attr));
#elif defined(WDM_DRIVER)
	KeInitializeSpinLock(&m_spinLock);
#else
	__try {
		::InitializeCriticalSection(&m_sect);
	} __except (EXCEPTION_EXECUTE_HANDLER) {		// can occur in Win < Vista
		Throw(GetExceptionCode());
	}
#endif
}

bool CCriticalSection::lock(uint32_t dwTimeout) {
	lock();
	return true;
}

bool CCriticalSection::TryLock() {
#if UCFG_USE_PTHREADS
	int r = ::pthread_mutex_trylock(&m_mutex);
	if (r == EBUSY)
		return false;
	PthreadCheck(r);
	return true;
#elif defined(WDM_DRIVER)
	Throw(E_NOTIMPL);
#else
	return ::TryEnterCriticalSection(&m_sect);				// don't handle EXCEPTION_POSSIBLE_DEADLOCK and "OutOfMemory on Win2K"
#endif
}

/*

DWORD CCriticalSection::SetSpinCount(DWORD dwSpinCount)
{
return SetCriticalSectionSpinCount(&m_sect, dwSpinCount);
}*/



#if UCFG_USE_PTHREADS

CEvent::CEvent(bool bInitiallyOwn, bool bManualReset) {
	m_bManual = bManualReset;
	m_bSignaled = bInitiallyOwn;	
	PthreadCheck(::pthread_cond_init(&m_cond, NULL));
}

#else

CEvent::CEvent(bool bInitiallyOwn, bool bManualReset, RCString name
#ifdef WIN32
			   , LPSECURITY_ATTRIBUTES lpsaAttribute
#endif
)
:	CSyncObject(name)
{
#if UCFG_WDM
	::KeInitializeEvent(&m_ev, bManualReset ? NotificationEvent : SynchronizationEvent, (BOOLEAN)bInitiallyOwn);
	Attach(&m_ev);
#else
	AttachCreated(::CreateEvent(lpsaAttribute, bManualReset, bInitiallyOwn, name));
#endif
}

#endif

CEvent::~CEvent() {
#if UCFG_USE_PTHREADS
	PthreadCheck(::pthread_cond_destroy(&m_cond));
#elif UCFG_WDM
	Detach();
#endif
}

void CEvent::Set() {
#if UCFG_USE_PTHREADS
    EXT_LOCK (m_mutex) {
		m_bSignaled = true;
		if (m_bManual)
			PthreadCheck(::pthread_cond_broadcast(&m_cond));
		else
			PthreadCheck(::pthread_cond_signal(&m_cond));
	}
#elif UCFG_WDM
	::KeSetEvent(Obj(), IO_NO_INCREMENT, FALSE);
#else		
	Win32Check(::SetEvent(HandleAccess(*this)));
#endif
}

#if !UCFG_USE_PTHREADS && !defined(WDM_DRIVER)
void CEvent::Pulse() {
	Win32Check(::PulseEvent(HandleAccess(*this)));
}
#endif

void CEvent::Reset() {
#if UCFG_USE_PTHREADS
	EXT_LOCK (m_mutex) {
		m_bSignaled = false;
	}
#elif UCFG_WDM
	::KeClearEvent(Obj());
#else
	Win32Check(::ResetEvent(HandleAccess(_self)));
#endif
}

bool CEvent::lock(uint32_t dwTimeout) {
#if UCFG_USE_PTHREADS
	timespec abstime;
	if (dwTimeout != INFINITE)
		abstime = DateTime::UtcNow()+TimeSpan::FromMilliseconds(dwTimeout);
	EXT_LOCK (m_mutex) {
		while (!m_bSignaled) {
			if (dwTimeout == INFINITE)
				PthreadCheck(::pthread_cond_wait(&m_cond, &m_mutex.m_mutex));
			else {
				switch (int rc = ::pthread_cond_timedwait(&m_cond, &m_mutex.m_mutex, &abstime)) {
					case 0: continue;
					case ETIMEDOUT:
						if (INFINITE == dwTimeout)
							Throw(E_FAIL);
						return false;
					default:
						PthreadCheck(rc);
				}
			}
		}
		if (!m_bManual)
			m_bSignaled = false;
		return true;
	}
#else
	return base::lock(dwTimeout);
#endif
}

void CEvent::unlock() {
}

#if UCFG_WIN32

static class AutoEventPool {
public:
	~AutoEventPool() {
		EXT_FOR (HANDLE h, m_handles) {
			Win32Check(::CloseHandle(h));
		}
	}

	HANDLE AllocHandle() {
		HANDLE r = 0;
		EXT_LOCK (m_mtx) {
			if (!m_handles.empty()) {
				r = m_handles.back();
				m_handles.pop_back();
			}
		}
		if (!r)
			return ::CreateEvent(0, FALSE, FALSE, 0);
		Win32Check(::ResetEvent(r));
		return r;		
	}

	void ReleaseHandle(HANDLE h) {
		EXT_LOCK (m_mtx) {
			m_handles.push_back(h);
		}
	}
private:
	mutex m_mtx;
	vector<HANDLE> m_handles;
} s_autoEventPool;

HANDLE AFXAPI AutoResetEvent::AllocatePooledHandle() {
	return s_autoEventPool.AllocHandle();
}

void AFXAPI AutoResetEvent::ReleasePooledHandle(HANDLE h) {
	s_autoEventPool.ReleaseHandle(h);
}

#endif // UCFG_WIN32


#if UCFG_USE_PTHREADS

#else

CMutex::CMutex(bool bInitiallyOwn, RCString name
#ifdef WIN32
			   , LPSECURITY_ATTRIBUTES lpsaAttribute
#endif			   
			   )
:	CSyncObject(name)
{
#if UCFG_WDM
	::KeInitializeMutex(&m_mutex, 0);
	Attach(&m_mutex);
#else
	AttachCreated(::CreateMutex(lpsaAttribute, bInitiallyOwn, name));
#endif
}

#endif

#if UCFG_WIN32_FULL

CMutex::CMutex(RCString name, DWORD dwAccess, bool bInherit)
	:	CSyncObject(name)
{
	Attach(::OpenMutex(dwAccess, bInherit, name));
}
#endif

CMutex::~CMutex() {
#if UCFG_USE_PTHREADS
#elif UCFG_WDM
	Detach();
#endif
}


void CMutex::unlock() {
#ifdef WIN32
	Win32Check(::ReleaseMutex(HandleAccess(_self)));
#elif UCFG_WDM
	KeReleaseMutex(&m_mutex, FALSE);
#else
	Throw(E_FAIL); //!!!TODO
#endif
}

CSemaphore::CSemaphore(LONG lInitialCount, LONG lMaxCount, RCString name
#ifdef WIN32
	   , LPSECURITY_ATTRIBUTES lpsaAttributes
#endif
)
	:	CSyncObject(name)
{
#ifdef WIN32
	AttachCreated(::CreateSemaphore(lpsaAttributes, lInitialCount, lMaxCount, name));
#elif UCFG_WDM	
	KeInitializeSemaphore(&m_sem, lInitialCount, lMaxCount);
	Attach(&m_sem);
#elif UCFG_USE_PTHREADS
	if (name.empty()) {
		m_psem = &m_sem;
		CCheck(::sem_init(&m_sem, 0, lInitialCount));
	} else {
		m_psem = ::sem_open(name, O_CREAT);
		if (m_psem == SEM_FAILED) {
			m_psem = 0;			
			CCheck(-1);
		}
	}
#endif
}

CSemaphore::~CSemaphore() {
#if UCFG_USE_PTHREADS
	if (m_psem) {
		if (m_psem == &m_sem)
			CCheck(::sem_destroy(&m_sem));
		else
			CCheck(::sem_close(m_psem));
	}
#endif
}

LONG CSemaphore::Unlock(LONG lCount) {
#ifdef WIN32
	LONG r; //!!!
	Win32Check(::ReleaseSemaphore(HandleAccess(_self), lCount, &r));
	return r;
#elif UCFG_WDM
	return KeReleaseSemaphore(&m_sem, 0, lCount, FALSE);
#elif UCFG_USE_PTHREADS
	CCheck(::sem_post(m_psem));
	return 0;	//!!!?
#endif
}

void CSemaphore::unlock() {
	Unlock(1); 
}



} // Ext::



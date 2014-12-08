#pragma once


#if UCFG_WIN32 && !UCFG_WCE
#	include <winternl.h>
#endif

#if UCFG_USE_PTHREADS
#	include <semaphore.h>
#endif

#if !UCFG_STDSTL || !UCFG_CPP11_HAVE_MUTEX
#	include <el/stl/mutex>

namespace std {
	using ExtSTL::mutex;
	using ExtSTL::lock_guard;
}

#endif

#if !UCFG_STDSTL || !UCFG_CPP11_HAVE_THREAD
#	include <el/stl/thread>

namespace std {
	using ExtSTL::thread;
}

#endif

#include EXT_HEADER_FILESYSTEM

namespace Ext {
using std::sys::exists;
using std::sys::path;
using std::sys::copy_file;
using std::sys::copy_options;
using std::sys::current_path;
using std::sys::create_directory;
using std::sys::create_directories;
using std::sys::temp_directory_path;
using std::sys::directory_iterator;
using std::sys::is_regular_file;
using std::sys::is_directory;

class AFX_CLASS COperatingSystem {
	typedef COperatingSystem class_type;
public:

#if UCFG_WDM
	typedef RTL_OSVERSIONINFOEXW COsVerInfo;
#elif UCFG_WCE
	typedef OSVERSIONINFO COsVerInfo;
#elif UCFG_WIN32
	typedef OSVERSIONINFOEX COsVerInfo;
#endif

	static String AFXAPI get_ComputerName();
	DEFPROP_GET(String, ComputerName);

	static String AFXAPI get_UserName();
	DEFPROP_GET(String, UserName);

#if UCFG_WIN32 || UCFG_USE_POSIX
	static path AFXAPI get_ExeFilePath();
	DEFPROP_GET(path, ExeFilePath);
#endif

#ifdef _WIN32
	EXT_API static COsVerInfo AFXAPI get_Version();
	DEFPROP_GET(COsVerInfo, Version);
#endif

#if UCFG_WIN32
	static DWORD AFXAPI GetSysColor(int nIndex);
	static void AFXAPI MessageBeep(UINT uType = 0xFFFFFFFF);
	EXT_API static CStringVector AFXAPI QueryDosDevice(RCString dev);

	/*!!!
	static SYSTEM_INFO get_Info() {
		SYSTEM_INFO si;
		::GetSystemInfo(&si);
		return si;
	}
	DEFPROP_GET(SYSTEM_INFO, Info);
	*/

	bool Is64BitNativeSystem();

	static path AFXAPI get_WindowsDirectory();
	DEFPROP_GET(path, WindowsDirectory);

#	if UCFG_WIN32_FULL
	EXT_API static std::vector<String> AFXAPI get_LogicalDriveStrings();
	DEFPROP_GET(std::vector<String>, LogicalDriveStrings);
#	endif

#endif

	static Int64 AFXAPI get_PerformanceCounter();
	DEFPROP_GET(Int64, PerformanceCounter);

	static Int64 AFXAPI get_PerformanceFrequency();
	DEFPROP_GET(Int64, PerformanceFrequency);
};

extern EXT_DATA COperatingSystem System;

class CEvent : public CSyncObject {
	typedef CSyncObject base;
public:
	CEvent(std::nullptr_t p) {
	}
#if UCFG_USE_PTHREADS
	CEvent(bool bInitiallyOwn = false, bool bManualReset = false);
#else
	CEvent(bool bInitiallyOwn = false, bool bManualReset = false, RCString name = nullptr
#ifdef WIN32
		, LPSECURITY_ATTRIBUTES lpsaAttribute = 0
#endif
		);
#endif
	~CEvent();

#if UCFG_WDM
	KEVENT *Obj() { return (KEVENT*)m_pObject; }
#endif

#if !UCFG_USE_PTHREADS && !defined(WDM_DRIVER)
	void Pulse();
#endif

	void Set();
	void Reset();
	bool Lock(UInt32 dwTimeout = INFINITE);
	void Unlock();
private:
#if UCFG_USE_PTHREADS
	pthread_cond_t m_cond;
	CCriticalSection m_mutex;
	CBool m_bManual, m_bSignaled;
#elif UCFG_WDM
	KEVENT m_ev;
#endif
};

typedef CEvent EventWaitHandle;

class AutoResetEvent : public EventWaitHandle {
	typedef EventWaitHandle base;
public:
	AutoResetEvent(std::nullptr_t p)
		:	base(p)
	{}

	AutoResetEvent(bool bInit = false)
		:	base(bInit, false)
	{}

#if UCFG_WIN32
	static HANDLE AFXAPI AllocatePooledHandle();
	static void AFXAPI ReleasePooledHandle(HANDLE h);
#endif
};

class ManualResetEvent : public EventWaitHandle {
	typedef EventWaitHandle base;
public:
	ManualResetEvent(bool bInit = false)
		:	base(bInit, true)
	{}
};


class CMutex : public CSyncObject {
public:
	CMutex(bool bInitiallyOwn = false, RCString name = nullptr
#ifdef WIN32
		, LPSECURITY_ATTRIBUTES lpsaAttribute = 0
#endif
	);
#if UCFG_WIN32_FULL
	CMutex(RCString name, DWORD dwAccess = MUTEX_ALL_ACCESS, bool bInherit = false);
#endif
	~CMutex();

#if UCFG_WDM
	KMUTEX *Obj() { return (KMUTEX*)m_pObject; }
#endif

	void Unlock();
private:
#if UCFG_USE_PTHREADS
#elif UCFG_WDM
	KMUTEX m_mutex;
#endif
};

class AFX_CLASS CSemaphore : public CSyncObject {
public:
	CSemaphore(LONG lInitialCount = 1, LONG lMaxCount = 1, RCString name = nullptr
#ifdef WIN32		
		, LPSECURITY_ATTRIBUTES lpsaAttributes = 0
#endif
		);
#if UCFG_WIN32_FULL
	CSemaphore(RCString name, DWORD dwAccess = SEMAPHORE_ALL_ACCESS, bool bInherit = false);
#endif
	~CSemaphore();

#if UCFG_USE_PTHREADS
	sem_t m_sem, *m_psem;
#endif

#if UCFG_WDM
	KSEMAPHORE m_sem;
	KSEMAPHORE *Obj() { return (KSEMAPHORE*)m_pObject; }
#endif

#if UCFG_USE_PTHREADS
	bool Lock(DWORD dwTimeout = INFINITE) {
		while (true) {
			int rc = ::sem_wait(m_psem);
			if (!rc)
				return true;
			if (errno != EINTR)
				CCheck(-1);				
		}
	}
#endif

	void Unlock();
	LONG Unlock(LONG lCount);
};


} // Ext::






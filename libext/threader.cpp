/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#pragma warning(disable: 4073)
#pragma init_seg(lib)

#if UCFG_WIN32
#	include <winuser.h>

#	include <el/libext/win32/ext-win.h>
#endif

#ifndef UCFG_THREAD_SUSPEND_ON_START
#	define UCFG_THREAD_SUSPEND_ON_START 0
#endif

#if UCFG_EXTENDED && UCFG_GUI
#	include <el/gui/controls.h>
#endif

//!!!#include <exception>//!!!

#if !UCFG_STDSTL || !UCFG_STD_MUTEX || UCFG_SPECIAL_CRT
#	include <el/stl/mutex>
#endif

using namespace std;
using namespace Ext;

class CListDestructibleTlses {
public:
	void insert(CDestructibleTls& tls) {
		EXT_LOCKED(m_mtx, m_tlses.push_back(tls));
	}

	void erase(CDestructibleTls& tls) {
		EXT_LOCKED(m_mtx, m_tlses.erase(IntrusiveList<CDestructibleTls>::iterator(&tls)));
	}

	void ThreadCleanup() {
		EXT_LOCK (m_mtx) {
			EXT_FOR (CDestructibleTls& dtls, m_tlses) {
				if (void *p = dtls.get_Value())
					dtls.OnThreadDetach(p);
			}
		}
	}
private:
	CCriticalSection m_mtx;						// MS std::mutex does not work in DllMain
	IntrusiveList<CDestructibleTls> m_tlses;
};

static InterlockedSingleton<CListDestructibleTlses> s_pDestructibleTlses;

static void TlsCleanup(void *arg) {
	s_pDestructibleTlses->ThreadCleanup();
}

#if UCFG_USE_PTHREADS

static pthread_key_t s_keyCleanup;
static volatile int s_initCleanup = ::pthread_key_create(&s_keyCleanup, &TlsCleanup);

#elif defined(_MSC_VER)

static void NTAPI TlsCallback(PVOID dllHandle, DWORD reason, PVOID reserved) {
	if (reason == DLL_THREAD_DETACH)
		TlsCleanup(0);
}

extern "C" {

extern DWORD _tls_used;
DWORD volatile dw = _tls_used;	// forces a tls directory to be created, volatile prevent optimization

#pragma section(".CRT$XLC",long,read)
 __declspec(allocate(".CRT$XLC")) PIMAGE_TLS_CALLBACK _xl_y  = TlsCallback;
} // "C"

#endif // _MSC_VER

namespace ExtSTL {
#if !UCFG_STDSTL || !UCFG_STD_THREAD
	ostream& AFXAPI operator<<(ostream& os, const thread::id& v);
#endif	// !UCFG_STDSTL || !UCFG_STD_THREAD

#if UCFG_WCE

ostream& operator<<(ostream& os, const thread::id& v) {
	return os << v.m_tid;
}

#endif

} // ExtSTL::



namespace Ext {

using namespace std;

size_t ThreadBase::DefaultStackSize;

ThreadBase::ThreadBase(thread_group *ownRef)
	: StackSize(0)
	, StackOffset(0)
	, m_bAutoDelete(true)
	, m_owner(ownRef)
	, m_bStop(false)
	, m_bInterruptionEnabled(true)
	, m_bAPC(false)
#if UCFG_WIN32
	, m_nThreadID(0)
#endif
#if UCFG_USE_PTHREADS
	, m_tid()
	, m_ptid((pthread_t)0)
#elif UCFG_OLE
	, m_usingCOM(E_NoInit)
	, m_dwCoInit(COINIT_APARTMENTTHREADED)
#endif
{
	ASSERT(1000 < std::abs(int32_t((uint8_t*)&ownRef - (uint8_t*)this))); // Thread cannot be in stack
}

ThreadBase::~ThreadBase() {
	if (m_threadRef) {
		m_threadRef->StopChilds();
		delete m_threadRef;
	}
#if UCFG_WIN32
	if (_AFX_THREAD_STATE *ats = exchange(m_pAfxThreadState, nullptr))
		delete ats;
#endif
#if UCFG_USE_POSIX
	if (m_ptid)
		PthreadCheck(::pthread_detach(m_ptid));
#endif
}

#if UCFG_WIN32

_AFX_THREAD_STATE& ThreadBase::AfxThreadState() {
	if (!m_pAfxThreadState)
		m_pAfxThreadState.reset(new _AFX_THREAD_STATE);
	return *m_pAfxThreadState;
}

#endif  // UCFG_WIN32

//!!!DCTls Thread::t_pCurThreader;//!!!

//!!!#if UCFG_EXTENDED

ThreadBase::propclass_CurrentThread ThreadBase::CurrentThread; //!!!

//!!!Rstatic CTls s_currentThread;

static class CThreadDestructibleTls : public CDestructibleTls {
	typedef CDestructibleTls base;
public:
	CThreadDestructibleTls& operator=(const ThreadBase* p) {
		base::put_Value(p);
		return _self;
	}

	operator ThreadBase*() const { return (ThreadBase*)get_Value(); }
	ThreadBase* operator->() const { return operator ThreadBase*(); }

	void OnThreadDetach(void *p) override {
		CCounterIncDec<ThreadBase, InterlockedPolicy>::Release((ThreadBase*)p);
	}
} t_pCurThread;


//EXT_THREAD_PTR(ThreadBase, t_pCurThread);

ThreadBase* ThreadBase::get_CurrentThread() {
	ThreadBase* r = t_pCurThread;
	if (!r) {
		(r = new Thread)->AttachSelf();				//!!! Leak
	}
	return r;
}

ThreadBase* AFXAPI ThreadBase::TryGetCurrentThread() {
	return t_pCurThread;
}

void ThreadBase::SetCurrentThread() {
	t_pCurThread = this;
}

//!!!#endif

void ThreadBase::Sleep(DWORD dwMilliseconds) {
#if UCFG_WIN32 || UCFG_USE_PTHREADS
	CurrentThread->SleepImp(dwMilliseconds); //!!!
#else
	::usleep(dwMilliseconds * 1000);
#endif
}

#if UCFG_USE_PTHREADS

int PthreadCheck(int code, int allowableError) {
	if (code && (code != allowableError))
		Throw(HRESULT_FROM_C(code));
	return code;
}

void ThreadBase::ReleaseHandle(intptr_t h) const {
	m_tid = thread::id();
	if (pthread_t ptid = exchange(m_ptid, (pthread_t)0))
		PthreadCheck(::pthread_detach(ptid));
}

#endif

#if UCFG_WIN32
HANDLE GetRealThreadHandle() {
	HANDLE r;
	Win32Check(::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &r, 0, FALSE, DUPLICATE_SAME_ACCESS));
	return r;
}
#endif //  UCFG_WIN32

void ThreadBase::Attach(intptr_t h, uint32_t id, bool bOwn) {
#if UCFG_THREAD_SUSPEND_ON_START
	SafeHandle::Attach(h, bOwn);
#else
	if (h == 0 || h == m_invalidHandleValue) {
#if UCFG_USE_POSIX
		CCheck(-1);
#elif UCFG_WIN32
		Win32Check(false);
#endif
	}
	ThreadSafeAttach(h, bOwn);
#endif
#if UCFG_WIN32
	m_nThreadID = id;
#endif
#if UCFG_WIN32 && !UCFG_STDSTL //!!!?
	m_tid.m_tid = id;
#endif
}

void ThreadBase::AttachSelf() {
	if (!TryGetCurrentThread())
		SetCurrentThread();
#if UCFG_WIN32
	Attach((intptr_t)GetRealThreadHandle(), ::GetCurrentThreadId());
#	if !UCFG_STDSTL
	m_tid = std::this_thread::get_id();
#	endif
#else
	m_ptid = ::pthread_self();
#	if UCFG_STD_THREAD
	m_tid = std::this_thread::get_id();
#	else
	m_tid = thread::id(__gthread_self());
#	endif
#endif
}

thread_group& ThreadBase::GetThreadRef() {
	if (!m_threadRef)
		m_threadRef.reset(new thread_group);
	return *m_threadRef;
}

void ThreadBase::SleepImp(unsigned long ms) {
#if UCFG_WIN32_FULL
	if (m_bAPC)
		m_bAPC = false;
	else if (!m_bStop)
		SleepEx(ms, TRUE);
#else
	usleep(ms * 1000);
#endif
	interruption_point();
}


/*!!!
LPTOP_LEVEL_EXCEPTION_FILTER g_pfnUnhadledExceptionFilter;

void __stdcall MyExceptHandler(EXCEPTION_POINTERS *ep)
{
TRC(1, "MyExcept_handler");//!!!T
if (g_pfnUnhadledExceptionFilter)
g_pfnUnhadledExceptionFilter(ep);
else
{
AfxLogErrorCode(0xFFFFFFFF); //!!1
::MessageBox(0, "Fatal Error", "Critical Error!\nReport about it to " MANUFACTURER_EMAIL, MB_ICONEXCLAMATION); //!!!
}
ExitProcess(3);
}

void __cdecl Myterminate_handler()//!!!
{
__try
{
RaiseException(0x02345678, 0, 0, 0);
}
__except(MyExceptHandler(GetExceptionInformation()))
{}
AfxLogErrorCode(0xFFFFFFFF); //!!1
::MessageBox(0, "Fatal Error", "Critical Error!\nReport about it to " MANUFACTURER_EMAIL, MB_ICONEXCLAMATION); //!!!
ExitProcess(3);
}
*/

#if UCFG_OLE

void ThreadBase::InitCOM() {
	m_usingCOM.Initialize(m_dwCoInit);
}

#endif

thread_group::thread_group(bool bSync)
	: m_aRef(0)
	, m_bSync(bSync)
	, aRefCountActiveThreads(0)
{
}

thread_group::~thread_group() {
	if (m_bSync)			//!!!?
		StopChilds();
}

int thread_group::size() const {
	return EXT_LOCKED(m_cs, (int)m_threads.size());
}

void thread_group::add_thread(ThreadBase *t) {
	EXT_LOCK (m_cs) {
		m_threads.insert(t);
		ExternalAddRef();
	}
}

void thread_group::remove_thread(ThreadBase *t) {
	EXT_LOCK (m_cs) {
		if (t->m_bAutoDelete) {
			m_threads.erase(t);
		}
	}
}

void thread_group::interrupt_all() {
	EXT_LOCK (m_cs) {
		for (CThreadColl::iterator it = m_threads.begin(), e = m_threads.end(); it != e; ++it) {
			ThreadBase *t = it->get();
			t->m_bAutoDelete = false;
			if (*t)
				t->interrupt();
		}
	}
}

void thread_group::join_all() {
	CThreadColl ar;
	EXT_LOCKED(m_cs, ar.swap(m_threads));
	for (CThreadColl::iterator i(ar.begin()), e(ar.end()); i!=e; ++i) {
		const ptr<ThreadBase>& r = *i;
		if (r->Valid())
			r->Join();

		//!!!    int ec = t->ExitCode;
	}
}

void thread_group::StopChilds() {
	interrupt_all();
	join_all();
}

bool thread_group::StopChild(ThreadBase *t, int msTimeout) {
	EXT_LOCK (m_cs) {
		t->m_bAutoDelete = false;
		if (*t)
			t->interrupt();
	}
	if (*t) {
		if (!t->Join(msTimeout))
			return false;
	}

	EXT_LOCKED(m_cs, m_threads.erase(t));
	return true;
}

void ThreadBase::Delete() {
	if (m_owner) {
		m_owner->ExternalRelease();
		m_owner->remove_thread(this);
	}
}

void ThreadBase::Execute() {
}

uint32_t ThreadBase::get_ExitCode() {
#if UCFG_USE_PTHREADS
	return m_exitCode;
#else
	DWORD r;
	Win32Check(::GetExitCodeThread(HandleAccess(_self), &r));
	return r;
#endif
}

void ThreadBase::put_Name(RCString name) {
	m_name = name;
#if UCFG_USE_PTHREADS && defined(HAVE_PTHREAD_SETNAME_NP)
	CCheck(pthread_setname_np(m_ptid, name.Left(15)));
#elif UCFG_WIN32
	struct THREADNAME_INFO {
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	};

	ASSERT(m_nThreadID);
	THREADNAME_INFO info = { 0x1000, m_name, m_nThreadID };
	__try {
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR*)&info);
	} __except(EXCEPTION_CONTINUE_EXECUTION) {
	}
#endif
}

int ThreadBase::get_Priority() {
#if UCFG_USE_PTHREADS
	return 0;
#else
	int p = ::GetThreadPriority(HandleAccess(_self));
	Win32Check(p != THREAD_PRIORITY_ERROR_RETURN);
	return p;
#endif
}

void ThreadBase::put_Priority(int nPriority) {
#if !UCFG_USE_PTHREADS
	Win32Check(::SetThreadPriority(HandleAccess(_self), nPriority));
#endif
}

void ThreadBase::OnEnd() {
#if UCFG_OLE
	m_usingCOM.Uninitialize();
#endif
}

void ThreadBase::interruption_point() {
	if (m_bInterruptionEnabled && m_bStop)
		Throw(ExtErr::ThreadInterrupted);
}

void ThreadBase::OnAPC() {
	m_bAPC = true;
//!!!	if (m_bStop)
//!!!		Socket::ReleaseFromAPC();
}

#if !UCFG_USE_PTHREADS

CONTEXT ThreadBase::get_Context(DWORD contextFlags) {
	CONTEXT context;
	context.ContextFlags = contextFlags;
	Win32Check(::GetThreadContext(HandleAccess(_self), &context));
	return context;
}

void ThreadBase::put_Context(const CONTEXT& context) {
	Win32Check(::SetThreadContext(HandleAccess(_self), &context));
}

CTimesInfo ThreadBase::get_Times() const {
	CTimesInfo r;
	Win32Check(::GetThreadTimes(HandleAccess(_self), &r.m_tmCreation, &r.m_tmExit, &r.m_tmKernel, &r.m_tmUser));
	return r;
}

DWORD ThreadBase::Resume() {
	DWORD prevCount = ::ResumeThread(HandleAccess(_self));
	Win32Check(prevCount != 0xFFFFFFFF);
	return prevCount;
}

DWORD ThreadBase::Suspend() {
	DWORD prevCount = ::SuspendThread(HandleAccess(_self));
	Win32Check(prevCount != 0xFFFFFFFF);
	return prevCount;
}

void ThreadBase::PostMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	Win32Check(::PostThreadMessage(m_nThreadID, message, wParam, lParam));
}


#	if !UCFG_WCE

VOID CALLBACK ThreadBase::APCProc(ULONG_PTR param) {
	ThreadBase *t = (ThreadBase*)param;
	if (t->m_bInterruptionEnabled)
		t->OnAPC();
}

void ThreadBase::QueueUserAPC(PAPCFUNC pfnAPC, ULONG_PTR dwData) {
	static DWORD winMajVersion = System.Version.dwMajorVersion;
	if (::QueueUserAPC(pfnAPC, HandleAccess(_self), dwData))
		return;
	if (winMajVersion >= 6)
		Win32Check(false, ERROR_GEN_FAILURE);
	else if (WaitForSingleObject(HandleAccess(_self), 0) == WAIT_TIMEOUT)
		Throw(ExtErr::QueueUserAPC);				// No GetLastError value
}

void ThreadBase::QueueAPC() {
	try {
		QueueUserAPC(&APCProc, (DWORD_PTR)this);
	} catch (RCExc) {
	}
}
#	endif // !UCFG_WCE

#endif // UCFG_USE_PTHREADS

bool ThreadBase::Join(int ms) {
#if UCFG_USE_PTHREADS
	ASSERT(ms == INFINITE);
	void *r;
	PthreadCheck(::pthread_join(m_ptid, &r));
	m_ptid = (pthread_t)0;
	m_exitCode = (uint32_t)(uintptr_t)r;
	return true;
#else
	int r = ::WaitForSingleObject(HandleAccess(_self), ms);
	if (r == WAIT_TIMEOUT)
		return false;
	Win32Check(r != WAIT_FAILED);
	return true;
#endif
}

#if UCFG_USE_PTHREADS
static void Sigusr1Handler(int sig) {
}
#endif

void ThreadBase::interrupt() {
	Stop();
}

void ThreadBase::Stop() {
	m_bStop = true;
	if (Valid()) {
		if (m_bInterruptionEnabled) {		//!!!
#if UCFG_USE_PTHREADS
			static bool s_bSigusr1HandlerInited;
			if (!s_bSigusr1HandlerInited) {
				struct sigaction act = { 0 };
				::sigemptyset(&act.sa_mask);
				act.sa_handler = &Sigusr1Handler;
				::sigaction(SIGUSR1, &act, nullptr);
				s_bSigusr1HandlerInited = true;
			}
			PthreadCheck(::pthread_kill(m_ptid, SIGUSR1));  //!!!? may be other signal

#elif !UCFG_WCE
			try {
				DBG_LOCAL_IGNORE_WIN32(ERROR_GEN_FAILURE);

				QueueAPC();
			} catch (RCExc) {
			}
#endif
		}
	}
}

void ThreadBase::WaitStop() {
	if (Valid())
		Join();
}

uint8_t __afxThreadData[sizeof(CThreadSlotData)];
CThreadSlotData *_afxThreadData;

void CThreadSlotData::DeleteValues(CThreadData *pData, HINSTANCE hInst) {
	bool bDelete = true;
	for (size_t i = 1; i < pData->size(); i++) {
		if (!hInst || m_arSlotData[i].hInst == hInst) {
			if ((*pData)[i])										// delete the data since hInst matches (or is NULL)
				delete exchange((*pData)[i], nullptr);
		} else if ((*pData)[i])
			bDelete = false;
	}
	if (bDelete) {
		EXT_LOCK (m_criticalSection) {
			m_tdatas.erase(pData->ThreadId);
			m_tls.Value = 0;
		}
	}
}

#define SLOT_USED   0x01    // slot is allocated



//!!!__declspec(nothrow)
CThreadSlotData::CThreadSlotData()
	: m_nRover(1)
	, m_nMax(0)
{
}

void CThreadSlotData::DeleteValues(HINSTANCE hInst, bool bAll) {
	EXT_LOCK (m_criticalSection) {
		if (bAll) {
			for (CTDatas::iterator it = m_tdatas.begin(), e = m_tdatas.end(); it != e; ++it)
				DeleteValues(&it->second, hInst);
		} else if (CThreadData* pData = (CThreadData*)(void*)m_tls.Value)
			DeleteValues(pData, hInst);
	}
}

CThreadSlotData::~CThreadSlotData() {
	DeleteValues(0, true);
}

void AFXAPI AfxTermThread(HINSTANCE hInstTerm) {
	if (_afxThreadData)
		_afxThreadData->DeleteValues(hInstTerm, false);		//!!!
}

void AFXAPI AfxEndThread(UINT nExitCode, bool bDelete) {
#if UCFG_EXTENDED || UCFG_WIN32
	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
	if (ThreadBase *pThread = Thread::TryGetCurrentThread()) {
		pThread->OnEnd();
		if (bDelete)
			pThread->Delete();
	}
#endif
	t_pCurThread = nullptr;
	AfxTermThread();
#if UCFG_USE_PTHREADS
	::pthread_exit((void*)(uintptr_t)nExitCode);
#elif UCFG_WCE
	::ExitThread(nExitCode);
#else
	_endthreadex(nExitCode);
#endif
}

void ThreadBase::OnEndThread(bool bDelete) {
	if (!!Name) {
		TRC(6, Name);
	}

	OnEnd();
	if (bDelete)
		Delete();
	AfxTermThread();
}

uint32_t ThreadBase::CppThreadThunk() {
	SetCurrentThread();
#if UCFG_USE_POSIX
	m_ptid = ::pthread_self();
#	if UCFG_STD_THREAD
	m_tid = std::this_thread::get_id();
#	else
	m_tid = thread::id(__gthread_self());
#	endif
#else
#	if !UCFG_THREAD_SUSPEND_ON_START
	Attach((intptr_t)GetRealThreadHandle(), ::GetCurrentThreadId());
#	endif
#endif
	uint32_t exitCode = (UINT)E_FAIL; //!!!
	alloca(StackOffset);							// to prevent cache line aliasing
	try {
#ifdef X_CPPRTTI
		try {
			const type_info& ti = typeid(*this);
			Name = ti.name();
		} catch (const bad_typeid& ex) {		//!!! option /GR- in VC++, really don't work by unknown reason
		}
#endif // _CPPRTTI
		{
			struct ActiveThreadKeeper {
				thread_group *m_tr;

				ActiveThreadKeeper(thread_group *tr)
					: m_tr(tr)
				{
					if (m_tr)
						++(m_tr->aRefCountActiveThreads);
				}

				~ActiveThreadKeeper() {
					if (m_tr)
						--(m_tr->aRefCountActiveThreads);
				}
			} actveThreadKeeper(m_owner);

			DBG_LOCAL_IGNORE_CONDITION(ExtErr::ThreadInterrupted);

			Execute();
			exitCode = m_exitCode;
		}
		OnEndThread(true);
	} catch (thread_interrupted& ex) {
		exitCode = m_exitCode = ex.code().value();
		TRC(1, ex.what() << " in Thread: " << get_id() << "\t" << Name);
		OnEndThread(true);
	} catch (...) {
		TRC(0, "Unhandled exception in Thread: " << get_id() << "\t" << Name);
		ProcessExceptionInCatch();
		OnEndThread(true);
	}
	CCounterIncDec<ThreadBase, InterlockedPolicy>::Release(this);
	return exitCode;
}

#if UCFG_WCE

UINT ThreadBase::ThreaderFunction(LPVOID pParam) {
	UINT r = (UINT)E_FAIL;
	__try {
//!!!R		_AFX_THREAD_STARTUP *pStartup = (_AFX_THREAD_STARTUP*)pParam;
		ThreadBase *pT = (ThreadBase*)pParam;
		r = pT->CppThreadThunk();
	} __except (ProcessExceptionInFilter(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER) {
		ProcessExceptionInExcept();
	}
	t_pCurThread = nullptr;
	return r;
}

#else

#if UCFG_USE_PTHREADS
void * __cdecl ThreadBase::ThreaderFunction(void *pParam)
#else
UINT ThreadBase::ThreaderFunction(LPVOID pParam)
#endif
{
	ThreadBase *pT = (ThreadBase*)pParam;;

#if UCFG_WIN32 && defined(_AFXDLL)
	AfxSetModuleState(pT->m_pModuleStateForThread);
	pT->m_pModuleState = pT->m_pModuleStateForThread ? pT->m_pModuleStateForThread : &_afxBaseModuleState;
#endif

	uint32_t r = pT->CppThreadThunk();
	t_pCurThread = nullptr;

#if UCFG_USE_PTHREADS
	::pthread_exit((void*)(uintptr_t)r);
	return (void*)(uintptr_t)r;
#elif UCFG_WCE
	::ExitThread(r);
	return r;
#else
	_endthreadex(r);
	return r;
#endif
}

#endif

void ThreadBase::Create(DWORD dwCreateFlags, size_t nStackSize
#ifdef WIN32
				, LPSECURITY_ATTRIBUTES lpSecurityAttrs
#endif
		)
{
	ASSERT(!Valid());

#ifdef WIN32
	m_pModuleStateForThread.reset(AfxGetModuleState());
#endif
	++base::m_aRef;	// Runned thread decrements m_aRef;
	ptr<ThreadBase> tThis = this;			// if keep this object live if the fread exits quickly
#if UCFG_USE_PTHREADS
	CAttr attr;
	if (nStackSize)
		attr.StackSize = nStackSize;
	int rc = ::pthread_create(&m_ptid, attr, &ThreaderFunction, this);
	if (rc) {
		CCounterIncDec<ThreadBase, InterlockedPolicy>::Release(this);
		PthreadCheck(rc);
	}
#	if UCFG_LIBCPP_VERSION
	reinterpret_cast<thread::native_handle_type&>(m_tid) = m_ptid;	//!!!C
#	else
	m_tid = thread::id(m_ptid);
#	endif
#else
	HANDLE h;
	DWORD threadID = 0;
	DWORD flCreate = dwCreateFlags | (UCFG_THREAD_SUSPEND_ON_START ? CREATE_SUSPENDED : 0);										// CREATE_SUSPENDED is necessary to sync access to SafeHandle
#	if UCFG_WCE
		h = ::CreateThread(lpSecurityAttrs, nStackSize, (LPTHREAD_START_ROUTINE)&ThreaderFunction, this, flCreate, &threadID);
#	else
		h = (HANDLE)_beginthreadex(lpSecurityAttrs, (UINT)nStackSize, &ThreaderFunction, this, flCreate, (UINT*)&threadID);
#	endif
	if (!h || h==(HANDLE)-1) {
		CCounterIncDec<ThreadBase, InterlockedPolicy>::Release(this);
	}
	Attach((intptr_t)h, threadID);
#	if UCFG_THREAD_SUSPEND_ON_START
	if (!(dwCreateFlags & CREATE_SUSPENDED))
		Resume();
#	endif
#endif	// !UCFG_USE_PTHREADS
}

void ThreadBase::Start(DWORD flags) {
	ptr<ThreadBase> tSelf= this;		// to prevent Memory Leak if BeforeStart() throws an Exception
	BeforeStart();
	if (m_owner) {
		m_owner->add_thread(this);
	}
	if (!StackSize) {
		if (!ThreadBase::DefaultStackSize) {
			String sStackSize = Environment::GetEnvironmentVariable("UCFG_THREAD_STACK_SIZE");
			ThreadBase::DefaultStackSize = !!sStackSize ? Convert::ToUInt32(sStackSize) : UCFG_THREAD_STACK_SIZE;
		}
		StackSize = ThreadBase::DefaultStackSize;
	}
#ifdef _WIN32
	if (StackSize)
		flags |= STACK_SIZE_PARAM_IS_A_RESERVATION;
#endif
	Create(flags, StackSize);
}

namespace this_thread {

bool AFXAPI interruption_enabled() noexcept {
	return ThreadBase::get_CurrentThread()->m_bInterruptionEnabled;
}

bool AFXAPI interruption_requested() noexcept {
	return ThreadBase::get_CurrentThread()->m_bStop;
}

void AFXAPI interruption_point() {
	ThreadBase::get_CurrentThread()->interruption_point();
}

disable_interruption::disable_interruption()
	: base(ThreadBase::get_CurrentThread()->m_bInterruptionEnabled, false)
{}

restore_interruption::restore_interruption(disable_interruption& di)
	: base(ThreadBase::get_CurrentThread()->m_bInterruptionEnabled, di.m_prev)
{}

void AFXAPI sleep_for(const TimeSpan& span) {
	DWORD ms = clamp((DWORD)duration_cast<milliseconds>(span).count(), (DWORD)0, (DWORD)INFINITE);
	ThreadBase::get_CurrentThread()->Sleep(ms);
}

} // Ext::this_thread::


CSeparateThreadThreader::CSeparateThreadThreader(CSeparateThread& st)
	: ThreadBase(&st)
	, m_st(st)
{
	m_bInterruptionEnabled = true;
}

CSeparateThreadThreader::~CSeparateThreadThreader() {
	EXT_LOCK (m_st.m_cs) {
		m_st.m_pT = nullptr;
	}
}

void CSeparateThreadThreader::BeforeStart() {
	m_st.BeforeStart();
}

void CSeparateThreadThreader::Execute() {
	m_st.Execute();
}

void CSeparateThreadThreader::Stop() {
	ThreadBase::Stop();
	m_st.Stop();
}

CSeparateThread::~CSeparateThread() {
	if (m_bSync)			//!!!?
		StopChilds();
}

void CSeparateThread::Stop() {
//!!!R		if (m_pT)							// Endless recursion
//!!!R			m_pT->Stop();
}

void CSeparateThread::Create(DWORD flags) {
	(m_pT = new CSeparateThreadThreader(_self))->Start(flags);
}

#ifdef WIN32
CWinThread::CWinThread(thread_group *ownRef)
	: ThreadBase(ownRef)
{
	m_pThreadParams = 0;
	m_pfnThreadProc = 0;
	CommonConstruct();
}

CWinThread::CWinThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam) {
	m_pfnThreadProc = pfnThreadProc;
	m_pThreadParams = pParam;
	CommonConstruct();
}

void CWinThread::CommonConstruct() {
	m_hr = 0;
	m_lpfnOleTermOrFreeLib = 0;
}

#endif



//!!!D Need for Win32s static long _afxTlsRef;

void AFXAPI AfxTlsAddRef() {
	//!!!D 	++_afxTlsRef;
}

void AFXAPI AfxTlsRelease() {
	//!!!D	if (_afxTlsRef == 0 || --_afxTlsRef == 0)
	{
		if (_afxThreadData) {
			_afxThreadData->~CThreadSlotData();
			_afxThreadData = NULL;
		}
	}
}



CTls::CTls() {
#if UCFG_USE_PTHREADS
	PthreadCheck(::pthread_key_create(&m_key, 0));
#else
	Win32Check((m_key = ::TlsAlloc()) != TLS_OUT_OF_INDEXES);
#endif
}

CTls::~CTls() {
#if UCFG_USE_PTHREADS
	PthreadCheck(::pthread_key_delete(m_key));
#else
	Win32Check(::TlsFree(m_key));
#endif
}

void CTls::put_Value(const void *p) {
#if UCFG_USE_PTHREADS
	PthreadCheck(::pthread_setspecific(m_key, p));
#else
	Win32Check(::TlsSetValue(m_key, (void*)p));
#endif
}

CDestructibleTls::CDestructibleTls() {
	s_pDestructibleTlses->insert(_self);
}

CDestructibleTls::~CDestructibleTls() {
	s_pDestructibleTlses->erase(_self);
}

void CThreadSlotData::AssignInstance(HINSTANCE hInst) {
	EXT_LOCK (m_criticalSection) {
		ASSERT(hInst != NULL);

		for (size_t i = 1; i < m_arSlotData.size(); i++) {
			if (m_arSlotData[i].hInst == NULL && (m_arSlotData[i].dwFlags & SLOT_USED))
				m_arSlotData[i].hInst = hInst;
		}
	}
}

void AFXAPI AfxInitLocalData(HINSTANCE hInst) {
	if (_afxThreadData)
		_afxThreadData->AssignInstance(hInst);
}

void AFXAPI AfxTermLocalData(HINSTANCE hInst, bool bAll) {
	if (_afxThreadData)
		_afxThreadData->DeleteValues(hInst, bAll);
}


#ifdef WIN32
DWORD AFXAPI WaitWithMS(DWORD dwMS, HANDLE h0, HANDLE h1, HANDLE h2, HANDLE h3, HANDLE h4, HANDLE h5) {
	DWORD r = WAIT_FAILED;
	if (!h1)
		r = WaitForSingleObject(h0, dwMS);
	else {
		HANDLE ar[6] = {h0, h1, h2, h3, h4, h5};
		int i;
		for (i=0; i<6; i++)
			if (!ar[i])
				break;
		r = WaitForMultipleObjects(i, ar, FALSE, dwMS);
	}
	Win32Check(r != WAIT_FAILED);
	return r;
}

DWORD AFXAPI Wait(HANDLE h0, HANDLE h1, HANDLE h2, HANDLE h3, HANDLE h4, HANDLE h5) {
	return WaitWithMS(INFINITE, h0, h1, h2, h3, h4, h5);
}
#endif




#ifdef _WIN32

#if UCFG_WIN32_FULL
void WorkItem::QueueAPC() {
	if (ExecutingThread)
		ExecutingThread->QueueAPC();
}
#endif

void WorkItem::Cancel() {
	if (!Finished) {
		Stop();
		if (ExecutingThread) {
			{
				EXT_LOCK (ExecutingThread->m_cs) {
					ExecutingThread->m_wiToCancel = this;
				}
			}
#if UCFG_WIN32_FULL
			ExecutingThread->QueueAPC();

#endif
		}
	}
}

PoolThread::PoolThread(ThreadPool& pool)
	: base(&pool.m_tr)
	, m_pool(pool)
{
}

void PoolThread::Execute() {
	Name = "PoolThread";

	while (true) {
		while (true) {
			if (m_bStop)
				return;
			ptr<WorkItem> wi;
			EXT_LOCK (ThreadPool::I->m_cs) {
				if (!ThreadPool::I->m_queue.empty()) {
					wi = ThreadPool::I->m_queue.front();
					ThreadPool::I->m_queue.pop();
				}
			}
			if (!wi)
				break;
			m_curWi = wi;
			wi->ExecutingThread = this;
			if (!wi->m_bStop) {
				try {
					wi->Execute();
				} catch (RCExc ex) {
					wi->HResult = HResultInCatch(ex);
				}
				wi->Finished = true;
				m_curWi = nullptr;
			}
		}
		EXT_LOCK (m_cs) {
			m_wiToCancel = nullptr;
		}
		int r = Wait((HANDLE)(intptr_t)Handle(m_pool.m_evStop), (HANDLE)(intptr_t)Handle(ThreadPool::I->m_sem));
	}
}

void PoolThread::OnAPC() {
	if (m_wiToCancel == m_curWi)
		m_wiToCancel->OnAPC();
}


ThreadPool *ThreadPool::I;

ThreadPool::ThreadPool()
	:	m_evStop(FALSE, TRUE)
	,	m_sem(0, UCFG_POOL_THREADS)
{
	ASSERT(I == nullptr);
	I = this;

	for (int i = 0; i < UCFG_POOL_THREADS; ++i)
		(new PoolThread(_self))->Start();
}

void ThreadPool::Ensure() {
	if (!I)
		AfxGetApp()->m_pThreadPool = new ThreadPool;
}


ptr<WorkItem> ThreadPool::QueueUserWorkItem(WorkItem *wi) {
	Ensure();
	EXT_LOCK (ThreadPool::I->m_cs) {
		ThreadPool::I->m_queue.push(wi);
		if (ThreadPool::I->m_queue.size() < UCFG_POOL_THREADS)
			ThreadPool::I->m_sem.unlock();
	}
	return wi;
}
#endif  // _WIN32

bool g_bProcessDetached;

#ifdef WIN32

int CWinThread::ExitInstance() {
#if UCFG_WIN_MSG
	return (int)AfxGetCurrentMessage().wParam;
#else
	return true;
#endif
}
#endif



} // Ext::

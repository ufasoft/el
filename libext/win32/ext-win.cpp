#include <el/ext.h>

#include <windows.h>
#include <wininet.h>
#include <commctrl.h>


#if UCFG_WND
#	include <el/libext/win32/ext-wnd.h>
#endif

#if UCFG_GUI
#	include <el/gui/gdi.h>
#	include <el/gui/menu.h>

#	if UCFG_EXTENDED
#		include <el/gui/controls.h>
#	endif
#	include <el/gui/ext-image.h>
#endif

#include <el/libext/win32/ext-win.h>
#include <el/libext/win32/ext-full-win.h>

#pragma warning(disable: 4073)
#pragma init_seg(lib)

#if UCFG_CRASH_DUMP
#	include <el/comp/crashdump.h>
#endif

namespace Ext {
#if UCFG_EXTENDED && UCFG_WIN_MSG
#	include "mfcimpl.h"
#endif

using namespace std;

#if UCFG_EXTENDED && UCFG_GUI

CCursor Cursor;

#else

/*!!!R String AFXAPI AfxProcessError(HRESULT hr) {
	return Convert::ToString(hr, 16);
}*/

#endif // UCFG_EXTENDED


#if UCFG_USE_DECLSPEC_THREAD
__declspec(thread) HRESULT t_lastHResult;
HRESULT AFXAPI GetLastHResult() { return t_lastHResult; }
void AFXAPI SetLastHResult(HRESULT hr) { t_lastHResult = hr; }
#else
CTls t_lastHResult;
HRESULT AFXAPI GetLastHResult() { return (HRESULT)(uintptr_t)t_lastHResult.Value; }
void AFXAPI SetLastHResult(HRESULT hr) { t_lastHResult.Value = (void*)(uintptr_t)hr; }
#endif // UCFG_USE_DECLSPEC_THREAD

int AFXAPI Win32Check(LRESULT i) {
	if (i)
		return (int)i;
	DWORD dw = ::GetLastError();
	if (dw & 0xFFFF0000)
		Throw(dw);
	error_code ec;
	if (dw)
		ec = error_code(dw, system_category());
	else {
		HRESULT hr = GetLastHResult();
		ec = error_code(hr ? hr : E_EXT_UnknownWin32Error, hresult_category());
	}
	Throw(ec);
}

bool AFXAPI Win32Check(BOOL b, DWORD allowableError) {
	Win32Check(b || ::GetLastError()==allowableError);
	return b;
}

void CSyncObject::AttachCreated(intptr_t h) {
	Attach(h);
	m_bAlreadyExists = GetLastError()==ERROR_ALREADY_EXISTS;
}

ProcessModule::ProcessModule(class Process& process, HMODULE hModule)
	:	Process(process)
	,	HModule(hModule)
{
	Win32Check(::GetModuleInformation((HANDLE)(intptr_t)Handle(*process.m_pimpl), hModule, &m_mi, sizeof m_mi));
}

#if !UCFG_WCE

HANDLE StdHandle::Get(DWORD nStdHandle) {
	HANDLE h = ::GetStdHandle(nStdHandle);
	Win32Check(h != INVALID_HANDLE_VALUE);
	return h;
}

vector<String> COperatingSystem::get_LogicalDriveStrings() {
	vector<String> r;
	int n = Win32Check(::GetLogicalDriveStrings(0, 0))+1;
	TCHAR *p = (TCHAR*)alloca(n*sizeof(TCHAR));
	n = Win32Check(::GetLogicalDriveStrings(n, p));
	for (; *p; p+=_tcslen(p)+1)
		r.push_back(p);
	return r;
}

String ProcessModule::get_FileName() const {
	TCHAR buf[MAX_PATH];
	Win32Check(::GetModuleFileNameEx((HANDLE)(intptr_t)Handle(*Process.m_pimpl), HModule, buf, size(buf)));
	return buf;
}

String ProcessModule::get_ModuleName() const {
	TCHAR buf[MAX_PATH];
	Win32Check(::GetModuleBaseName((HANDLE)(intptr_t)Handle(*Process.m_pimpl), HModule, buf, size(buf)));
	return buf;
}

vector<ProcessModule> GetProcessModules(Process& process) {
	vector<HMODULE> ar(5);
	for (bool bContinue=true; bContinue;) {
		DWORD cbNeeded;
		BOOL r = ::EnumProcessModules((HANDLE)(intptr_t)Handle(*process.m_pimpl), &ar[0], ar.size()*sizeof(HMODULE), &cbNeeded);
		Win32Check(r);
		bContinue = cbNeeded > ar.size()*sizeof(HMODULE);
		ar.resize(cbNeeded/sizeof(HMODULE));
	}
	vector<ProcessModule> arm;
	for (size_t i=0; i<ar.size(); ++i)
		arm.push_back(ProcessModule(process, ar[i]));
	return arm;
}

static String ToDosPath(RCString lpath) {
	vector<String> lds = System.LogicalDriveStrings;
	for (size_t i=0; i<lds.size(); ++i) {
		String ld = lds[i];
		String dd = ld.Right(1)=="\\" ? ld.Left(ld.length()-1) : ld;
		vector<String> v = System.QueryDosDevice(dd);
		if (v.size()) {
			String lp = v[0];
			if (lp.length() < lpath.length() && !lp.CompareNoCase(lpath.Left(lp.length()))) {
				return ((ld.Right(1) == "\\" && lpath.at(0) == '\\') ? dd : ld) + lpath.substr(lp.length());
			}
		}
	}
	return lpath;
}


typedef DWORD (WINAPI *PFN_GetProcessImageFileName)(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);

static CDynamicLibrary s_dllPsapi("psapi.dll");

String ProcessObj::get_MainModuleFileName() {
	TCHAR buf[MAX_PATH];
	try {
		DBG_LOCAL_IGNORE_WIN32(ERROR_PARTIAL_COPY);
		DBG_LOCAL_IGNORE_WIN32(ERROR_INVALID_HANDLE);

		Win32Check(::GetModuleFileNameEx((HANDLE)(intptr_t)Handle(_self), 0, buf, size(buf)));
		return buf;
	} catch (RCExc) {
	}
	DlProcWrap<PFN_GetProcessImageFileName> pfnGetProcessImageFileName(s_dllPsapi, EXT_WINAPI_WA_NAME(GetProcessImageFileName));
	if (pfnGetProcessImageFileName) {
		Win32Check(pfnGetProcessImageFileName((HANDLE)(intptr_t)Handle(_self), buf, size(buf)));
		return ToDosPath(buf);
	} else
		Throw(HRESULT_FROM_WIN32(ERROR_PARTIAL_COPY));
}

#endif // !UCFG_WCE

ProcessObj::ProcessObj(pid_t pid, DWORD dwAccess, bool bInherit)
	:	SafeHandle(0, false)
	,	m_pid(pid)
{
	CommonInit();
	if (pid)
		Attach((intptr_t)::OpenProcess(dwAccess, bInherit, pid));
	/*!!!
	else
	{
		Attach(::GetCurrentProcess());
		m_bOwn = false;
		m_ID = GetCurrentProcessId();
	}*/
}

void COperatingSystem::MessageBeep(UINT uType) {
	Win32Check(::MessageBeep(uType));
}


//!!!#if !UCFG_WCE && UCFG_EXTENDED

#if !UCFG_WCE

DWORD File::GetOverlappedResult(OVERLAPPED& ov, bool bWait) {
	DWORD r;
	Win32Check(::GetOverlappedResult((HANDLE)(intptr_t)HandleAccess(_self), &ov, &r, bWait));
	return r;
}


DWORD File::DeviceIoControlAndWait(int code, LPCVOID bufIn, size_t nIn, LPVOID bufOut, size_t nOut) {
	DWORD dw;
	COvlEvent ovl;
	if (!DeviceIoControl(code, bufIn, nIn, bufOut, nOut, &dw, &ovl))
		dw = GetOverlappedResult(ovl);
	return dw;
}
#endif


//!!!#endif // UCFG_WCE


#ifdef _AFXDLL
	AFX_MODULE_STATE _afxBaseModuleState(true, &AfxWndProcBase);
#else
	AFX_MODULE_STATE _afxBaseModuleState(true); //!!!
#endif

_AFX_THREAD_STATE * AFXAPI AfxGetThreadState() {
	return &ThreadBase::get_CurrentThread()->AfxThreadState();
}

AFX_MODULE_THREAD_STATE::AFX_MODULE_THREAD_STATE()
	:	m_pfnNewHandler(0)
{
}

AFX_MODULE_THREAD_STATE::~AFX_MODULE_THREAD_STATE() {
}

CThreadHandleMaps& AFX_MODULE_THREAD_STATE::GetHandleMaps() {
	if (!m_handleMaps.get())
		m_handleMaps.reset(new CThreadHandleMaps);
	return *m_handleMaps.get();
}

AFX_MODULE_THREAD_STATE* AFXAPI AfxGetModuleThreadState() {
	AFX_MODULE_STATE *ms = AfxGetModuleState();
	AFX_MODULE_THREAD_STATE *r = ms->m_thread;
	if (!r)
		ms->m_thread.reset(r = new AFX_MODULE_THREAD_STATE);
	return r;
}

void AFX_MODULE_STATE::SetHInstance(HMODULE hModule) {
	m_hCurrentInstanceHandle = m_hCurrentResourceHandle = hModule;
#if UCFG_CRASH_DUMP
	if (CCrashDump::I)
		CCrashDump::I->Modules.insert(hModule);
#endif
}

#ifdef _AFXDLL

AFX_MODULE_STATE::AFX_MODULE_STATE(bool bDLL, WNDPROC pfnAfxWndProc)
	:	m_pfnAfxWndProc(pfnAfxWndProc)
	,	m_dwVersion(_MFC_VER),
#else
AFX_MODULE_STATE::AFX_MODULE_STATE(bool bDLL)
	:	
#endif
	m_bDLL(bDLL)
	,	m_fRegisteredClasses(0)
	,	m_pfnFilterToolTipMessage(0)
{
}

AFX_MODULE_STATE::~AFX_MODULE_STATE() {
}

path AFX_MODULE_STATE::get_FileName() {
	TCHAR szModule[_MAX_PATH];
	Win32Check(::GetModuleFileName(m_hCurrentInstanceHandle, szModule, _MAX_PATH));
	return szModule;
}

_AFX_THREAD_STATE::_AFX_THREAD_STATE()
	:	m_bDlgCreate(false)
	,	m_hLockoutNotifyWindow(0)
	,	m_nLastHit(0)
	,	m_nLastStatus(0)
	,	m_bInMsgFilter(false)
	,	m_pLastInfo(0)
{
}

_AFX_THREAD_STATE::~_AFX_THREAD_STATE() {
#if UCFG_EXTENDED && UCFG_GUI
	delete m_pToolTip.get();
#endif
	delete (TOOLINFO*)m_pLastInfo;
}

bool AFXAPI AfxIsValidAddress(const void* lp, UINT nBytes, bool bReadWrite) {
	return true;
}

CWinApp * AFXAPI AfxGetApp() {
	return AfxGetModuleState()->m_pCurrentWinApp;
}

CWinApp::CWinApp(RCString lpszAppName)
	:	m_nWaitCursorCount(0)
	,	m_hcurWaitCursorRestore(0)
{
	m_appName = lpszAppName;
	_AFX_CMDTARGET_GETSTATE()->m_pCurrentWinApp = this;
}

CWinApp::~CWinApp() {
	AFX_MODULE_STATE* pModuleState = _AFX_CMDTARGET_GETSTATE();
	if (pModuleState->m_currentAppName == m_appName)
		pModuleState->m_currentAppName = "";
	if (pModuleState->m_pCurrentWinApp == this)
		pModuleState->m_pCurrentWinApp = 0;
}

void AFXAPI AfxWinInit(HINSTANCE hInstance, HINSTANCE hPrevInstance, RCString lpCmdLine, int nCmdShow) {
	AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
	pModuleState->SetHInstance(hInstance);
	// fill in the initial state for the application
	if (CAppBase *pApp = AfxGetCApp()) {
		//!!!    pApp->InitCOM();
		// Windows specific initialization (not done if no CWinApp)
#if UCFG_COMPLEX_WINAPP
		pApp->m_hInstance = hInstance;
		pApp->m_cmdLine = lpCmdLine;
		pApp->m_nCmdShow = nCmdShow;
#	if UCFG_EXTENDED
		pApp->SetCurrentHandles();
#	endif
#endif
	}
}

BOOL CWinApp::InitInstance() {
	return TRUE;
}

//#if UCFG_GUI

DialogResult MessageBox::Show(RCString text) {
#if UCFG_COMPLEX_WINAPP && UCFG_EXTENDED
	return (DialogResult)AfxMessageBox(text);
#else
	return (DialogResult)::MessageBox(0, text, _T("Message"), MB_OK); //!!!
#endif
}

DialogResult MessageBox::Show(RCString text, RCString caption, int buttons, MessageBoxIcon icon) {
	return (DialogResult)::MessageBox(0, text, caption, buttons | int(icon));
}

//#endif // UCFG_GUI

typedef WINBASEAPI BOOL (WINAPI *PFN_Wow64DisableWow64FsRedirection)(PVOID *OldValue);
typedef WINBASEAPI BOOL (WINAPI *PFN_Wow64RevertWow64FsRedirection)(PVOID OldValue);

void Wow64FsRedirectionKeeper::Disable() {
	DlProcWrap<PFN_Wow64DisableWow64FsRedirection> pfn("KERNEL32.DLL", "Wow64DisableWow64FsRedirection");
	if (pfn) {
		if (Win32Check(pfn(&m_oldValue), ERROR_INVALID_FUNCTION))
			m_bDisabled = true;
	}
}

Wow64FsRedirectionKeeper::~Wow64FsRedirectionKeeper() {
	if (m_bDisabled) {
		DlProcWrap<PFN_Wow64RevertWow64FsRedirection> pfn("KERNEL32.DLL", "Wow64RevertWow64FsRedirection");
		if (pfn)
			Win32Check(pfn(m_oldValue));
	}
}

CVirtualMemory::CVirtualMemory(void *lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect)
	:	m_address(0)
{
	Allocate(lpAddress, dwSize, flAllocationType, flProtect);
}

CVirtualMemory::~CVirtualMemory() {
	Free();
}

void CVirtualMemory::Allocate(void *lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect) {
	if (m_address)
		Throw(E_EXT_NonEmptyPointer);
	Win32Check((m_address = ::VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect)) != 0);
}

void CVirtualMemory::Commit(void *lpAddress, DWORD dwSize, DWORD flProtect) {
	Win32Check(::VirtualAlloc(lpAddress, dwSize, MEM_COMMIT, flProtect) != 0);
}

void CVirtualMemory::Free() {
	if (m_address) {
		Win32Check(::VirtualFree(m_address, 0, MEM_RELEASE));
		m_address = 0;
	}
}

CHeap::CHeap() {
	Win32Check((m_h = ::HeapCreate(0, 0, 0))!=0);
}

CHeap::~CHeap() {
	try {
		Destroy();
	} catch (RCExc) {
	}
}

void CHeap::Destroy() {
	if (m_h)
		Win32Check(::HeapDestroy(exchange(m_h, (HANDLE)0)));
}

size_t CHeap::Size(void *p, DWORD flags) {
	size_t r = ::HeapSize(m_h, flags, p);
	if (r == (size_t)-1)
		Throw(E_FAIL);
	return r;
}

void *CHeap::Alloc(size_t size, DWORD flags) {
	void *r = ::HeapAlloc(m_h, flags, size);
	if (!r)
		Throw(E_OUTOFMEMORY);
	return r;
}

void CHeap::Free(void *p, DWORD flags) {
	Win32Check(::HeapFree(m_h, flags, p));
}

bool AFXAPI AfxOleCanExitApp() {
#if UCFG_COM_IMPLOBJ
	return !AfxGetModuleState()->m_comModule.GetLockCount();
#else
	return true;
#endif
}

IMPLEMENT_DYNAMIC(CCmdTarget, Object)

CCmdTarget::CCmdTarget() {
	m_pModuleState = AfxGetModuleState();
	ASSERT(m_pModuleState != NULL);
}

CCmdTarget::~CCmdTarget() {
}

BOOL CCmdTarget::InitInstance() {
	return FALSE;
}

const AFX_MSGMAP* CCmdTarget::GetThisMessageMap() {
	static const AFX_MSGMAP_ENTRY _messageEntries[] =
	{
		{ 0, 0, AfxSig_end, 0 }     // nothing here
	};
	static const AFX_MSGMAP messageMap =
	{
		NULL,
		&_messageEntries[0]
	};
	return &messageMap;	
}

const AFX_MSGMAP* CCmdTarget::GetMessageMap() const {
	return GetThisMessageMap();
}

CodepageCvt::result CodepageCvt::do_out(mbstate_t& state, const wchar_t *_First1, const wchar_t *_Last1, const wchar_t *& _Mid1, char *_First2, char *_Last2, char *& _Mid2) const {
	int n = ::WideCharToMultiByte(m_cp, 0, _First1, int(_Last1-_First1), _First2, int(_Last2-_First2), 0, 0);
	if (n > 0) {
		_Mid1 = _Last1;
		_Mid2 = _First2+n;
		return ok;
	}
	return error;
}


HRESULT CDllServer::OnRegister() {
#if UCFG_COM_IMPLOBJ
	return COleObjectFactory::UpdateRegistryAll();
#else
	return S_OK;
#endif
}

HRESULT CDllServer::OnUnregister() {
#if UCFG_COM_IMPLOBJ
	return COleObjectFactory::UpdateRegistryAll(false);
#else
	return S_OK;
#endif
}

#if UCFG_COMPLEX_WINAPP	
CWinThread * AFXAPI AfxGetThread() {
	ThreadBase *t = Thread::TryGetCurrentThread();
	CWinThread *pThread = t ? t->AsWinThread() : 0;
	return pThread ? pThread : AfxGetApp();
}
#endif


int AFXAPI AfxMessageBox(RCString text, UINT nType, UINT nIDHelp) {
#if UCFG_WND
	if (CWinApp *pApp = AfxGetApp())
		return pApp->DoMessageBox(text, nType, nIDHelp);
	else
		return pApp->CWinApp::DoMessageBox(text, nType, nIDHelp);
#else
	return ::MessageBox(0, text, _T("Message Box"), nType);
#endif
}

int AFXAPI AfxMessageBox(UINT nIDPrompt, UINT nType, UINT nIDHelp) {
	String string;
	string.Load(nIDPrompt);
	if (nIDHelp == (UINT)-1)
		nIDHelp = nIDPrompt;
	return AfxMessageBox(string, nType, nIDHelp);
}


void AfxWinTerm() {
	//!!!  ::CoFreeUnusedLibraries();
	_AFX_THREAD_STATE *pTS = AfxGetThreadState();
	if (!AfxGetModuleState()->m_bDLL) {
#if !UCFG_WCE && UCFG_EXTENDED
		pTS->m_hookCbt.Unhook();
		pTS->m_hookMsg.Unhook();
#endif
	}
}

static int AfxWinMainEx(HINSTANCE hInstance, HINSTANCE hPrevInstance, RCString lpCmdLine, int nCmdShow) {
	AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
	pModuleState->m_bDLL = false;
	int nReturnCode = 1;
	try {
		AfxWinInit(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
		CWinThread* pThread = AfxGetThread(); 
		bool rc = pThread->InitInstance();
#if UCFG_WIN_MSG
		if (rc)
			nReturnCode = pThread->RunLoop();
		else {
			if (pThread->m_pMainWnd) {
				TRC(0, "Warning: Destroying non-NULL m_pMainWnd");
				pThread->m_pMainWnd->Destroy();
			}
			nReturnCode = pThread->ExitInstance();
		}
#else
		nReturnCode = pThread->ExitInstance();
#endif
	} catch (RCExc ex) {
		nReturnCode = HResultInCatch(ex);
		if (!GetSilentUI())
			AfxMessageBox(ex.what(), MB_ICONSTOP | MB_OK);
	}
	return nReturnCode;
}

static int AbortFilter() {
#if UCFG_COMPLEX_WINAPP			//!!!?
	AfxGetApp()->OnAbort();
#endif
	return EXCEPTION_CONTINUE_SEARCH;
}

int AFXAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, RCString lpCmdLine, int nCmdShow) {
	int nReturnCode = 0;
	__try {
		nReturnCode = AfxWinMainEx(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	} __except(AbortFilter()) {
	}
	AfxWinTerm();
	return nReturnCode;
}


static EXT_THREAD_PTR(AFX_MODULE_STATE) t_pModuleState;
static EXT_THREAD_PTR(AFX_MODULE_STATE) t_pPrevModuleState;

AFX_MODULE_STATE* AFXAPI AfxGetModuleState() {
	AFX_MODULE_STATE *r;
	if (!(r = t_pModuleState))
		r = &_afxBaseModuleState;
	return r;
}

void AFXAPI AfxSetModuleState(AFX_MODULE_STATE *pNewState) {
	t_pPrevModuleState = t_pModuleState;						// std::exchange() does not work with EXT_THREAD_PTR
	t_pModuleState = pNewState;
}

void AFXAPI AfxRestoreModuleState() {
	t_pModuleState = t_pPrevModuleState;
	t_pPrevModuleState = nullptr;
}

AFX_MAINTAIN_STATE2::AFX_MAINTAIN_STATE2(AFX_MODULE_STATE* pNewState)
	:	m_pPrevModuleState(t_pModuleState)
{
	t_pModuleState = pNewState;
}

AFX_MAINTAIN_STATE2::~AFX_MAINTAIN_STATE2() {
	t_pModuleState = m_pPrevModuleState;
}

struct Win32CodeErrc {
	uint16_t Code;
	errc Errc;
};

static const Win32CodeErrc s_win32code2errc[] ={
	ERROR_FILE_NOT_FOUND,		errc::no_such_file_or_directory,
	ERROR_PATH_NOT_FOUND,		errc::no_such_file_or_directory,
	ERROR_ACCESS_DENIED,		errc::permission_denied,
	ERROR_INVALID_HANDLE,		errc::bad_file_descriptor,
	ERROR_OUTOFMEMORY,			errc::not_enough_memory,
	ERROR_NOT_ENOUGH_MEMORY,	errc::not_enough_memory,
	ERROR_NOT_SUPPORTED,		errc::not_supported,
	ERROR_INVALID_PARAMETER,	errc::invalid_argument,
	ERROR_BROKEN_PIPE,			errc::broken_pipe,
	ERROR_ALREADY_EXISTS,		errc::file_exists,
	ERROR_FILENAME_EXCED_RANGE, errc::filename_too_long,
	ERROR_FILE_TOO_LARGE,		errc::file_too_large,
	ERROR_CANCELLED,			errc::operation_canceled,
	ERROR_WAIT_NO_CHILDREN,		errc::no_child_process,
	ERROR_ARITHMETIC_OVERFLOW,	errc::result_out_of_range,
	ERROR_BUSY,					errc::device_or_resource_busy,
	ERROR_DEVICE_IN_USE,		errc::device_or_resource_busy,
	ERROR_BAD_FORMAT,			errc::executable_format_error,
	ERROR_DIR_NOT_EMPTY,		errc::directory_not_empty,
	ERROR_DISK_FULL,			errc::no_space_on_device,
	ERROR_INVALID_ADDRESS,		errc::bad_address,
	ERROR_TIMEOUT,				errc::timed_out,
	ERROR_IO_PENDING,			errc::resource_unavailable_try_again,
	ERROR_NOT_SAME_DEVICE,		errc::cross_device_link,
	ERROR_WRITE_PROTECT,		errc::read_only_file_system,
	ERROR_POSSIBLE_DEADLOCK,	errc::resource_deadlock_would_occur,
	ERROR_PRIVILEGE_NOT_HELD,	errc::operation_not_permitted,
	ERROR_INTERNET_CANNOT_CONNECT, errc::connection_refused,
	WSAENOBUFS, 				errc::no_buffer_space,
	WSAEINTR,					errc::interrupted,
	WSAEBADF,					errc::bad_file_descriptor,
	WSAEACCES,					errc::permission_denied,
	WSAEFAULT,					errc::bad_address,
	WSAEINVAL,					errc::invalid_argument,
	WSAEMFILE,					errc::too_many_files_open,
	WSAEWOULDBLOCK,				errc::operation_would_block,
	WSAEINPROGRESS,				errc::operation_in_progress,
	WSAEALREADY,				errc::connection_already_in_progress,
	WSAENOTSOCK,				errc::not_a_socket,
	WSAEDESTADDRREQ,			errc::destination_address_required,
	WSAEMSGSIZE,				errc::message_size,
	WSAEPROTOTYPE,				errc::wrong_protocol_type,
	WSAENOPROTOOPT,				errc::no_protocol_option,
	WSAEPROTONOSUPPORT,			errc::protocol_not_supported,
	WSAEOPNOTSUPP,				errc::protocol_not_supported,
	WSAEAFNOSUPPORT,			errc::address_family_not_supported,
	WSAEADDRINUSE,				errc::address_in_use,
	WSAEADDRNOTAVAIL,			errc::address_not_available,
	WSAENETDOWN,				errc::network_down,
	WSAENETUNREACH,				errc::network_unreachable,
	WSAENETRESET,				errc::network_reset,
	WSAECONNABORTED,			errc::connection_aborted,
	WSAECONNRESET,				errc::connection_reset,
	WSAEISCONN,					errc::already_connected,
	WSAENOTCONN,				errc::not_connected,
	WSAETIMEDOUT,				errc::timed_out,
	WSAECONNREFUSED,			errc::connection_refused,
	WSAELOOP,					errc::too_many_symbolic_link_levels,
	WSAENAMETOOLONG,			errc::filename_too_long,
	WSAEHOSTUNREACH,			errc::host_unreachable,
	WSAENOTEMPTY,				errc::directory_not_empty,
	WSAECANCELLED,				errc::operation_canceled,
	0
};

static class Win32Category : public error_category {			// outside function to eliminate thread-safe static machinery
	typedef error_category base;

	const char *name() const noexcept override { return "Win32"; }

	string message(int eval) const override {
#if !UCFG_WDM
		TCHAR buf[256];

		if (eval >= INTERNET_ERROR_BASE && eval <= INTERNET_ERROR_LAST) {
			if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, LPCVOID(GetModuleHandle(_T("wininet.dll"))),
				eval, 0, buf, sizeof buf, 0))
				return "WinInet: " + String(buf);
		} else if (eval >= WSABASEERR && eval<WSABASEERR + 1024) {
			if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
				LPCVOID(_afxBaseModuleState.m_hCurrentResourceHandle),
				eval, 0, buf, sizeof buf, 0))
				return String(buf);
		}
		if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, eval, 0, buf, sizeof buf, 0))
			return String(buf);
#endif
		return Convert::ToString(eval, 16);
	}

	error_condition default_error_condition(int errval) const noexcept override {
		int code;
		for (const Win32CodeErrc *p=s_win32code2errc; (code=p->Code); ++p)
			if (code == errval)
				return p->Errc;
		return error_condition(errval, *this);
	}

	bool equivalent(int errval, const error_condition& c) const noexcept override {			//!!!TODO
		switch (errval) {
		case ERROR_TOO_MANY_OPEN_FILES:	return c==errc::too_many_files_open || c==errc::too_many_files_open_in_system;
		default:
			return base::equivalent(errval, c);
		}
	}

	bool equivalent(const error_code& ec, int errval) const noexcept override {
		if (ec.category()==hresult_category() && HRESULT_FACILITY(ec.value()) == FACILITY_WIN32)
			return errval == (ec.value() & 0xFFFF);
		else
			return base::equivalent(ec, errval);
	}

} s_win32Category;

const error_category& win32_category() {
	return s_win32Category;
}


} // Ext::


#if defined(_EXT) || !defined(_AFXDLL)
	extern "C" int _charmax = CHAR_MAX;
#endif

#if UCFG_WCE
#	undef DEFINE_GUID
#	define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const CLSID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DECLSPEC_SELECTANY  __declspec(selectany)

extern "C" {
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_OLEGUID(IID_IUnknown,            0x00000000L, 0, 0);
DEFINE_OLEGUID(IID_IClassFactory,       0x00000001L, 0, 0);
DEFINE_GUID(CLSID_XMLHTTPRequest, 0xED8C108E, 0x4349, 0x11D2, 0x91, 0xA4, 0x00, 0xC0, 0x4F, 0x79, 0x69, 0xE8);
} // "C"

#endif // UCFG_WCE

#if UCFG_EXTENDED && !UCFG_WCE

namespace Ext {

CWindowsHook::CWindowsHook()
	:	m_handle(0)
{}

CWindowsHook::~CWindowsHook() {
	Unhook();
}

void CWindowsHook::Set(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId) {
	m_handle = SetWindowsHookEx(idHook, lpfn, hMod, dwThreadId);
	Win32Check(m_handle != 0);
}

void CWindowsHook::Unhook() {
	if (m_handle)
		::UnhookWindowsHookEx(exchange(m_handle, (HHOOK)0));
	//!!!    Win32Check(::UnhookWindowsHookEx(m_handle));
}

LRESULT CWindowsHook::CallNext(int nCode, WPARAM wParam, LPARAM lParam) {
	return CallNextHookEx(m_handle, nCode, wParam, lParam);
}

} // Ext::

#endif



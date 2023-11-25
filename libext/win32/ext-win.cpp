/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

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


Ext::CCrashDumpBase *Ext::CCrashDumpBase::I;


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


ProcessModule::ProcessModule(class Process& process, HMODULE hModule)
	: Process(process)
	, HModule(hModule)
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


#endif // !UCFG_WCE

void COperatingSystem::MessageBeep(UINT uType) {
	Win32Check(::MessageBeep(uType));
}


//!!!#if !UCFG_WCE && UCFG_EXTENDED

#if !UCFG_WCE



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

#if UCFG_THREAD_MANAGEMENT
_AFX_THREAD_STATE * AFXAPI AfxGetThreadState() {
	return &ThreadBase::get_CurrentThread()->AfxThreadState();
}
#endif

AFX_MODULE_THREAD_STATE::AFX_MODULE_THREAD_STATE()
	: m_pfnNewHandler(0)
{
}

AFX_MODULE_THREAD_STATE::~AFX_MODULE_THREAD_STATE() {
}

CThreadHandleMaps& AFX_MODULE_THREAD_STATE::GetHandleMaps() {
	if (!m_handleMaps.get())
		m_handleMaps.reset(new CThreadHandleMaps);
	return *m_handleMaps.get();
}

#if UCFG_THREAD_MANAGEMENT
AFX_MODULE_THREAD_STATE* AFXAPI AfxGetModuleThreadState() {
	AFX_MODULE_STATE *ms = AfxGetModuleState();
	AFX_MODULE_THREAD_STATE *r = ms->m_thread;
	if (!r)
		ms->m_thread.reset(r = new AFX_MODULE_THREAD_STATE);
	return r;
}
#endif

void AFX_MODULE_STATE::SetHInstance(HMODULE hModule) {
	m_hCurrentInstanceHandle = m_hCurrentResourceHandle = hModule;
#if UCFG_CRASH_DUMP
	if (CCrashDumpBase::I)
		CCrashDumpBase::I->Modules.insert(hModule);
#endif
}

#ifdef _AFXDLL

AFX_MODULE_STATE::AFX_MODULE_STATE(bool bDLL, WNDPROC pfnAfxWndProc)
	: m_pfnAfxWndProc(pfnAfxWndProc)
	, m_dwVersion(_MFC_VER),
#else
AFX_MODULE_STATE::AFX_MODULE_STATE(bool bDLL)
	:
#endif
	m_bDLL(bDLL)
	, m_fRegisteredClasses(0)
	, m_pfnFilterToolTipMessage(0)
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
	: m_bDlgCreate(false)
	, m_hLockoutNotifyWindow(0)
	, m_nLastHit(0)
	, m_nLastStatus(0)
	, m_bInMsgFilter(false)
	, m_pLastInfo(0)
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
	: m_nWaitCursorCount(0)
	, m_hcurWaitCursorRestore(0)
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
	: m_address(0)
{
	Allocate(lpAddress, dwSize, flAllocationType, flProtect);
}

CVirtualMemory::~CVirtualMemory() {
	Free();
}

void CVirtualMemory::Allocate(void *lpAddress, DWORD dwSize, DWORD flAllocationType, DWORD flProtect) {
	if (m_address)
		Throw(ExtErr::NonEmptyPointer);
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
		{ 0, 0, 0, 0 }     // nothing here		AfxSig_end = 0
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
		return _Mid1==_Last1 ? ok : partial;
	}
	return error;
}

/*!!!R implemented in codecvt_utf8_utf16
CodepageCvt::result CodepageCvt::do_in(mbstate_t& s, const char *fb, const char *fe, const char *&fn, wchar_t *tb, wchar_t *te, wchar_t *&tn) const {
	if (m_cp == CP_UTF8) {
		tn = tb;
		for (fn=fb; fn!=fe;) {
			if (tn == te)
				return partial;
			if (!s._Byte) {
				byte ch = byte(*fn);
				if (ch < 0x80)
					s._Wchar = ch;
				else if (ch < 0xC0)
					return error;
				else if (ch < 0xE0) {
					s._Wchar = ch & 0x1F;
					s._Byte = 1;
				} else if (ch < 0xF0) {
					s._Wchar = ch & 0xF;
					s._Byte = 2;
				} else if (ch < 0xF8) {
					s._Wchar = ch & 7;
					s._Byte = 3;
				} else if (ch < 0xFC) {
					s._Wchar = ch & 3;
					s._Byte = 4;
				} else {
					s._Wchar = ch & 3;
					s._Byte = 5;
				}
				++fn;
			}

			for (; s._Byte; ++fn) {
				if (fn == fe)
					return partial;
				char ch = *fn;
				if (!(ch & 0x80) || (ch & 0x40))
					return error;
				s._Wchar = (s._Wchar << 6) | (ch & 0x3F);
				s._Byte--;
			}
			*tn++ = exchange(s._Wchar, 0);
		}
		return s._Byte ? partial : ok;
	} else {
		Throw(E_NOTIMPL);
	}
}*/

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

#if UCFG_COM    //!!!?
int AFXAPI AfxMessageBox(UINT nIDPrompt, UINT nType, UINT nIDHelp) {
	String string;
	string.Load(nIDPrompt);
	if (nIDHelp == (UINT)-1)
		nIDHelp = nIDPrompt;
	return AfxMessageBox(string, nType, nIDHelp);
}
#endif

void AfxWinTerm() {
	//!!!  ::CoFreeUnusedLibraries();
	if (!AfxGetModuleState()->m_bDLL) {
#if !UCFG_WCE && UCFG_EXTENDED
		_AFX_THREAD_STATE* pTS = AfxGetThreadState();
		pTS->m_hookCbt.Unhook();
		pTS->m_hookMsg.Unhook();
#endif
	}
}

#if UCFG_EXTENDED && UCFG_WIN_MSG
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

#endif // UCFG_EXTENDED && UCFG_WIN_MSG



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
	: m_pPrevModuleState(t_pModuleState)
{
	t_pModuleState = pNewState;
}

AFX_MAINTAIN_STATE2::~AFX_MAINTAIN_STATE2() {
	t_pModuleState = m_pPrevModuleState;
}



bool AFXAPI AfxHasResource(const CResID& name, const CResID& typ) {
	if (FindResource(AfxGetResourceHandle(), name, typ))
		return true;
#ifdef _AFXDLL
	AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
	for (AFX_MODULE_STATE::CLibraryList::iterator i(pModuleState->m_libraryList.begin()); i!=pModuleState->m_libraryList.end(); ++i)
		if (FindResource((*i)->m_hModule, name, typ))
			return true;
#endif
	return false;
}

} // Ext::


#if !UCFG_STDSTL && (defined(_EXT) || !defined(_AFXDLL))
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
	: m_handle(0)
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

namespace Ext::Win {

AcceleratorTable& AcceleratorTable::operator=(const AcceleratorTable& other) {
	this->~AcceleratorTable();
	_hAccel = 0;
	if (other._hAccel) {
		auto n = ::CopyAcceleratorTable(other._hAccel, 0, 0);
		auto accels = (ACCEL*)alloca(n * sizeof(ACCEL));
		::CopyAcceleratorTable(other._hAccel, accels, n);
		Win32Check((_hAccel = ::CreateAcceleratorTable(accels, n)) != 0);
	}
	return *this;
}

CGlobalAlloc::CGlobalAlloc(size_t size, DWORD flags)
	: m_handle(::GlobalAlloc(flags, size)) {
	Win32Check(m_handle != 0);
}

CGlobalAlloc::CGlobalAlloc(const Blob& blob)
	: m_handle(::GlobalAlloc(GMEM_FIXED, blob.size())) {
	Win32Check(m_handle != 0);
	memcpy(::GlobalLock(m_handle), blob.constData(), blob.size());
	GlobalUnlock(m_handle);
}

CGlobalAlloc::~CGlobalAlloc() {
	if (m_handle)
		Win32Check(::GlobalFree(m_handle) == 0);
}

HGLOBAL CGlobalAlloc::Detach() {
	return exchange(m_handle, HGLOBAL(0));
}

GlobalLocker::GlobalLocker(HGLOBAL hGlobal) {
	_ptr = ::GlobalLock(_hGlobal = hGlobal);
	Win32Check(_ptr != 0);
}

GlobalLocker::~GlobalLocker() {
	Win32Check(::GlobalUnlock(_hGlobal), NO_ERROR);
}


} // Ext::Win::

#include <el/ext.h>

#include EXT_HEADER_CODECVT

#include <windows.h>

#if UCFG_EXTENDED  
#	include <afxres.h>
#endif


#include <el/gui/menu.h>

#include <el/libext/win32/extmfc.h>
#include <el/libext/win32/ext-full-win.h>


namespace Ext {
using namespace std;

//#include "mfcimpl.h"


path AFXAPI ToShortPath(const path& p) {
	TCHAR buf[_MAX_PATH];
	DWORD dw = GetShortPathName(p.native(), buf, size(buf));
	Win32Check(dw < size(buf) && dw);
	return buf;
}


CSemaphore::CSemaphore(RCString name, DWORD dwAccess, bool bInherit)
	:	CSyncObject(name)
{
	Attach((intptr_t)::OpenSemaphore(dwAccess, bInherit, name));
}



bool String::Load(UINT nID, WORD wLanguage)
{
	HINSTANCE hInst = AfxGetResourceHandle();
	HRSRC hSrc = FindResourceEx(hInst, RT_STRING, MAKEINTRESOURCE(nID/16+1), wLanguage);
	if (!hSrc)
		return false;
	HGLOBAL hTemplate = LoadResource(hInst, hSrc);
	WORD *p = (WORD*)hTemplate;
	for (int i=0; i<nID%16; i++, p++)
		if (*p)
			p += *p;
	if (!*p)
		return false;
	int len = *p++;
	_self = String((wchar_t*)p, len);
	return true;
}

String String::FromGlobalAtom(ATOM a) {
	TCHAR *p;
	for (int n=100, len=n;; len<<=1) {
		p = new TCHAR[len];
		if (::GlobalGetAtomName(a, p, len))
			break;
		Win32Check(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		delete[] p;
	}
	return p;
}

void Console::Beep(DWORD dwFreq, DWORD dwDuration) {
	Win32Check(::Beep(dwFreq, dwDuration));
}

CONSOLE_SCREEN_BUFFER_INFO COutputConsole::get_ScreenBufferInfo() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	Win32Check(::GetConsoleScreenBufferInfo((HANDLE)(intptr_t)HandleAccess(_self), &csbi));
	return csbi;
}

void COutputConsole::SetCursorPosition(COORD coord) {
	Win32Check(::SetConsoleCursorPosition((HANDLE)(intptr_t)HandleAccess(_self), coord));
}


void Process::GenerateConsoleCtrlEvent(DWORD dwCtrlEvent) {
	Win32Check(::GenerateConsoleCtrlEvent(dwCtrlEvent, ID));
}

static const TCHAR _afxOldWndProc[] = _T("AfxOldWndProcExt");

static void _AfxPreInitDialog(CWnd* pWnd, LPRECT lpRectOld, DWORD* pdwStyleOld) {
	ASSERT(lpRectOld != NULL);
	ASSERT(pdwStyleOld != NULL);
	*lpRectOld = pWnd->WindowRect;
	*pdwStyleOld = pWnd->Style;
}

static void _AfxPostInitDialog(CWnd* pWnd, const RECT& rectOld, DWORD dwStyleOld) {
	// must be hidden to start with
	if (dwStyleOld & WS_VISIBLE)
		return;

	// must not be visible after WM_INITDIALOG
	if (pWnd->Style & (WS_VISIBLE|WS_CHILD))
		return;

	// must not move during WM_INITDIALOG
	Rectangle rect = pWnd->WindowRect;
	if (rectOld.left != rect.left || rectOld.top != rect.top)
		return;

	// must be unowned or owner disabled
	CWnd* pParent = pWnd->GetWindow(GW_OWNER);
	if (pParent && pParent->IsEnabled)
		return;
	//!!!	if (!pWnd->CheckAutoCenter())
	//		return;
	// center modal dialog boxes/message boxes
	//!!!	pWnd->CenterWindow();
}

static void _AfxHandleActivate(CWnd* pWnd, WPARAM nState, CWnd* pWndOther)
{
	ASSERT(pWnd != NULL);
	// send WM_ACTIVATETOPLEVEL when top-level parents change
	CWnd* pTopLevel;
	if (!(pWnd->Style & WS_CHILD) &&
		(pTopLevel = pWnd->GetTopLevelParent()) != pWndOther->GetTopLevelParent())
	{
		// lParam points to window getting the WM_ACTIVATE message and
		//  hWndOther from the WM_ACTIVATE.
		HWND hWnd2[2];
		hWnd2[0] = pWnd->m_hWnd;
		hWnd2[1] = pWndOther->GetSafeHwnd();
		// send it...
		pTopLevel->SendMessage(WM_ACTIVATETOPLEVEL, nState, (LPARAM)&hWnd2[0]);
	}
}

static BOOL _AfxHandleSetCursor(CWnd* pWnd, UINT nHitTest, UINT nMsg) {
	if (nHitTest == HTERROR &&
		(nMsg == WM_LBUTTONDOWN || nMsg == WM_MBUTTONDOWN ||
		nMsg == WM_RBUTTONDOWN))
	{
		// activate the last active window if not active
		CWnd* pLastActive = pWnd->GetTopLevelParent();
		if (pLastActive != NULL)
			pLastActive = pLastActive->LastActivePopup;
		if (pLastActive && pLastActive != CWnd::Foreground && pLastActive->IsEnabled) {
			CWnd::Foreground = pLastActive;
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CALLBACK _AfxActivationWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam) {
	WNDPROC oldWndProc = (WNDPROC)::GetProp(hWnd, _afxOldWndProc);
	ASSERT(oldWndProc != NULL);

	LRESULT lResult = 0;
	try {
		BOOL bCallDefault = TRUE;
		switch (nMsg) {
		case WM_INITDIALOG:
			{
				DWORD dwStyle;
				Rectangle rectOld;
				CWnd* pWnd = CWnd::FromHandle(hWnd);
				_AfxPreInitDialog(pWnd, &rectOld, &dwStyle);
				bCallDefault = FALSE;
				lResult = CallWindowProc(oldWndProc, hWnd, nMsg, wParam, lParam);
				_AfxPostInitDialog(pWnd, rectOld, dwStyle);
			}
			break;

		case WM_ACTIVATE:
			_AfxHandleActivate(CWnd::FromHandle(hWnd), wParam,
				CWnd::FromHandle((HWND)lParam));
			break;

		case WM_SETCURSOR:
			bCallDefault = !_AfxHandleSetCursor(CWnd::FromHandle(hWnd),
				(short)LOWORD(lParam), HIWORD(lParam));
			break;

#if !UCFG_WCE
		case WM_NCDESTROY:
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
			RemoveProp(hWnd, _afxOldWndProc);
			GlobalDeleteAtom(GlobalFindAtom(_afxOldWndProc));
			break;
#endif
		}

		// call original wndproc for default handling
		if (bCallDefault)
			lResult = CallWindowProc(oldWndProc, hWnd, nMsg, wParam, lParam);
	} catch (RCExc e) {
		// handle exception
		MSG msg;
		msg.hwnd = hWnd;
		msg.message = nMsg;
		msg.wParam = wParam;
		msg.lParam = lParam;
		lResult = AfxGetThread()->ProcessWndProcException(e, &msg);
		TRC(1, "Warning: Uncaught exception in _AfxActivationWndProc (returning " << lResult << ")");
	}

	return lResult;
}

LRESULT CALLBACK _AfxGrayBackgroundWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam) {
	// handle standard gray backgrounds if enabled
	/*!!!	_AFX_WIN_STATE* pWinState = &_afxWinState;
	if (pWinState->m_hDlgBkBrush != NULL &&
	(nMsg == WM_CTLCOLORBTN || nMsg == WM_CTLCOLORDLG ||
	nMsg == WM_CTLCOLORSTATIC || nMsg == WM_CTLCOLORSCROLLBAR ||
	nMsg == WM_CTLCOLORLISTBOX) &&
	CWnd::GrayCtlColor((HDC)wParam, (HWND)lParam,
	(UINT)(nMsg - WM_CTLCOLORMSGBOX),
	pWinState->m_hDlgBkBrush, pWinState->m_crDlgTextClr))
	{
	return (LRESULT)pWinState->m_hDlgBkBrush;
	}*/

	// do standard activation related things as well
	return _AfxActivationWndProc(hWnd, nMsg, wParam, lParam);
}

LRESULT CALLBACK _AfxCbtFilterHook(int code, WPARAM wParam, LPARAM lParam)
{
	_AFX_THREAD_STATE *pThreadState = AfxGetThreadState();
	if (code == HCBT_CREATEWND)
	{
		ASSERT(lParam != NULL);
		LPCREATESTRUCT lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;
		ASSERT(lpcs != NULL);
		CWnd* pWndInit = pThreadState->m_pWndInit;
		BOOL bContextIsDLL = AfxGetModuleState()->m_bDLL;
		if (pWndInit || (!(lpcs->style & WS_CHILD) && !bContextIsDLL))
		{
			// Note: special check to avoid subclassing the IME window
			ASSERT(wParam != NULL); // should be non-NULL HWND
			HWND hWnd = (HWND)wParam;
			WNDPROC oldWndProc;
			if (pWndInit)
			{
#ifdef _AFXDLL
				AFX_MANAGE_STATE(pWndInit->m_pModuleState);
#endif
				// the window should not be in the permanent map at this time
				ASSERT(CWnd::FromHandlePermanent(hWnd) == NULL);
				// connect the HWND to pWndInit...
				pWndInit->Attach(hWnd);
				// allow other subclassing to occur first
				pWndInit->PreSubclassWindow();
				WNDPROC *pOldWndProc = pWndInit->GetSuperWndProcAddr();
				ASSERT(pOldWndProc != NULL);
				// subclass the window with standard AfxWndProc
				WNDPROC afxWndProc = AfxGetAfxWndProc();
				oldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (DWORD_PTR)afxWndProc);
				ASSERT(oldWndProc != NULL);
				if (oldWndProc != afxWndProc)
					*pOldWndProc = oldWndProc;
				pThreadState->m_pWndInit = 0;
			}
			else
			{
				ASSERT(!bContextIsDLL);   // should never get here
				// subclass the window with the proc which does gray backgrounds
				oldWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
				if (oldWndProc != NULL && GetProp(hWnd, _afxOldWndProc) == NULL)
		  {
			  SetProp(hWnd, _afxOldWndProc, oldWndProc);
			  if ((WNDPROC)GetProp(hWnd, _afxOldWndProc) == oldWndProc)
			  {
				  GlobalAddAtom(_afxOldWndProc);
				  SetWindowLongPtr(hWnd, GWLP_WNDPROC, (DWORD_PTR)(pThreadState->m_bDlgCreate ? _AfxGrayBackgroundWndProc : _AfxActivationWndProc));
				  ASSERT(oldWndProc != NULL);
			  }
		  }
			}
		}
	}
	return pThreadState->m_hookCbt.CallNext(code, wParam, lParam);
}


#if UCFG_GUI

CMenu *CWnd::GetSystemMenu(bool bRevert) const {
	return CMenu::FromHandle(::GetSystemMenu(m_hWnd, bRevert));
}


CWnd *CWnd::GetDescendant(int nID, bool bOnlyPerm) const {
	// GetDlgItem recursive (return first found)
	// breadth-first for 1 level, then depth-first for next level

	// use GetDlgItem since it is a fast USER function
	HWND hWndChild;
	CWnd* pWndChild;
	if ((hWndChild = ::GetDlgItem(m_hWnd, nID)) != NULL)
	{
		if (::GetTopWindow(hWndChild) != NULL)
		{
			// children with the same ID as their parent have priority
      pWndChild = CWnd::FromHandle(hWndChild)->GetDescendant(nID, bOnlyPerm);
			if (pWndChild)
				return pWndChild;
		}
		// return temporary handle if allowed
		if (!bOnlyPerm)
			return CWnd::FromHandle(hWndChild);

		// return only permanent handle
		pWndChild = CWnd::FromHandlePermanent(hWndChild);
		if (pWndChild != NULL)
			return pWndChild;
	}

	// walk each child
	for (hWndChild = ::GetTopWindow(m_hWnd); hWndChild != NULL;
		hWndChild = Ext::GetNextWindow(hWndChild, GW_HWNDNEXT))
	{
      pWndChild = CWnd::FromHandle(hWndChild)->GetDescendant(nID, bOnlyPerm);
		if (pWndChild)
			return pWndChild;
	}
	return NULL;    // not found
}

void CWnd::SendMessageToDescendants(UINT message, WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm)
{
	// walk through HWNDs to avoid creating temporary CWnd objects
	// unless we need to call this function recursively
	for (CWnd *pChild = GetTopWindow(); pChild; pChild = pChild->GetNextWindow())
	{
		// if bOnlyPerm is TRUE, don't send to non-permanent windows
		if (bOnlyPerm)
		{
      MSG msg = {pChild->m_hWnd, message, wParam, lParam};
      AfxCallWndProc(pChild, msg);
		}
		else
			pChild->SendMessage(message, wParam, lParam);
		if (bDeep && pChild->GetTopWindow())
			pChild->SendMessageToDescendants(message, wParam, lParam, bDeep, bOnlyPerm);
	}
}

bool CWnd::ExecuteDlgInit(const CResID& resID) {
  // find resource handle
  LPVOID lpResource = 0;
  if (LPCTSTR(resID))
    if (AfxHasResource(resID, RT_DLGINIT))
      lpResource = (void*)Resource(resID, RT_DLGINIT).get_Data();
  return ExecuteDlgInit(lpResource);
}

bool CWnd::ExecuteDlgInit(void *lpResource)
{
	BOOL bSuccess = TRUE;
	if (lpResource)
	{
		UNALIGNED WORD* lpnRes = (WORD*)lpResource;
		while (bSuccess && *lpnRes != 0)
		{
			WORD nIDC = *lpnRes++;
			WORD nMsg = *lpnRes++;
			DWORD dwLen = *((UNALIGNED DWORD*&)lpnRes)++;

			// In Win32 the WM_ messages have changed.  They have
			// to be translated from the 32-bit values to 16-bit
			// values here.

			#define WIN16_LB_ADDSTRING  0x0401
			#define WIN16_CB_ADDSTRING  0x0403
			#define AFX_CB_ADDSTRING    0x1234

			// unfortunately, WIN16_CB_ADDSTRING == CBEM_INSERTITEM
			if (nMsg == AFX_CB_ADDSTRING)
				nMsg = CBEM_INSERTITEM;
			else if (nMsg == WIN16_LB_ADDSTRING)
				nMsg = LB_ADDSTRING;
			else if (nMsg == WIN16_CB_ADDSTRING)
				nMsg = CB_ADDSTRING;

			// check for invalid/unknown message types
			ASSERT(nMsg == LB_ADDSTRING || nMsg == CB_ADDSTRING ||
				nMsg == CBEM_INSERTITEM ||
				nMsg == WM_OCC_LOADFROMSTREAM ||
				nMsg == WM_OCC_LOADFROMSTREAM_EX ||
				nMsg == WM_OCC_LOADFROMSTORAGE ||
				nMsg == WM_OCC_LOADFROMSTORAGE_EX ||
				nMsg == WM_OCC_INITNEW);

#ifdef _DEBUG
			// For AddStrings, the count must exactly delimit the
			// string, including the NULL termination.  This check
			// will not catch all mal-formed ADDSTRINGs, but will
			// catch some.
			if (nMsg == LB_ADDSTRING || nMsg == CB_ADDSTRING || nMsg == CBEM_INSERTITEM)
				ASSERT(*((LPBYTE)lpnRes + (UINT)dwLen - 1) == 0);
#endif

			if (nMsg == CBEM_INSERTITEM)
			{
//!!!				USES_CONVERSION;
				COMBOBOXEXITEM item;
				item.mask = CBEIF_TEXT;
				item.iItem = -1;
//!!!				item.pszText = A2T(LPSTR(lpnRes));
				item.pszText = (TCHAR*)lpnRes;
				if (::SendDlgItemMessage(m_hWnd, nIDC, nMsg, 0, (LPARAM) &item) == -1)
					bSuccess = FALSE;
			}
			else if (nMsg == LB_ADDSTRING || nMsg == CB_ADDSTRING)
			{
				// List/Combobox returns -1 for error
				if (::SendDlgItemMessageA(m_hWnd, nIDC, nMsg, 0, (LPARAM) lpnRes) == -1)
					bSuccess = FALSE;
			}
			// skip past data
			lpnRes = (WORD*)((LPBYTE)lpnRes + (UINT)dwLen);
		}
	}
	// send update message to all controls after all other siblings loaded
	if (bSuccess)
		SendMessageToDescendants(WM_INITIALUPDATE, 0, 0, FALSE, FALSE);
	return bSuccess;
}

#endif

#if UCFG_COUT_REDIRECTOR=='U'

extern "C" {
	int __cdecl _free_osfhnd(int const fh);
#	if _VC_CRT_MAJOR_VERSION >= 14
		int __cdecl __acrt_lowio_set_os_handle(int const fh, intptr_t const value);
#	else
		int __cdecl _set_osfhnd(int const fh, intptr_t const value);
#		define __acrt_lowio_set_os_handle _set_osfhnd
#	endif
} // "C"

class PipeConvertor : public Thread {
	typedef Thread base;
public:
	HANDLE HConsole, HRead, HWrite;
	DWORD m_nStdHandle;
	int m_fd;

	PipeConvertor(FILE *pFile)
		: m_nStdHandle(STD_OUTPUT_HANDLE)
		, m_fd(fileno(pFile))
		, m_state()
	{
		if (pFile == stderr)
			m_nStdHandle = STD_ERROR_HANDLE;

		HConsole = GetStdHandle(m_nStdHandle);
		Win32Check(::CreatePipe(&HRead, &HWrite, 0, 0));

		SetStdHandle(m_nStdHandle, HWrite);

		_free_osfhnd(m_fd);
		__acrt_lowio_set_os_handle(m_fd, (intptr_t)HWrite);
	}

	void Execute() override {
		Name = "UTF8-PipeConvertor";

		char buf[16], *e = buf + size(buf);

		DWORD dw;
		for (char *b = buf; Win32Check(::ReadFile(HRead, b, DWORD(e-b), &dw, 0), ERROR_BROKEN_PIPE);) {
			const char *next = 0;
			wchar_t wbuf[sizeof buf];
			wchar_t *toNext;
		LAB_AGAIN:
			switch (codecvt_base::result rc = m_cvt.in(m_state, buf, b+dw, next, wbuf, wbuf+size(wbuf), toNext)) {
			case codecvt_base::ok:
			case codecvt_base::partial:
			case codecvt_base::error:
				DWORD dwOut;
				Win32Check(::WriteConsole(HConsole, wbuf, DWORD(toNext-wbuf), &dwOut, 0));
				if (rc == codecvt_base::error) {
					TRC(3, "Cannot convert UTF-8 char 0x" << hex << int(byte(*next)));
					if (++next != b+dw) {
						wbuf[0] = '?';
						Win32Check(::WriteConsole(HConsole, wbuf, 1, &dwOut, 0));
						memmove(b=buf, next, dw -= DWORD(next-b));
						goto LAB_AGAIN;
					}
				}
			}
			memmove(buf, next, dw -= DWORD(next-b));
			b = buf + dw;
		}
		::CloseHandle(HRead);
	}

	void CloseOut() {
		_free_osfhnd(m_fd);
		__acrt_lowio_set_os_handle(m_fd, (intptr_t)HConsole);
		SetStdHandle(m_nStdHandle, HConsole);
		::CloseHandle(HWrite);
		Wait(HandleAccess(*this));
	}

	static void CreateIsTTY(PipeConvertor *&p, FILE *file) {
		if (isatty(fileno(file)))
			(p = new PipeConvertor(file))->Start();
	}
private:
	std::codecvt_utf8_utf16<wchar_t> m_cvt;
	mbstate_t m_state;
};

static PipeConvertor *s_pPipeConvertorStdout, *s_pPipeConvertorStderr;

static void __cdecl ClosePipeConvertor() {
	if (PipeConvertor *p = exchange(s_pPipeConvertorStdout, nullptr))
		p->CloseOut();
	if (PipeConvertor *p = exchange(s_pPipeConvertorStderr, nullptr))
		p->CloseOut();
}
#endif // UCFG_COUT_REDIRECTOR=='U'

void CConApp::InitOutRedirectors() {
#if UCFG_COUT_REDIRECTOR=='R'
	// _write() statically replaced;
#elif UCFG_COUT_REDIRECTOR=='U'
	setlocale(LC_CTYPE, "");
	_wsetlocale(LC_CTYPE, L"");

	// ploci->_public._locale_lc_codepage = CP_UTF8;

	static bool s_bSetUtf8Mode;
	if (!exchange(s_bSetUtf8Mode, true)) {
		PipeConvertor::CreateIsTTY(s_pPipeConvertorStdout, stdout);
		PipeConvertor::CreateIsTTY(s_pPipeConvertorStderr, stderr);
		RegisterAtExit(ClosePipeConvertor);

//		SetConsoleOutputCP(CP_UTF8);
	}
#elif UCFG_COUT_REDIRECTOR
	setlocale(LC_CTYPE, "");
	_wsetlocale(LC_CTYPE, L"");

	static bool s_bSetUtf8Mode;
	if (!exchange(s_bSetUtf8Mode, true)) {
#	if _VC_CRT_MAJOR_VERSION<14
		_setmode(_fileno(stdin), _O_U8TEXT);
		_setmode(_fileno(stdout), _O_U8TEXT);
		_setmode(_fileno(stderr), _O_U8TEXT);
#	endif
		static OutputForwarderBuffer coutBuffer(*cout.rdbuf(), wcout.rdbuf()),
			cerrBuffer(*cerr.rdbuf(), wcerr.rdbuf()),
			clogBuffer(*clog.rdbuf(), wclog.rdbuf()); 
	    cout.rdbuf( &coutBuffer );
    	cerr.rdbuf( &cerrBuffer );
	    clog.rdbuf( &clogBuffer );
	}
#else
	setlocale(LC_CTYPE, "");
#endif // UCFG_COUT_REDIRECTOR
}

} // Ext::



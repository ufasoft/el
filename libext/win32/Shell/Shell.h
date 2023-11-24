// Copyright(c) 2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Derived from AdfView code http://www.viksoe.dk/code/adfview.htm , Written by Bjarke Viksoe (bjarke@viksoe.dk)

#pragma once

#include <shobjidl.h>
#include <shlobj_core.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>

#include <el/gui/listview.h>
#include <el/gui/menu.h>

namespace Ext::Win::Shell {
using namespace ATL;
using namespace Ext::Gui;

#ifndef UCFG_USE_SHELL_DEFVIEW
#	define UCFG_USE_SHELL_DEFVIEW 1		// enable the Windows standard DefView implementation
#endif

extern const Ext::Guid
	GuidPreviewHandler
	, GuidPreviewHandlerWow64;

// Our EntryFlags enumeration. It defines additional flags
// for a file/folder. It is a bitfield.
enum ENTRYFLAGS {
	EF_DELETED = 1   // The file is deleted
};

struct ShellPath {
	LPITEMIDLIST _pidl;
public:
	bool IsChild() const { return ILIsChild(_pidl); }
	bool IsEmpty() const { return ILIsEmpty(_pidl); }

	PIDLIST_ABSOLUTE ClonePidl() const { return ::ILClone(_pidl); }

	ShellPath() : _pidl(0) {}

	ShellPath(const ShellPath& p)
		: _pidl(::ILCloneFull(p._pidl))
	{}

	ShellPath(PCIDLIST_ABSOLUTE pidl, bool takeOwnership = false)
		: _pidl(takeOwnership ? (LPITEMIDLIST)pidl : ::ILCloneFull(pidl))
	{}

	ShellPath(ShellPath&& p)
		: _pidl(std::exchange(p._pidl, nullptr))
	{}

	~ShellPath() { ::ILFree(_pidl); }

	void Attach(LPITEMIDLIST p) {
		if (_pidl)
			Throw(E_FAIL);
		_pidl = p;
	}

	LPITEMIDLIST Detach() {
		return exchange(_pidl, nullptr);
	}

	ShellPath& operator=(const ShellPath& p) {
		::ILFree(std::exchange(_pidl, ::ILCloneFull(p._pidl)));
		return *this;
	}

	operator PIDLIST_RELATIVE() const { return _pidl; }

	friend bool operator==(const ShellPath& a, const ShellPath& b) {
		return ::ILIsEqual(a, b);ShellPath r;
	}

	friend ShellPath operator/(const ShellPath& a, const ShellPath& b) {
		ShellPath r;
		r._pidl = ::ILCombine(a, b);
		return r;
	}

	ShellPath& operator/=(const ShellPath& p) {
		if (_pidl) {
			*this = *this / p;
		} else {
			_pidl = ::ILClone(p._pidl);
		}
		return *this;
	}

	size_t size() const {
		size_t r = 0;
		for (auto p = _pidl; !ILIsEmpty(p); p = ILNext(p))
			++r;
		return r;
	}

	static ShellPath FromString(const String& name) {
		TRC(1, name);
		auto cbName = name.length() * sizeof(WCHAR);
		auto cb = sizeof(USHORT) + cbName;
		auto p = (ITEMIDLIST*)::CoTaskMemAlloc(cb + sizeof(USHORT));
		memset(p, 0, cb + sizeof(USHORT));
		p->mkid.cb = (USHORT)cb;
		memcpy(p->mkid.abID, name.c_wstr(), cbName);
		return ShellPath(p, true);
	}

	static ShellPath FromDisplayName(const String& name) {
		TRC(1, name);
		ShellPath r;
		OleCheck(::SHParseDisplayName(name, nullptr, &r._pidl, 0, nullptr));
		return r;
	}

	String ToSimpleString() const {
		return _pidl->mkid.cb
			? String((const WCHAR*)_pidl->mkid.abID, (_pidl->mkid.cb - 2) / 2)
			: nullptr;
	}

	String ToString() const {
		TCHAR buf[MAX_PATH];
		Win32Check(::SHGetPathFromIDList(_pidl, buf));
		return buf;
	}
};

static_assert(sizeof(ShellPath) == sizeof(LPITEMIDLIST));

class ATL_NO_VTABLE CPidlEnum :
	public CComObjectRootEx<CComSingleThreadModel>
	, public CComCoClass<CPidlEnum>
	, public IEnumIDList
{
private:
	vector<ShellPath> ShellPaths;
	size_t pos_ = 0;
public:

	CPidlEnum() {}

	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CPidlEnum)
		COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
	END_COM_MAP()

	STDMETHOD(Next)(ULONG celt, LPITEMIDLIST* rgelt, ULONG* pceltFetched) {
		size_t size = ShellPaths.size();
		TRC(1, "CPidlEnum::Next() " << pos_ << " of " << size << " elements");
		*rgelt = NULL;
		if (pceltFetched)
			*pceltFetched = 0;
		if (!pceltFetched && celt != 1)
			return E_POINTER;
		ULONG nCount = 0;
		for (; pos_ < size && nCount < celt; ++pos_) {
			auto& path = ShellPaths[pos_];
			TRC(1, "path src: " << path.ToSimpleString());
			rgelt[nCount++] = path.ClonePidl();
		}
		if (pceltFetched)
			*pceltFetched = nCount;
		return celt == nCount ? S_OK : S_FALSE;
	}

	STDMETHODIMP Reset() METHOD_BEGIN {
		pos_ = 0;
	} METHOD_END

	STDMETHODIMP Skip(ULONG celt) METHOD_BEGIN {
		ULONG nCount = 0;
		size_t size = ShellPaths.size();
		for (; pos_ < size && nCount < celt; ++pos_, ++nCount)
			;
		return nCount == celt ? S_OK : S_FALSE;
	} METHOD_END

	void Init(const vector<ShellPath>& shellPaths, int pos = 0) {
		ShellPaths = shellPaths;
		pos_ = pos;
	}

	STDMETHODIMP Clone(IEnumIDList** ppenum) METHOD_BEGIN {
		CreateComInstance<CPidlEnum>(ppenum, ShellPaths, pos_);
	} METHOD_END
};

class ShellThreadRef {
public:
	static CUnkPtr get_Value();
	static void put_Value(IUnknown *unk);
};


// Constants that tell us the order of columns in the ListView control.
// These must be defined in display order.
enum COLUMNINFO_ID {
	COL_NAME = 0
	, COL_SIZE
	, COL_TYPE
	, COL_TIME
	, COL_ATTRIBS
};

// IShellViewImpl

enum TOOLBARITEM {
	TBI_STD = 0
	, TBI_VIEW
	, TBI_LOCAL
	, TBI_LAST
};

struct NS_TOOLBUTTONINFO {
	TOOLBARITEM nType;
	TBBUTTON tb;
};

template< class T >
class ATL_NO_VTABLE IShellViewImpl : public IShellView3
{
protected:
	bool _bErrorShown = false;
public:

	enum { IDC_LISTVIEW = 123 };

	BEGIN_MSG_MAP(IShellViewImpl<T>)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
		MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenu)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		NOTIFY_CODE_HANDLER(NM_SETFOCUS, OnNotifySetFocus)
		NOTIFY_CODE_HANDLER(NM_KILLFOCUS, OnNotifyKillFocus)
	END_MSG_MAP()
public:
	CWnd _wnd;
	AcceleratorTable _acceleratorTable;
	ListView listView;

	DWORD _dwListViewStyle;
#ifdef _DEBUG
	const MSG* m_pCurrentMsg;
#endif // _DEBUG
	SHELLFLAGSTATE m_ShellFlags;
	FOLDERSETTINGS _FolderSettings;
	CComPtr<IDropTarget> iDropTarget;
	CComQIPtr<IShellBrowser> iShellBrowser;
	CComQIPtr<ICommDlgBrowser> iCommDlg;
	ULONG m_dwProfferCookie;
	ULONG m_hChangeNotify;
	UINT m_uViewState;
	int m_iIconSize;
	HMENU m_hMenu = 0;

public:
	// IOleWindow
	STDMETHODIMP GetWindow(HWND* phWnd) METHOD_BEGIN {
		ATLASSERT(phWnd);
		TRC(1, "return HWND: " << _wnd.m_hWnd);
		*phWnd = _wnd;
	} METHOD_END

	STDMETHODIMP ContextSensitiveHelp(BOOL) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	// IShellView
	STDMETHODIMP TranslateAccelerator(LPMSG /*lpmsg*/) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP Refresh(void) METHOD_BEGIN {
		ATLASSERT(::IsWindow(listView));
		static_cast<T*>(this)->FillListView();		// Refill the list
	} METHOD_END

	STDMETHODIMP AddPropertySheetPages(DWORD /*dwReserved*/, LPFNADDPROPSHEETPAGE /*lpfn*/, LPARAM /*lParam*/) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		return pT->SelectAndPositionItem(pidlItem, uFlags, NULL);
	} METHOD_END

	STDMETHODIMP GetItemObject(UINT /*uItem*/, REFIID /*riid*/, LPVOID* ppRetVal) METHOD_BEGIN {
		ATLASSERT(ppRetVal);
		if (ppRetVal != NULL) *ppRetVal = NULL;
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP EnableModeless(BOOL /*fEnable*/) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetCurrentInfo(LPFOLDERSETTINGS lpFS) METHOD_BEGIN {
		ATLASSERT(lpFS);
		*lpFS = _FolderSettings;
	} METHOD_END

	STDMETHODIMP UIActivate(UINT uState) METHOD_BEGIN {
		// Only do this if we are active
		if (m_uViewState == uState) return S_OK;
		// ViewActivate() handles merging of menus etc
		T* pT = static_cast<T*>(this);
		if (SVUIA_ACTIVATE_FOCUS == uState)
			listView.SetFocus();
		pT->ViewActivate(uState);
		if (uState != SVUIA_DEACTIVATE && iShellBrowser) {
			// Update the status bar: set 'parts' and change text
			LRESULT lResult = 0;
			int nPartArray[1] = { -1 };
			iShellBrowser->SendControlMsg(FCW_STATUS, SB_SETPARTS, 1, (LPARAM)nPartArray, &lResult);
			// Set the statusbar text to the default description.
			iShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 0, (LPARAM)(const TCHAR*)pT->Description, &lResult);
		}
	} METHOD_END

	STDMETHODIMP CreateViewWindow(
			IShellView* /*lpPrevView*/,
			LPCFOLDERSETTINGS pFS,
			IShellBrowser* pSB,
			RECT* prcView,
			HWND* phWnd) METHOD_BEGIN {
		ATLASSERT(prcView);
		ATLASSERT(pSB);
		ATLASSERT(pFS);
		ATLASSERT(phWnd);

		T* pT = static_cast<T*>(this);

		*phWnd = NULL;

		// Register the ClassName.
		String className = pT->ClassName;
		// If our window class has not been registered, then do so now...
		WNDCLASS wc = { 0 };
		if (::GetClassInfo(pT->HInstance, className, &wc) == FALSE) {
			wc.style = 0;
			wc.lpfnWndProc = (WNDPROC)WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = pT->HInstance;
			wc.hIcon = NULL;
			wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			wc.lpszMenuName = NULL;
			wc.lpszClassName = className;
			if (!::RegisterClass(&wc)) return HRESULT_FROM_WIN32(::GetLastError());
		}

		// Set up the member variables
		iCommDlg = pSB;
		iShellBrowser = pSB;
		_FolderSettings = *pFS;
		m_ShellFlags.fWin95Classic = TRUE;
		m_ShellFlags.fShowAttribCol = TRUE;
		m_ShellFlags.fShowAllObjects = TRUE;

		// Get our parent window and create host window
		HWND hwndShell = NULL;
		iShellBrowser->GetWindow(&hwndShell);
		*phWnd = ::CreateWindowEx(WS_EX_CONTROLPARENT,
			className,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
			prcView->left, prcView->top,
			prcView->right - prcView->left, prcView->bottom - prcView->top,
			hwndShell,
			NULL,
			pT->HInstance,
			(LPVOID)pT);
		if (*phWnd == NULL) return HRESULT_FROM_WIN32(::GetLastError());

		pT->MergeToolbar(SVUIA_ACTIVATE_FOCUS);
	} METHOD_END

	STDMETHODIMP DestroyViewWindow(void) METHOD_BEGIN {
		// Make absolutely sure all our UI is cleaned up.
		UIActivate(SVUIA_DEACTIVATE);

		// Kill the window
		listView.Columns.Destroy();
		listView.Destroy();
		_wnd.Destroy();
		TRC(1, "Killed Windows");

		// Release the shell browser objects
		iShellBrowser.Release();
		iCommDlg.Release();
		TRC(1, "Released Dialogs");
	} METHOD_END

	STDMETHODIMP SaveViewState(void) METHOD_BEGIN {
		if (!iShellBrowser) return S_OK;
		CComPtr<IStream> spStream;
		iShellBrowser->GetViewStateStream(STGM_WRITE, &spStream);
		if (spStream == NULL) return S_OK;
		DWORD dwWritten = 0;
		spStream->Write(&_FolderSettings, sizeof(_FolderSettings), &dwWritten);
	} METHOD_END

	// IShellView2
	STDMETHODIMP CreateViewWindow2(LPSV2CVW2_PARAMS lpParams) METHOD_BEGIN {
		// The pvid takes precedence over pfs->ViewMode
		ViewModeFromSVID(lpParams->pvid, (FOLDERVIEWMODE*)&lpParams->pfs->ViewMode);
		// Create the view...
		return CreateViewWindow(lpParams->psvPrev, lpParams->pfs, lpParams->psbOwner, lpParams->prcView, &lpParams->hwndView);
	} METHOD_END

	STDMETHODIMP GetView(SHELLVIEWID* pvid, ULONG uView) METHOD_BEGIN {
		TRC(1, "uView: " << (long)uView)
		if (pvid == NULL) return E_POINTER;
		switch (uView) {
		case 0:                 return SVIDFromViewMode(FVM_ICON, pvid);
		case 1:                 return SVIDFromViewMode(FVM_SMALLICON, pvid);
		case 2:                 return SVIDFromViewMode(FVM_LIST, pvid);
		case 3:                 return SVIDFromViewMode(FVM_DETAILS, pvid);
		case 4:                 return SVIDFromViewMode(FVM_TILE, pvid);
		case SV2GV_CURRENTVIEW: return SVIDFromViewMode((FOLDERVIEWMODE)_FolderSettings.ViewMode, pvid);
		case SV2GV_DEFAULTVIEW: return SVIDFromViewMode(FVM_DETAILS, pvid);
		}
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP HandleRename(LPCITEMIDLIST pidl) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		POINT ptDummy = { 0 };
		return pT->SelectAndPositionItem(pidl, SVSI_EDIT, &ptDummy);
	} METHOD_END

	STDMETHODIMP SelectAndPositionItem(LPCITEMIDLIST /*pidlItem*/, UINT /*uFlags*/, POINT* /*point*/) METHOD_BEGIN {
	} METHOD_END

	// IShellView3
	STDMETHODIMP CreateViewWindow3(
			IShellBrowser* psbOwner,
			IShellView* psvPrev,
			SV3CVW3_FLAGS dwViewFlags,
			FOLDERFLAGS dwMask,
			FOLDERFLAGS dwFlags,
			FOLDERVIEWMODE fvMode,
			const SHELLVIEWID* /*pvid*/,
			const RECT* prcView,
			HWND* phwndView) METHOD_BEGIN {
		ATLASSERT(psbOwner);
		FOLDERSETTINGS fs = { 0 };
		CComPtr<IStream> spStream;
		psbOwner->GetViewStateStream(STGM_READ, &spStream);
		DWORD dwRead = 0;
		if (spStream != NULL) spStream->Read(&fs, sizeof(fs), &dwRead);
		if ((dwViewFlags & SV3CVW3_FORCEVIEWMODE) != 0) fs.ViewMode = fvMode;
		if ((dwViewFlags & SV3CVW3_FORCEFOLDERFLAGS) != 0) fs.fFlags = (dwFlags & dwMask);
		RECT rcView = *prcView;
		// Create the view...
		return CreateViewWindow(psvPrev, &fs, psbOwner, &rcView, phwndView);
	} METHOD_END

	// View handlers
	LRESULT ViewActivate(UINT uState) {
		ATLTRACE2(atlTraceCOM, 0, _T("IShellView::_ViewActivate %d\n"), uState);
		// Don't do anything if the state isn't really changing
		if (m_uViewState == uState) return S_OK;
		if (!::IsWindow(listView))
			return S_OK;
		T* pT = static_cast<T*>(this);
		// Deactivate old view/menus first
		pT->ViewDeactivate();
		// Only do this if we are now active...
		if (uState != SVUIA_DEACTIVATE) {
			pT->MergeMenus(uState);
			pT->UpdateToolbar();
		}
		m_uViewState = uState;
		return 0;
	}

	LRESULT ViewDeactivate() {
		ATLTRACE2(atlTraceCOM, 0, _T("IShellView::ViewDeactivate\n"));
		if (!::IsWindow(listView))
			return S_OK;
		T* pT = static_cast<T*>(this);
		pT->MergeMenus(SVUIA_DEACTIVATE);
		pT->MergeToolbar(SVUIA_DEACTIVATE);
		m_uViewState = SVUIA_DEACTIVATE;
		return 0;
	}

	// Since ::SHGetSettings() is not implemented in all versions of the shell, get the
	// function address manually at run time. This allows the extension to run on all
	// platforms.
	void GetShellSettings(SHELLFLAGSTATE& sfs, DWORD dwMask) {
		ATLASSERT(::GetModuleHandle(_T("SHELL32.DLL")) != NULL);
		typedef void (WINAPI* PFNSHGETSETTINGSPROC)(LPSHELLFLAGSTATE, DWORD);
		static PFNSHGETSETTINGSPROC fnSHGetSettings = (PFNSHGETSETTINGSPROC) ::GetProcAddress(::GetModuleHandle(_T("SHELL32.DLL")), "SHGetSettings");
		if (fnSHGetSettings != NULL) fnSHGetSettings(&sfs, dwMask);
	}

	// Windows Vista assigns its own theme to its Shell controls (ie. ListView). We'll allow
	// it too.
	void _SetShellControlTheme(HWND hWnd) {
		typedef void (WINAPI* PFNSETWINDOWTHEME)(HWND, LPCWSTR, LPVOID);
		static HMODULE hInstUtx = ::LoadLibrary(_T("UxTheme.dll"));
		if (hInstUtx == NULL) return;
		static PFNSETWINDOWTHEME fnSetWindowTheme = (PFNSETWINDOWTHEME) ::GetProcAddress(hInstUtx, "SetWindowTheme");
		if (fnSetWindowTheme != NULL) fnSetWindowTheme(hWnd, L"explorer", NULL);
	}

	// Create an IDropTarget (through the IShellFolder::CreateViewObject) and register it as drag'n'drop
	// for the list window.
	void RegisterDropTarget() {
		iDropTarget.Release();
		T* pT = static_cast<T*>(this);
		TRC(1, "Getting SVGIO_BACKGROUND...");
		OleCheck(pT->GetItemObject(SVGIO_BACKGROUND, IID_IDropTarget, (LPVOID*)&iDropTarget));
		TRC(1, "Registering DragDrop...");
		OleCheck(::RegisterDragDrop(listView, iDropTarget));
		TRC(1, "Registered DragDrop");
	}


	// Register for view changes.
	// This API was previously undocumented and thus only exported by ordinal. Since then,
	// Microsoft was forced to document it (reluctantly) in a DoJ anti-trust settlement.
	HRESULT _RegisterChangeNotify(LPCITEMIDLIST pidl, UINT uMsg, LONG fEvents = SHCNE_UPDATEDIR) {
#define SHCNF_ACCEPT_INTERRUPTS     0x0001
#define SHCNF_ACCEPT_NON_INTERRUPTS 0x0002
		typedef ULONG(WINAPI* PFNSHCHANGENOTIFYREGISTER)(HWND, int, LONG, UINT, int, SHChangeNotifyEntry*);
		PFNSHCHANGENOTIFYREGISTER fnSHChangeNotifyRegister = NULL;
		if (fnSHChangeNotifyRegister == NULL)
			fnSHChangeNotifyRegister = (PFNSHCHANGENOTIFYREGISTER) ::GetProcAddress(::GetModuleHandle(_T("SHELL32.DLL")), "SHChangeNotifyRegister");
		if (fnSHChangeNotifyRegister == NULL)
			fnSHChangeNotifyRegister = (PFNSHCHANGENOTIFYREGISTER) ::GetProcAddress(::GetModuleHandle(_T("SHELL32.DLL")), MAKEINTRESOURCEA(2));
		if (fnSHChangeNotifyRegister == NULL) return E_NOTIMPL;
		SHChangeNotifyEntry Nr = { pidl, TRUE };
		m_hChangeNotify = fnSHChangeNotifyRegister(_wnd.m_hWnd,
			SHCNF_ACCEPT_INTERRUPTS | SHCNF_ACCEPT_NON_INTERRUPTS,
			fEvents,
			uMsg,
			1, &Nr);
		return m_hChangeNotify != NULL ? S_OK : E_FAIL;
	}

	void _RevokeChangeNotify() {
		if (m_hChangeNotify == NULL) return;
		typedef BOOL(WINAPI* PFNSHCHANGENOTIFYDEREGISTER)(ULONG);
		PFNSHCHANGENOTIFYDEREGISTER fnSHChangeNotifyDeregister = NULL;
		if (fnSHChangeNotifyDeregister == NULL)
			fnSHChangeNotifyDeregister = (PFNSHCHANGENOTIFYDEREGISTER) ::GetProcAddress(::GetModuleHandle(_T("SHELL32.DLL")), "SHChangeNotifyDeregister");
		if (fnSHChangeNotifyDeregister == NULL)
			fnSHChangeNotifyDeregister = (PFNSHCHANGENOTIFYDEREGISTER) ::GetProcAddress(::GetModuleHandle(_T("SHELL32.DLL")), MAKEINTRESOURCEA(4));
		if (fnSHChangeNotifyDeregister == NULL) return;
		fnSHChangeNotifyDeregister(m_hChangeNotify);
		m_hChangeNotify = NULL;
	}

	HRESULT _RegisterProffer(REFGUID guidService) {
		m_dwProfferCookie = 0;
		CComQIPtr<IProfferService> spProffer = iShellBrowser;
		CComQIPtr<IServiceProvider> spProvider = this;
		if (spProffer == NULL || spProvider == NULL) return E_NOINTERFACE;
		return spProffer->ProfferService(guidService, spProvider, &m_dwProfferCookie);
	}

	void _RevokeProffer() {
		if (m_dwProfferCookie)
			if (CComQIPtr<IProfferService> spProffer = iShellBrowser)
				spProffer->RevokeService(exchange(m_dwProfferCookie, 0));
	}

	// A helper function which will take care of some of
	// the fancy new Win98 settings...
	void _UpdateShellSettings(void) {
		// Get the m_ShellFlags state
		GetShellSettings(m_ShellFlags,
			SSF_DESKTOPHTML |
			SSF_NOCONFIRMRECYCLE |
			SSF_SHOWALLOBJECTS |
			SSF_SHOWATTRIBCOL |
			SSF_DOUBLECLICKINWEBVIEW |
			SSF_SHOWCOMPCOLOR |
			SSF_WIN95CLASSIC);

#ifndef LVS_EX_DOUBLEBUFFER
		const DWORD LVS_EX_DOUBLEBUFFER = 0x00010000;
#endif // LVS_EX_DOUBLEBUFFER
#ifndef LVS_EX_HEADERINALLVIEWS
		const DWORD LVS_EX_HEADERINALLVIEWS = 0x02000000;
#endif // LVS_EX_HEADERINALLVIEWS
#ifndef FWF_FULLROWSELECT
		const DWORD FWF_FULLROWSELECT = 0x200000;
#endif // FWF_FULLROWSELECT

		// Update the ListView control accordingly
		DWORD dwExStyles = LVS_EX_HEADERDRAGDROP |      // Allow but no auto-persist
			LVS_EX_DOUBLEBUFFER |        // Causes blue marquee on WinXP
			LVS_EX_HEADERINALLVIEWS;     // Causes headers to always display on Vista
		if (!m_ShellFlags.fWin95Classic && !m_ShellFlags.fDoubleClickInWebView) {
			dwExStyles |= LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT;
		}
		T* pT = static_cast<T*>(this);
		if (Environment::OSVersion.Version.Major >= 6 || (FWF_FULLROWSELECT & _FolderSettings.fFlags) != 0) {
			dwExStyles |= LVS_EX_FULLROWSELECT;
		}
		if ((FWF_SINGLECLICKACTIVATE & _FolderSettings.fFlags) != 0) {
			dwExStyles |= LVS_EX_ONECLICKACTIVATE;
		}
		ListView_SetExtendedListViewStyle(listView, dwExStyles);
	}

	LRESULT DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam) {
		return ::DefWindowProc(_wnd.m_hWnd, uMsg, wParam, lParam);
	}

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {
		try {
			TRC(1, "HWND: " << hWnd << ", Message: " << Win::Message(uMessage))
				T* pT = reinterpret_cast<T*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (uMessage == WM_NCCREATE) {
				LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
				pT = (T*)lpcs->lpCreateParams;
				::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pT);
				pT->_wnd.Attach(hWnd); // Set the window handle
				return 1;
			}
			ATLASSERT(pT);
#ifdef _DEBUG
			MSG msg = { pT->_wnd.m_hWnd, uMessage, wParam, lParam, 0, { 0, 0 } };
			const MSG* pOldMsg = pT->m_pCurrentMsg;
			pT->m_pCurrentMsg = &msg;
#endif // _DEBUG
			// pass to the message map to process
			LRESULT lRes = 0;
			BOOL bRet = pT->ProcessWindowMessage(pT->_wnd, uMessage, wParam, lParam, lRes, 0);
			// Restore saved value for the current message
#ifdef _DEBUG
			ATLASSERT(pT->m_pCurrentMsg == &msg);
			pT->m_pCurrentMsg = pOldMsg;
#endif // _DEBUG
			if (!bRet) lRes = pT->DefWindowProc(uMessage, wParam, lParam);
			return lRes;
		} catch (const Exception& e) {
			TRC(1, "WndProc exception: " << e);
			return 0;
		}
	}

	// Message handlers

	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ATLTRACE2(atlTraceWindowing, 0, _T("IShellView::OnSetFocus\n"));
		if (::IsWindow(listView))
			listView.SetFocus();
		return 0;
	}

	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ATLTRACE2(atlTraceWindowing, 0, _T("IShellView::OnKillFocus\n"));
		return 0;
	}

	LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		T* pT = static_cast<T*>(this);
		pT->_UpdateShellSettings();
		return 0;
	}

	LRESULT OnNotifySetFocus(UINT /*CtlID*/, LPNMHDR /*lpnmh*/, BOOL&/*bHandled*/) {
		ATLTRACE2(atlTraceWindowing, 0, _T("IShellView::OnNotifySetFocus\n"));
		if (!::IsWindow(listView))
			return 0;
		if (m_uViewState == SVUIA_DEACTIVATE) return 0;
		// Tell the browser one of our windows has received the focus. This should always
		// be done before merging menus (ViewActivate() merges the menus) if one of our
		// windows has the focus.
		T* pT = static_cast<T*>(this);
		if (iShellBrowser) iShellBrowser->OnViewWindowActive(pT);
		pT->ViewActivate(SVUIA_ACTIVATE_FOCUS);
		if (iCommDlg != NULL) iCommDlg->OnStateChange(this, CDBOSC_SETFOCUS);
		return 0;
	}

	LRESULT OnNotifyKillFocus(UINT /*CtlID*/, LPNMHDR /*lpnmh*/, BOOL&/*bHandled*/) {
		ATLTRACE2(atlTraceWindowing, 0, _T("IShellView::OnNotifyKillFocus\n"));
		if (!::IsWindow(listView))
			return 0;
		if (m_uViewState == SVUIA_DEACTIVATE) return 0;
		T* pT = static_cast<T*>(this);
		pT->ViewActivate(SVUIA_ACTIVATE_NOFOCUS);
		if (iCommDlg != NULL) iCommDlg->OnStateChange(this, CDBOSC_KILLFOCUS);
		return 0;
	}

	LRESULT OnInitMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		T* pT = static_cast<T*>(this);
		pT->UpdateMenu((HMENU)wParam, NULL);
		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		// Resize the ListView to fit our window
		if (::IsWindow(listView))
			::MoveWindow(listView, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 1; // avoid flicker
	}

	LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
		TRC(1, "");
		// Create the ListView
		try {
			T* pT = static_cast<T*>(this);
			CreateListView();
			pT->InitListView();
			pT->FillListView();

			RegisterDropTarget();
		} catch (RCExc e) {
			TRC(1, e.what());
		}
		TRC(1, "returning");
		return 0;
	}

	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
		::RevokeDragDrop(listView);
		iDropTarget.Release();
		return 0;
	}

	// Operations

	BOOL AppendToolbarItems(
		const NS_TOOLBUTTONINFO *pButtons,
		int nCount,
		LPARAM lOffsetFile,
		LPARAM lOffsetView,
		LPARAM lOffsetCustom) {
		ATLASSERT(nCount > 0);
		if (!iShellBrowser) return TRUE;
		LPTBBUTTON ptbb = (LPTBBUTTON) ::GlobalAlloc(GPTR, sizeof(TBBUTTON) * nCount);
		if (ptbb == NULL) return FALSE;
		int i;
		for (i = 0; i < nCount; i++) {
			switch (pButtons[i].nType) {
			case TBI_STD:   ptbb[i].iBitmap = (int)lOffsetFile + pButtons[i].tb.iBitmap; break;
			case TBI_VIEW:  ptbb[i].iBitmap = (int)lOffsetView + pButtons[i].tb.iBitmap; break;
			case TBI_LOCAL: ptbb[i].iBitmap = (int)lOffsetCustom + pButtons[i].tb.iBitmap; break;
			}
			if (pButtons[i].nType == TBI_LAST) break;
			ptbb[i].idCommand = pButtons[i].tb.idCommand;
			ptbb[i].fsState = pButtons[i].tb.fsState;
			ptbb[i].fsStyle = pButtons[i].tb.fsStyle;
			ptbb[i].dwData = pButtons[i].tb.dwData;
			ptbb[i].iString = pButtons[i].tb.iString;
		}
		iShellBrowser->SetToolbarItems(ptbb, i, FCT_MERGE);
		::GlobalFree((HGLOBAL)ptbb);
		return TRUE;
	}

	void AppendToMenu(HMENU hMenu, Menu menuSource, UINT nPosition) {
		TRC_INDENT;
		TRC(1, "Position: " << nPosition);
		ATLASSERT(::IsMenu(hMenu));
		ATLASSERT(::IsMenu(menuSource));
		// Get the HMENU of the popup
		// Make sure that we start with only one separator menu-item
		int iStartPos = 0;
		if (menuSource.MenuItems[0].IsSeparator
			&& ((nPosition == 0) || (::GetMenuState(hMenu, nPosition - 1, MF_BYPOSITION) & MF_SEPARATOR) != 0))
				iStartPos++;
		// Go...
		int nMenuItems = menuSource.MenuItems.size();
		TRC(1, "nMenuItems: " << nMenuItems);
		for (int i = iStartPos; i < nMenuItems; i++) {
			// Get state information
			UINT state = ::GetMenuState(menuSource, i, MF_BYPOSITION);
			MenuItem menuItem = menuSource.MenuItems[i];
			TRC(1, "menu item ID: " << menuItem.MenuID << " State: " << hex << state);
			String itemText = menuItem.Text;
			TRC(1, "itemText: " << itemText);
			// Is this a separator?
			if (menuItem.IsSeparator) {
				::InsertMenu(hMenu, nPosition++, state | MF_STRING | MF_BYPOSITION, 0, _T(""));
			} else if (menuItem.IsParent) {
				// Strip the HIBYTE because it contains a count of items
				state = LOBYTE(state) | MF_POPUP;
				// Then create the new submenu by using recursive call
				HMENU hSubMenu = ::CreateMenu();
				AppendToMenu(hSubMenu, menuSource.MenuItems[i], 0);
				ATLASSERT(::GetMenuItemCount(hSubMenu) > 0);
				// Non-empty popup -- add it to the shared menu bar
				::InsertMenu(hMenu, nPosition++, state | MF_BYPOSITION, (UINT_PTR)hSubMenu, itemText);
			} else if (itemText.length() > 0) {
				// Only non-empty items should be added
				auto menuItemID = menuItem.MenuID;
				ATLASSERT(menuItemID > FCIDM_SHVIEWFIRST && menuItemID < FCIDM_SHVIEWLAST);
				// Here the state does not contain a count in the HIBYTE
				::InsertMenu(hMenu, nPosition++, state | MF_BYPOSITION, menuItemID, itemText);
			}
		}
	}

	HRESULT ViewModeFromSVID(const SHELLVIEWID* pvid, FOLDERVIEWMODE* pViewMode) const {
		if (pViewMode != NULL) *pViewMode = FVM_ICON;
		if (pvid == NULL || pViewMode == NULL) return E_INVALIDARG;
		if (*pvid == VID_LargeIcons)      *pViewMode = FVM_ICON;
		else if (*pvid == VID_SmallIcons) *pViewMode = FVM_SMALLICON;
		else if (*pvid == VID_Thumbnails) *pViewMode = FVM_THUMBNAIL;
		else if (*pvid == VID_ThumbStrip) *pViewMode = FVM_THUMBSTRIP;
		else if (*pvid == VID_List)       *pViewMode = FVM_LIST;
		else if (*pvid == VID_Tile)       *pViewMode = FVM_TILE;
		else if (*pvid == VID_Details)    *pViewMode = FVM_DETAILS;
		else return E_FAIL;
		return S_OK;
	}

	HRESULT SVIDFromViewMode(FOLDERVIEWMODE mode, SHELLVIEWID* svid) const {
		ATLASSERT(svid);
		switch (mode) {
		case FVM_SMALLICON:  *svid = VID_SmallIcons; break;
		case FVM_LIST:       *svid = VID_List;       break;
		case FVM_DETAILS:    *svid = VID_Details;    break;
		case FVM_THUMBNAIL:  *svid = VID_Thumbnails; break;
		case FVM_TILE:       *svid = VID_Tile;       break;
		case FVM_THUMBSTRIP: *svid = VID_ThumbStrip; break;
		case FVM_ICON:       *svid = VID_LargeIcons; break;
		default:             *svid = VID_LargeIcons; break;
		}
		return S_OK;
	}

	bool IsExplorerMode() const {
		if (!iShellBrowser) return FALSE;
		// MSDN actually documents that we can determine if we're in explorer mode
		// by asking for the tree control.
		HWND hwndTree = NULL;
		return SUCCEEDED(iShellBrowser->GetControlWindow(FCW_TREE, &hwndTree)) && hwndTree != NULL;
	}

	bool IsCommDlgMode() const {
		return iCommDlg != NULL;
	}

	// Overridables

	void CreateListView() {
		ATLASSERT((_dwListViewStyle & (WS_VISIBLE | WS_CHILD)) == (WS_VISIBLE | WS_CHILD));

		// Initialize and create the actual List View control
		_dwListViewStyle &= ~LVS_TYPEMASK;
		_dwListViewStyle |= LVS_ICON;
		if ((FWF_ALIGNLEFT & _FolderSettings.fFlags) != 0) _dwListViewStyle |= LVS_ALIGNLEFT;
		if ((FWF_AUTOARRANGE & _FolderSettings.fFlags) != 0) _dwListViewStyle |= LVS_AUTOARRANGE;
#if (_WIN32_IE >= 0x0500)
		if ((FWF_SHOWSELALWAYS & _FolderSettings.fFlags) != 0) _dwListViewStyle |= LVS_SHOWSELALWAYS;
#endif // _WIN32_IE
		T* pT = static_cast<T*>(this);
		if (Environment::OSVersion.Version.Major >= 6 )
			_FolderSettings.fFlags |= FWF_NOCLIENTEDGE;

		// Go on, create the ListView control
		HWND hwndList = ::CreateWindowEx((FWF_NOCLIENTEDGE & _FolderSettings.fFlags) != 0 ? 0 : WS_EX_CLIENTEDGE,
			WC_LISTVIEW,
			NULL,
			_dwListViewStyle,
			0, 0, 0, 0,
			_wnd,
			(HMENU)IDC_LISTVIEW,
			pT->HInstance,
			NULL);
		Win32Check(hwndList != 0);
		listView.Attach(hwndList);

		pT->_SetShellControlTheme(listView);
		pT->_UpdateShellSettings();
	}

	void InitListView();
	void FillListView();
	virtual void UpdateMenu(HMENU, LPCITEMIDLIST) {}
	virtual void UpdateToolbar() {}
	virtual void MergeMenus(UINT) {}
	virtual void MergeToolbar(UINT) {}
public:

	IShellViewImpl() : m_uViewState(SVUIA_DEACTIVATE), m_hChangeNotify(NULL), m_dwProfferCookie(0UL) {
		ZeroStruct(m_ShellFlags);
		ZeroStruct(_FolderSettings);
		m_iIconSize = ::GetSystemMetrics(SM_CXICON);
		_FolderSettings.ViewMode = FVM_DETAILS;
		_FolderSettings.fFlags = FWF_AUTOARRANGE | FWF_SHOWSELALWAYS;
		_dwListViewStyle = WS_TABSTOP | WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS;
	}

	~IShellViewImpl() {
		TRC(1, "");
	}
};

// IFolderView for ListView control
template <class T>
class ATL_NO_VTABLE IFolderView2Impl : public IFolderView2
{
public:
	// IFolderView1
	METHOD_SPEC GetCurrentViewMode(UINT* pViewMode) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*pViewMode = pT->_FolderSettings.ViewMode;
	} METHOD_END

	METHOD_SPEC SetCurrentViewMode(UINT ViewMode) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		pT->_FolderSettings.ViewMode = ViewMode;
		pT->InitListView();
		pT->FillListView();
	} METHOD_END

	METHOD_SPEC GetFolder(REFIID riid, LPVOID* ppv) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		return pT->m_pFolder->QueryInterface(riid, ppv);
	} METHOD_END

	METHOD_SPEC Item(int iItemIndex, LPITEMIDLIST* ppidl) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*ppidl = ShellPath::FromString(pT->listView.Items[iItemIndex].Text).Detach();
	} METHOD_END

	STDMETHODIMP ItemCount(UINT uFlags, int* pcItems) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*pcItems = (uFlags & SVGIO_TYPE_MASK) == SVGIO_CHECKED
			? 0
			: ((uFlags & SVGIO_TYPE_MASK) == SVGIO_SELECTION ? pT->listView.get_Selected().size() : pT->listView.Items.size());
	} METHOD_END

	STDMETHODIMP Items(UINT uFlags, REFIID riid, LPVOID* ppv) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		if ((uFlags & SVGIO_SELECTION) != 0 && pT->listView.get_Selected().empty())
			return pT->m_pFolder->CreateViewObject(pT->_wnd, riid, ppv);
		return pT->GetItemObject(uFlags, riid, ppv);
	} METHOD_END

	STDMETHODIMP GetSelectionMarkedItem(int* /*piItem*/) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetFocusedItem(int* piItem) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*piItem = ListView_GetNextItem(pT->listView, -1, LVNI_FOCUSED);
		return *piItem >= 0 ? S_OK : E_FAIL;
	} METHOD_END

	STDMETHODIMP GetItemPosition(LPCITEMIDLIST pidl, POINT* ppt) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		int iItem = GetListIndexFromPidl(pidl);
		if (iItem == -1) return E_FAIL;
		*ppt = pT->listView.Items[iItem].Position;
	} METHOD_END

	STDMETHODIMP GetSpacing(POINT* ppt) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		DWORD dwSpacing = ListView_GetItemSpacing(pT->listView, FALSE);
		ppt->x = LOWORD(dwSpacing);
		ppt->y = HIWORD(dwSpacing);
	} METHOD_END

	STDMETHODIMP GetDefaultSpacing(POINT* ppt) METHOD_BEGIN {
		ppt->x = ::GetSystemMetrics(SM_CXICONSPACING);
		ppt->y = ::GetSystemMetrics(SM_CYICONSPACING);
	} METHOD_END

	STDMETHODIMP GetAutoArrange(void) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		return pT->listView.AutoArrange ? S_OK : S_FALSE;
	} METHOD_END

	STDMETHODIMP SelectItem(int iItemIndex, DWORD uFlags) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		LPCITEMIDLIST pidl = GetPidlFromListIndex(iItemIndex);
		if (pidl == NULL) return E_FAIL;
		return pT->SelectAndPositionItem(pidl, uFlags, NULL);
	} METHOD_END

	STDMETHODIMP SelectAndPositionItems(UINT cidl, LPCITEMIDLIST* apidl, POINT* apt, DWORD uFlags) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		for (UINT i = 0; i < cidl; i++) {
			HRESULT Hr = pT->SelectAndPositionItem(apidl[i], uFlags, apt);
			if (FAILED(Hr)) return Hr;
			uFlags &= ~(SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_EDIT);
		}
	} METHOD_END

	// IFolderView2
	STDMETHODIMP SetGroupBy(REFPROPERTYKEY, BOOL) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetGroupBy(PROPERTYKEY*, BOOL*) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetViewProperty(LPCITEMIDLIST, REFPROPERTYKEY, REFPROPVARIANT) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetViewProperty(LPCITEMIDLIST, REFPROPERTYKEY, PROPVARIANT*) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetTileViewProperties(LPCITEMIDLIST, LPCWSTR) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetExtendedTileViewProperties(LPCITEMIDLIST, LPCWSTR) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetText(FVTEXTTYPE, LPCWSTR) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetCurrentFolderFlags(DWORD dwMask, DWORD dwValue) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		pT->_FolderSettings.fFlags = (pT->_FolderSettings.fFlags & ~dwMask) | (dwValue & dwMask);
		if (::IsWindow(pT->listView))
			pT->_UpdateShellSettings();
	} METHOD_END

	STDMETHODIMP GetCurrentFolderFlags(DWORD* pFlags) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*pFlags = pT->_FolderSettings.fFlags;
	} METHOD_END

	STDMETHODIMP GetSortColumnCount(int*) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetSortColumns(const SORTCOLUMN*, int) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetSortColumns(SORTCOLUMN*, int) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetItem(int, REFIID, void**) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetVisibleItem(int iStartIndex, BOOL fPrevious, int* piIndex) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		* piIndex = ListView_GetNextItem(pT->listView, iStartIndex, LVNI_VISIBLEONLY | (fPrevious ? LVNI_PREVIOUS : 0));
		return *piIndex == -1 ? S_FALSE : S_OK;
	} METHOD_END

	STDMETHODIMP GetSelectedItem(int iStartIndex, int* piIndex) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*piIndex = ListView_GetNextItem(pT->listView, iStartIndex, LVNI_SELECTED);
		return *piIndex == -1 ? S_FALSE : S_OK;
	} METHOD_END

	STDMETHODIMP GetSelection(BOOL fNoneImpliesFolder, IShellItemArray** ppsia) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		if (fNoneImpliesFolder) return pT->Items(SVGIO_SELECTION, IID_IShellItemArray, (LPVOID*)ppsia);
		return pT->GetItemObject(SVGIO_SELECTION, IID_IShellItemArray, (LPVOID*)ppsia);
	} METHOD_END

	STDMETHODIMP GetSelectionState(LPCITEMIDLIST pidl, DWORD* puState) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*puState = 0;
		int iItem = GetListIndexFromPidl(pidl);
		if (iItem == -1) return E_FAIL;
		DWORD dwState = ListView_GetItemState(pT->listView, iItem, LVIS_SELECTED | LVIS_FOCUSED);
		if ((dwState & LVIS_FOCUSED) != 0) *puState = SVSI_FOCUSED;
		if ((dwState & LVIS_SELECTED) != 0) *puState = SVSI_SELECT;
	} METHOD_END

	STDMETHODIMP InvokeVerbOnSelection(LPCSTR) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetViewModeAndIconSize(FOLDERVIEWMODE uViewMode, int iIconSize) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		pT->_FolderSettings.ViewMode = (int)uViewMode;
		pT->m_iIconSize = iIconSize;
		pT->InitListView();
		pT->FillListView();
	} METHOD_END

	STDMETHODIMP GetViewModeAndIconSize(FOLDERVIEWMODE* puViewMode, int* piIconSize) METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		*puViewMode = (FOLDERVIEWMODE)pT->_FolderSettings.ViewMode;
		*piIconSize = (pT->listView.GetLong(GWL_STYLE) & LVS_ICON) == 0 ? ::GetSystemMetrics(SM_CXSMICON) : pT->m_iIconSize;
	} METHOD_END

	STDMETHODIMP SetGroupSubsetCount(UINT) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP GetGroupSubsetCount(UINT*) METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP SetRedraw(BOOL bRedraw) METHOD_BEGIN {
		static_cast<T*>(this)->listView.SetRedraw(bRedraw);
	} METHOD_END

	STDMETHODIMP IsMoveInSameFolder() METHOD_BEGIN {
		return E_NOTIMPL;
	} METHOD_END

	STDMETHODIMP DoRename() METHOD_BEGIN {
		T* pT = static_cast<T*>(this);
		pT->listView.SetFocus();
		return ListView_EditLabel(pT->listView, ListView_GetNextItem(pT->listView.m_hWnd, -1, LVNI_SELECTED)) == NULL ? E_FAIL : S_OK;
	} METHOD_END

	// Implementation

	LPCITEMIDLIST GetPidlFromListIndex(int iItemIndex) {
		T* pT = static_cast<T*>(this);
		LVITEM lvItem = { 0 };
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = iItemIndex;
		if (!ListView_GetItem(pT->listView, &lvItem))
			return NULL;
		return reinterpret_cast<LPCITEMIDLIST>(lvItem.lParam);
	}

	int GetListIndexFromPidl(LPCITEMIDLIST pidl) {
		auto path = ShellPath(pidl).ToSimpleString();
		T* pT = static_cast<T*>(this);
		for (int i = pT->listView.Items.size(); i-- > 0;)
			if (pT->listView.Items[i].Text == path)
				return i;
		return -1;
	}
};

} // Ext::Win::Shell::

#pragma once

#include <winuser.h>
#include <shellapi.h>
#include <commctrl.h>



#if UCFG_COM
#	include <unknwn.h>
#endif

#if UCFG_GUI
//#	include <commctrl.h>
#endif

#include "ext-win.h"

namespace Ext {



class CListBox;
class CFont;

class CCreateContext;

struct T_WNDCLASS : WNDCLASS {
};
AFX_API void AFXAPI AfxRegisterClass(T_WNDCLASS *wndclass);

class CRgn;
class CScrollBar;

// CWnd::m_nFlags (generic to CWnd)
#define WF_TOOLTIPS         0x0001  // window is enabled for tooltips
#define WF_TEMPHIDE         0x0002  // window is temporarily hidden
#define WF_STAYDISABLED     0x0004  // window should stay disabled
#define WF_MODALLOOP        0x0008  // currently in modal loop
#define WF_CONTINUEMODAL    0x0010  // modal loop should continue running
#define WF_OLECTLCONTAINER  0x0100  // some descendant is an OLE control
#define WF_TRACKINGTOOLTIPS 0x0400  // window is enabled for tracking tooltips

// CWnd::m_nFlags (specific to CFrameWnd)
#define WF_STAYACTIVE       0x0020  // look active even though not active
#define WF_NOPOPMSG         0x0040  // ignore WM_POPMESSAGESTRING calls
#define WF_MODALDISABLE     0x0080  // window is disabled
#define WF_KEEPMINIACTIVE   0x0200  // stay activate even though you are deactivated

// flags for CWnd::RunModalLoop
#define MLF_NOIDLEMSG       0x0001  // don't send WM_ENTERIDLE messages
#define MLF_NOKICKIDLE      0x0002  // don't send WM_KICKIDLE messages
#define MLF_SHOWONIDLE      0x0004  // show window if not visible at idle time

// extra MFC defined TTF_ flags for TOOLINFO::uFlags
#define TTF_NOTBUTTON       0x80000000L // no status help on buttondown
#define TTF_ALWAYSTIP       0x40000000L // always show the tip even if not active

class CWnd : public CCmdTarget {	/*!!!AFX_CLASS*/
public:
	mutable HWND m_hWnd;
	HWND m_hWndOwner,
		m_hWndTop;
	UINT m_nFlags;
	bool m_bAutoDelete;
	CBool m_bIsView;

	static AFX_DATA const CWnd wndTop; // SetWindowPos's pWndInsertAfter
	static AFX_DATA const CWnd wndBottom; // SetWindowPos's pWndInsertAfter
	static AFX_DATA const CWnd wndTopMost; // SetWindowPos pWndInsertAfter
	static AFX_DATA const CWnd wndNoTopMost; // SetWindowPos pWndInsertAfter

	enum AdjustType { adjustBorder = 0, adjustOutside = 1 };

	CWnd();
	~CWnd();

	__forceinline operator HWND() const { return get_Handle(); }

	static int AFXAPI AfxGetDlgCtrlID(HWND hWnd);

	void CreateEx(DWORD dwExStyle, RCString className, RCString windowName, DWORD dwStyle, int x, int y, int width, int height, HWND hwndParent, HMENU nIDorHMenu, LPVOID lpParam = 0);
#if UCFG_COM
	bool CreateControl(REFCLSID clsid, RCString pszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, Stream *pStream, bool bStorage, BSTR bstrLicKey);
	bool CreateControl(REFCLSID clsid, LPCTSTR pszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, File *pFile = 0, bool bStorage = false, BSTR bstrLicKey = 0);
	LPUNKNOWN GetControlUnknown();
#endif
	HWND GetSafeHwnd() const { return this ? m_hWnd : 0; }

	void Attach(HWND hWndNew);
	HWND Detach();
	void Center(CWnd* pAlternateOwner = 0);
	void UpdateDialogControls(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);
	void BringToTop(int nCmdShow = SW_SHOWNORMAL);
	CWnd *SetActive();
	LRESULT Default();

	bool IsVisible() const { return ::IsWindowVisible(m_hWnd); }

	bool get_IsEnabled() const { return ::IsWindowEnabled(m_hWnd); }
	void put_IsEnabled(bool v) { ::EnableWindow(m_hWnd, v); }
	DEFPROP(bool, IsEnabled);

	DWORD get_Style() const { return (DWORD)GetLong(GWL_STYLE); }
	void put_Style(DWORD v) { SetLong(GWL_STYLE, v); }
	DEFPROP(DWORD, Style);

	DWORD get_ExStyle() const { return (DWORD)GetLong(GWL_EXSTYLE); }
	DEFPROP_GET(DWORD, ExStyle);

	DWORD get_ThreadID() { return ::GetWindowThreadProcessId(m_hWnd, 0); }
	DEFPROP_GET(DWORD, ThreadID);

	DWORD get_ProcessID() {
		DWORD dw;
		::GetWindowThreadProcessId(m_hWnd, &dw);
		return dw;
	}
	DEFPROP_GET(DWORD, ProcessID);

	static CWnd* GetDesktopWindow() { return FromHandle(::GetDesktopWindow()); }

	static CWnd * AFXAPI get_Foreground() { return FromHandle(::GetForegroundWindow()); }
	static void AFXAPI put_Foreground(CWnd *wnd) { Win32Check(::SetForegroundWindow(wnd->m_hWnd)); }
	STATIC_PROPERTY_DEF(CWnd, CWnd*, Foreground);

	static CWnd * AFXAPI get_Active() { return FromHandle(::GetActiveWindow()); }
	EXT_DATA STATIC_PROPERTY_DEF_GET(CWnd, CWnd*, Active);

	static CWnd * AFXAPI GetCapture() { return FromHandle(::GetCapture()); }
	CWnd *SetCapture() { return FromHandle(::SetCapture(m_hWnd)); }

	static CWnd * AFXAPI GetFocus() { return FromHandle(::GetFocus()); }
	CWnd *SetFocus() { return FromHandle(::SetFocus(m_hWnd)); }

	BOOL Show(int nCmdShow = SW_SHOWNORMAL) { return :: ShowWindow(m_hWnd, nCmdShow); }
	void Update() { Win32Check(::UpdateWindow(m_hWnd)); }

	void DrawMenuBar() { Win32Check(::DrawMenuBar(m_hWnd)); }
	void HideCaret() { Win32Check(::HideCaret(m_hWnd)); }
	void ShowCaret() { Win32Check(::ShowCaret(m_hWnd)); }
	void InvalidateRect(LPCRECT lpRect = 0, bool bErase = true) { Win32Check(::InvalidateRect(m_hWnd, lpRect, bErase)); }

	void Invalidate(bool bErase = true);
	bool IsChild(const CWnd *pWnd) const;
	void Move(int x, int y, int nWidth, int nHeight, bool bRepaint = true);
	void Move(const Rectangle& rect, bool bRepaint = true);
	int Scroll(int dx, int dy, LPCRECT lpRectScroll = 0, LPCRECT lpRectClip = 0, HRGN hRgn = 0,
		LPRECT lpRectUpdate = 0, UINT flags = SW_INVALIDATE);
	void ShowScrollBar(int wBar, bool bShow = true);
	void FilterToolTipMessage(MSG* pMsg);
	static void AFXAPI CancelToolTips(bool bKeys = false);
	static void AFXAPI _FilterToolTipMessage(MSG* pMsg, CWnd* pWnd);
	SCROLLINFO GetScrollInfo(int fnBar, UINT fMask = SIF_ALL);
	int SetScrollInfo(int fnBar, const SCROLLINFO& si, bool fRedraw = true);
	void SubclassWindow(HWND hWnd);
	void SubclassDlgItem(UINT nID, CWnd *pParent);
	void AttachControlSite(CWnd *pWndParent);
	HWND UnsubclassWindow();
	void SetRedraw(bool bRedraw = true);
#if UCFG_EXTENDED
	void Redraw(LPCRECT lpRectUpdate = 0, CRgn *prgnUpdate = 0, UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	void DoSuperclassPaint(CDC *pDC, const Rectangle& rcBounds);
	CFrameWnd* GetParentFrame() const;
	CFrameWnd *GetTopLevelFrame() const;
	bool EnableToolTips(bool bEnable = true, UINT nFlag = WF_TOOLTIPS);
protected:
	afx_msg BOOL OnEraseBkgnd(CDC*)                         {return (BOOL)Default();}
	afx_msg void OnIconEraseBkgnd(CDC*)                     {Default();}
	afx_msg LRESULT OnMenuChar(UINT, UINT, CMenu*)          {return Default();}
	afx_msg int OnCharToItem(UINT nChar, CListBox* pListBox, UINT nIndex);
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	afx_msg void OnInitMenu(CMenu*)                         {Default();}
	afx_msg void OnInitMenuPopup(CMenu*, UINT, BOOL)        {Default();}
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg int OnCompareItem(int nIDCtl, LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	afx_msg void OnDeleteItem(int nIDCtl, LPDELETEITEMSTRUCT lpDeleteItemStruct);
	virtual bool OnWndMsg(const MSG& msg, LRESULT *pResult);
	virtual bool OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	bool ReflectChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

public:
	static bool AFXAPI ReflectLastMsg(HWND hWndChild, LRESULT* pResult = NULL);
	CFont *get_Font() const;
	void put_Font(CFont *font, bool bRedraw = true);
	DEFPROP_CONST(CFont*, Font);

#if !UCFG_WCE
	CMenu *GetSystemMenu(bool bRevert) const;
	void AttachControlSite();
	CWnd *GetDescendant(int nID, bool bOnlyPerm = false) const;

	CMenu *get_Menu() const;
	void put_Menu(CMenu *pMenu);
	DEFPROP_CONST(CMenu*, Menu);
	void SendMessageToDescendants(UINT message, WPARAM wParam = 0, LPARAM lParam = 0, BOOL bDeep = TRUE, BOOL bOnlyPerm = FALSE);
protected:
	afx_msg BOOL OnHelpInfo(HELPINFO *pHI);
public:
#endif

#endif
	void SetPos(const CWnd *pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags);

	Point PointToClient(const POINT& point) const {
		Point r = point;
		Win32Check(::ScreenToClient(m_hWnd, &r));
		return r;
	}

	Point PointToScreen(const POINT& point) const {
		Point r = point;
		Win32Check(::ClientToScreen(m_hWnd, &r));
		return r;
	}

	Rectangle RectangleToClient(const RECT& rect) const {
		Point a(rect.left, rect.top), b(rect.right, rect.bottom);
		Win32Check(::ScreenToClient(m_hWnd, &a));
		Win32Check(::ScreenToClient(m_hWnd, &b));
		return Rectangle::FromLTRB(a.x, a.y, b.x, b.y);
	}

	Rectangle RectangleToScreen(const RECT& rect) const {
		Point a(rect.left, rect.top), b(rect.right, rect.bottom);
		Win32Check(::ClientToScreen(m_hWnd, &a));
		Win32Check(::ClientToScreen(m_hWnd, &b));
		return Rectangle::FromLTRB(a.x, a.y, b.x, b.y);
	}

	void GetClientRect(LPRECT lpRect) { //!!!comp
		*lpRect = ClientRect;
	}

	Rectangle get_ClientRect() const;
	DEFPROP_GET(Rectangle, ClientRect);

	Rectangle get_WindowRect() const;
	DEFPROP_GET(Rectangle, WindowRect);

	int get_DlgCtrlID() const;
	int put_DlgCtrlID(int nID);
	DEFPROP_CONST(int, DlgCtrlID);

	CWnd *GetDlgItem(int nID) const;
	void GetDlgItem(int nID, HWND *phWnd) const;

	bool UpdateData(bool bSaveAndValidate = true);
	void SetHandle(HANDLE h);
	bool ExecuteDlgInit(const CResID& resID);
	bool ExecuteDlgInit(void *lpResource);
	bool IsTopParentActive() const;
	CWnd *GetNextWindow(UINT nFlag = GW_HWNDNEXT) const;
	int TranslateAccelerator(HACCEL hAccel, MSG& msg);
	static CWnd * AFXAPI Find(RCString className = nullptr, RCString windowName = nullptr);
	static bool AFXAPI WalkPreTranslateTree(HWND hWndStop, MSG* pMsg);
	static HWND AFXAPI GetSafeOwner_(HWND hParent, HWND* pWndTop);
	bool PreTranslateInput(LPMSG lpMsg);
	bool IsDialogMessage(LPMSG lpMsg);
	CWnd *GetTopLevelParent() const;
	CWnd *GetParentOwner() const;
	bool ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
	bool ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
#if UCFG_COM
	void InvokeHelper(DISPID dwDispID, WORD wFlags, VARTYPE vtRet, void* pvRet, const BYTE* pbParamInfo, ...);
#endif
	int MessageBox(RCString text, RCString caption = nullptr, UINT nType = MB_OK);

	String get_Text();
	void put_Text(const String& s);
	DEFPROP(String, Text);

	CWnd *get_Parent() const;
	CWnd *put_Parent(CWnd *pWndNewParent);
	DEFPROP_CONST(CWnd*, Parent);

	CWnd *get_Owner() const;
	void put_Owner(CWnd *pWnd);
	DEFPROP_CONST(CWnd*, Owner);

	CWnd *GetWindow(UINT nCmd) const;
	CWnd *GetNextDlgTabItem(CWnd* pWndCtl, bool bPrevious = false) const;
	static CWnd * AFXAPI FromHandle(HWND hWnd);
	static CWnd * AFXAPI FromHandlePermanent(HWND hWnd);

	static void AFXAPI DeleteTempMap() {
#if UCFG_WND
		AfxGetModuleThreadState()->GetHandleMaps().m_mapHWND.DeleteTemp();
#endif
	}

	LONG GetLong(int nIndex) const;
	LONG SetLong(int nIndex, LONG dw);

	LONG_PTR GetLongPtr(int nIndex) const;
	LONG_PTR SetLongPtr(int nIndex, LONG_PTR dw);

#if UCFG_WCE
	WNDPROC GetWndProc() { return (WNDPROC)GetLong(GWL_WNDPROC); }
#else
	WNDPROC GetWndProc() { return (WNDPROC)GetLongPtr(GWLP_WNDPROC); }
#endif

	WNDPROC SetWndProc(WNDPROC wndproc);

	EXT_API LRESULT Send(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;

	EXT_API LRESULT Send(UINT message, WPARAM wParam, const void* p) const {
		return Send(message, wParam, (LPARAM)p);
	}

	//!!!O
	EXT_API LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const {
		return Send(message, wParam, lParam);
	}

	EXT_API void PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;
	EXT_API void SendNotifyMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0) const;
	bool SendChildNotifyLastMsg(LRESULT *pResult = 0);
	virtual void PreSubclassWindow();
	HICON GetIcon(bool bBigIcon) const;
	HICON SetIcon(HICON hIcon, bool bBigIcon);
	virtual BOOL Create(RCString lpszClassName, RCString lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = 0);
	virtual void Destroy();
	EXT_API virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual bool PreTranslateMessage(MSG* pMsg);

	DECLARE_DYNCREATE(CWnd)
	DECLARE_MESSAGE_MAP()
public:

#if UCFG_GUI
	virtual void OnDraw(CDC& dc, const Rectangle& rcBounds, const Rectangle& rcInvalid);
	virtual void CalcWindowRect(RECT *lpClientRect, UINT nAdjustType = adjustBorder);
#if !UCFG_WCE
	virtual void OnActivateView(bool bActivate, CWnd *pActivateView, CWnd *pDeactiveView);
#endif
	CDocument* GetDocument() const {return m_pDocument;}
#endif
//!!!R	bool Enable(bool bEnable = true) { return ::EnableWindow(m_hWnd, bEnable); }


#if !UCFG_WCE

protected:
	afx_msg void OnGetMinMaxInfo(MINMAXINFO*)               {Default();}
	afx_msg void OnDropFiles(HDROP)                         {Default();}
	afx_msg void OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS*)     {Default();}
public:
	bool get_AllowDrop() { return m_bAllowDrop; }
	void put_AllowDrop(bool v) { ::DragAcceptFiles(m_hWnd, bool(m_bAllowDrop=v)); }
	DEFPROP(bool, AllowDrop);

	void ShowOwnedPopups(bool bShow = true) { Win32Check(::ShowOwnedPopups(m_hWnd, bShow)); }
	bool IsZoomed() const  { return ::IsZoomed(m_hWnd); }
	bool IsIconic() const  { return ::IsIconic(m_hWnd); }
	bool IsUnicode() const { return ::IsWindowUnicode(m_hWnd); }

#if UCFG_GUI
	EXT_API virtual int OnToolHitTest(Point point, TOOLINFO* pTI) const;
#endif
	void EnableScrollBar(UINT wSBflags, UINT wArrows = ESB_ENABLE_BOTH);
	bool Flash(bool bInvert = true) { return ::FlashWindow(m_hWnd, bInvert); }

	CWnd *get_LastActivePopup() const { return FromHandle(::GetLastActivePopup(m_hWnd)); }
	DEFPROP_GET(CWnd*, LastActivePopup);

	CWnd *GetTopWindow() const { return FromHandle(::GetTopWindow(m_hWnd)); }

	WINDOWPLACEMENT get_Placement() const;
	void put_Placement(const WINDOWPLACEMENT& wndpl);
	DEFPROP_CONST(WINDOWPLACEMENT, Placement);

	WINDOWINFO get_Info();
	DEFPROP_GET(WINDOWINFO, Info);
#endif

private:
	CWnd(HWND hWnd);  // just for special initialization
	void CommonConstructor();
	//!!!  LRESULT DispatchEntry(const AFX_MSGMAP_ENTRY *lpEntry, const MSG& msg);
protected:

#if !UCFG_WCE && UCFG_OLE
	CComPtr<IOleContainer> m_iCtrlCont;
	observer_ptr<COleControlContainer> m_pCtrlCont;
	observer_ptr<COleControlSite> m_pCtrlSite;
#endif


	WNDPROC m_pfnSuper;
	String m_className;
	CBool m_bAllowDrop;

	virtual HWND get_Handle() const { return m_hWnd; }

	EXT_API LRESULT DefWindowProc(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	void InitControlContainer();
	void CreateDlg(const CResID& resID, CWnd *pParentWnd);
	void CreateDlgIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd, void *lpDialogInit, HINSTANCE hInst = 0);

	virtual void OnHandleCreated() {}

	// Default message map implementations
	afx_msg void OnActivateApp(BOOL, HTASK)                 {Default();}
	afx_msg void OnActivate(UINT, CWnd*, BOOL)              {Default();}
	afx_msg void OnCancelMode()                             {Default();}
	afx_msg void OnChildActivate()                          {Default();}
	afx_msg void OnClose()                                  {Default();}
	afx_msg void OnContextMenu(CWnd*, Point)               {Default();}
	afx_msg int OnCopyData(CWnd*, COPYDATASTRUCT*)          {return (int)Default();}
	EXT_API afx_msg int OnCreate(LPCREATESTRUCT);
	afx_msg void OnEnable(BOOL)                             {Default();}
	afx_msg void OnEndSession(BOOL)                         {Default();}

	afx_msg void OnKillFocus(CWnd*)                         {Default();}
	afx_msg void OnMenuSelect(UINT, UINT, HMENU)            {Default();}
	afx_msg void OnMove(int, int)                           {Default();}
	afx_msg void OnPaint()                                  {Default();}
	afx_msg HCURSOR OnQueryDragIcon()                       {return (HCURSOR)Default();}
	afx_msg BOOL OnQueryEndSession()                        {return (BOOL)Default();}
	afx_msg BOOL OnQueryNewPalette()                        {return (BOOL)Default();}
	afx_msg BOOL OnQueryOpen()                              {return (BOOL)Default();}
	afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT)             {return (BOOL)Default();}
	afx_msg void OnSetFocus(CWnd*)                          {Default();}
	afx_msg void OnShowWindow(BOOL, UINT)                   {Default();}
	afx_msg void OnSize(UINT, int, int)                     {Default();}
	afx_msg void OnTCard(UINT, DWORD)                       {Default();}
	afx_msg void OnWindowPosChanging(WINDOWPOS*)            {Default();}
	afx_msg void OnWindowPosChanged(WINDOWPOS*)             {Default();}
	afx_msg void OnPaletteIsChanging(CWnd*)                 {Default();}
	afx_msg BOOL OnNcActivate(BOOL)                         {return (BOOL)Default();}
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT)                 {return (BOOL)Default();}
	afx_msg UINT OnNcHitTest(Point)                        {return (UINT)Default();}
	afx_msg void OnNcLButtonDblClk(UINT, Point)            {Default();}
	afx_msg void OnNcLButtonDown(UINT, Point)              {Default();}
	afx_msg void OnNcLButtonUp(UINT, Point)                {Default();}
	afx_msg void OnNcMButtonDblClk(UINT, Point)            {Default();}
	afx_msg void OnNcMButtonDown(UINT, Point)              {Default();}
	afx_msg void OnNcMButtonUp(UINT, Point)                {Default();}
	afx_msg void OnNcMouseMove(UINT, Point)                {Default();}
	afx_msg void OnNcPaint()                                {Default();}
	afx_msg void OnNcRButtonDblClk(UINT, Point)            {Default();}
	afx_msg void OnNcRButtonDown(UINT, Point)              {Default();}
	afx_msg void OnNcRButtonUp(UINT, Point)                {Default();}
	afx_msg void OnSysChar(UINT, UINT, UINT)                {Default();}
	afx_msg void OnSysCommand(UINT, LPARAM)                 {Default();}
	afx_msg void OnSysDeadChar(UINT, UINT, UINT)            {Default();}
	afx_msg void OnSysKeyDown(UINT, UINT, UINT)             {Default();}
	afx_msg void OnSysKeyUp(UINT, UINT, UINT)               {Default();}
	afx_msg void OnCompacting(UINT)                         {Default();}
	afx_msg void OnFontChange()                             {Default();}
	afx_msg void OnPaletteChanged(CWnd*)                    {Default();}
	afx_msg void OnSpoolerStatus(UINT, UINT)                {Default();}
	afx_msg void OnTimeChange()                             {Default();}
	afx_msg void OnChar(UINT, UINT, UINT)                   {Default();}
	afx_msg void OnDeadChar(UINT, UINT, UINT)               {Default();}
	afx_msg void OnKeyDown(UINT, UINT, UINT)                {Default();}
	afx_msg void OnKeyUp(UINT, UINT, UINT)                  {Default();}
	afx_msg void OnLButtonDblClk(UINT, Point)              {Default();}
	afx_msg void OnLButtonDown(UINT, Point)                {Default();}
	afx_msg void OnLButtonUp(UINT, Point)                  {Default();}
	afx_msg void OnMButtonDblClk(UINT, Point)              {Default();}
	afx_msg void OnMButtonDown(UINT, Point)                {Default();}
	afx_msg void OnMButtonUp(UINT, Point)                  {Default();}
	afx_msg void OnMouseMove(UINT, Point)                  {Default();}
	afx_msg BOOL OnMouseWheel(UINT, short, Point)          {return (BOOL)Default();}
	afx_msg LRESULT OnRegisteredMouseWheel(WPARAM, LPARAM)  {return Default();}
	afx_msg void OnRButtonDblClk(UINT, Point)              {Default();}
	afx_msg void OnRButtonDown(UINT, Point)                {Default();}
	afx_msg void OnRButtonUp(UINT, Point)                  {Default();}
	afx_msg void OnTimer(UINT_PTR)                              {Default();}
	afx_msg void OnAskCbFormatName(UINT, LPTSTR)            {Default();}
	afx_msg void OnChangeCbChain(HWND, HWND)                {Default();}
	afx_msg void OnDestroyClipboard()                       {Default();}
	afx_msg void OnDrawClipboard()                          {Default();}
	afx_msg void OnHScrollClipboard(CWnd*, UINT, UINT)      {Default();}
	afx_msg void OnPaintClipboard(CWnd*, HGLOBAL)           {Default();}
	afx_msg void OnRenderAllFormats()                       {Default();}
	afx_msg void OnRenderFormat(UINT)                       {Default();}
	afx_msg void OnSizeClipboard(CWnd*, HGLOBAL)            {Default();}
	afx_msg void OnVScrollClipboard(CWnd*, UINT, UINT)      {Default();}
	afx_msg UINT OnGetDlgCode()                             { return (UINT)Default(); }
	afx_msg void OnMDIActivate(BOOL, CWnd*, CWnd*)          {Default();}
	afx_msg void OnEnterMenuLoop(BOOL)                      {Default();}
	afx_msg void OnExitMenuLoop(BOOL)                       {Default();}
	afx_msg void OnStyleChanged(int, LPSTYLESTRUCT)         {Default();}
	afx_msg void OnStyleChanging(int, LPSTYLESTRUCT)        {Default();}
	afx_msg void OnSizing(UINT, LPRECT)                     {Default();}
	afx_msg void OnMoving(UINT, LPRECT)                     {Default();}
	afx_msg void OnCaptureChanged(CWnd*)                    {Default();}
	afx_msg BOOL OnDeviceChange(UINT, DWORD)                {return (BOOL)Default();}

	//!!!D  afx_msg int OnMouseActivate(CWnd*, UINT, UINT)          {return (int)Default();}
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);

	afx_msg void OnEnterIdle(UINT nWhy, CWnd* pWho);
	afx_msg LRESULT OnNTCtlColor(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg void OnNcDestroy();

	static CWnd * AFXAPI GetSafeOwner(CWnd* pParent, HWND* pWndTop);
	virtual WNDPROC *GetSuperWndProcAddr();
	EXT_API virtual LRESULT DefWindowProc(const MSG& msg);
	virtual LRESULT WindowProc(const MSG& msg);
	virtual void PostNcDestroy();

#if UCFG_GUI
	observer_ptr<CDocument>  m_pDocument;

	virtual bool OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
#if !UCFG_WCE
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
#endif
	virtual void DoDataExchange(CDataExchange *pDX);
	virtual void OnActivateFrame(UINT nState, CFrameWnd* pFrameWnd) {}
#endif

	friend class CFrameWnd;
	friend class COccManager;
	friend class COleControlSite;
	friend class COleControlContainer;
	friend LRESULT AfxCallWndProc(CWnd*, const MSG& msg);
	friend LRESULT CALLBACK _AfxCbtFilterHook(int, WPARAM, LPARAM);
};

AFX_API String AFXAPI AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor = 0, HBRUSH hbrBackground = 0, HICON hIcon = 0);
AFX_API void AFXAPI AfxPostQuitMessage(int nExitCode);

class CWindowNotifier : protected CWnd
#if	UCFG_THREAD_MANAGEMENT
	, public Thread
#endif
{
#if	UCFG_THREAD_MANAGEMENT
public:
	typedef Thread::interlocked_policy interlocked_policy;
	using Thread::m_aRef;
	void Stop() override;
protected:
	void Execute() override;
#endif
public:
	typedef void (__stdcall *PFNNotify)(const MSG& msg);

	typedef std::unordered_set<PFNNotify> Sinks;
	Sinks m_sinks;

	CWindowNotifier();
	LRESULT WindowProc(const MSG& msg) override;
protected:
	std::mutex m_mtxDestory;
};


typedef void (AFX_MSG_CALL CWnd::*AFX_PMSGW)(void);
// like 'AFX_PMSG' but for CWnd derived classes only

typedef void (AFX_MSG_CALL CWinThread::*AFX_PMSGT)(void);
// like 'AFX_PMSG' but for CWinThread-derived classes only

#if !UCFG_WCE
AFX_API void AFXAPI AfxHookWindowCreate(CWnd *pWnd);
AFX_API bool AFXAPI AfxUnhookWindowCreate();
#endif

AFX_API CWnd * AFXAPI AfxGetMainWnd();

#undef AFX_DATA
#define AFX_DATA



//!!!extern EXT_DATA CCriticalSection g_criticalSection;


const int E_CDERR = 0x80CD0000;

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

#define AFX_WND_REG                     0x00001
#define AFX_WNDCONTROLBAR_REG           0x00002
#define AFX_WNDMDIFRAME_REG             0x00004
#define AFX_WNDFRAMEORVIEW_REG          0x00008
#define AFX_WNDCOMMCTLS_REG             0x00010 // means all original Win95
#define AFX_WNDOLECONTROL_REG           0x00020
#define AFX_WNDCOMMCTL_UPDOWN_REG       0x00040 // these are original Win95
#define AFX_WNDCOMMCTL_TREEVIEW_REG     0x00080
#define AFX_WNDCOMMCTL_TAB_REG          0x00100
#define AFX_WNDCOMMCTL_PROGRESS_REG     0x00200
#define AFX_WNDCOMMCTL_LISTVIEW_REG     0x00400
#define AFX_WNDCOMMCTL_HOTKEY_REG       0x00800
#define AFX_WNDCOMMCTL_BAR_REG          0x01000
#define AFX_WNDCOMMCTL_ANIMATE_REG      0x02000
#define AFX_WNDCOMMCTL_INTERNET_REG     0x04000 // these are new in IE4
#define AFX_WNDCOMMCTL_COOL_REG         0x08000
#define AFX_WNDCOMMCTL_USEREX_REG       0x10000
#define AFX_WNDCOMMCTL_DATE_REG         0x20000

#define AFX_WIN95CTLS_MASK              0x03FC0 // UPDOWN -> ANIMATE
#define AFX_WNDCOMMCTLSALL_REG          0x3C010 // COMMCTLS|INTERNET|COOL|USEREX|DATE
#define AFX_WNDCOMMCTLSNEW_REG          0x3C000 // INTERNET|COOL|USEREX|DATE

#ifndef ASSERT_VALID
#	define ASSERT_VALID(pOb)  ((void)0)
#endif

struct AFX_NOTIFY {
	LRESULT* pResult;
	NMHDR* pNMHDR;
};

// WM_CTLCOLOR for 16 bit API compatability
#define WM_CTLCOLOR     0x0019

/////////////////////////////////////////////////////////////////////////////
// Internal AFX Windows messages (see Technical note TN024 for more details)
// (0x0360 - 0x037F are reserved for MFC)

#define WM_QUERYAFXWNDPROC  0x0360  // lResult = 1 if processed by AfxWndProc
#define WM_SIZEPARENT       0x0361  // lParam = &AFX_SIZEPARENTPARAMS
#define WM_SETMESSAGESTRING 0x0362  // wParam = nIDS (or 0),
// lParam = lpszOther (or NULL)
#define WM_IDLEUPDATECMDUI  0x0363  // wParam == bDisableIfNoHandler
#define WM_INITIALUPDATE    0x0364  // (params unused) - sent to children
#define WM_COMMANDHELP      0x0365  // lResult = TRUE/FALSE,
// lParam = dwContext
#define WM_HELPHITTEST      0x0366  // lResult = dwContext,
// lParam = MAKELONG(x, y)
#define WM_EXITHELPMODE     0x0367  // (params unused)
#define WM_RECALCPARENT     0x0368  // force RecalcLayout on frame window
//  (only for inplace frame windows)
#define WM_SIZECHILD        0x0369  // special notify from COleResizeBar
// wParam = ID of child window
// lParam = lpRectNew (new position/size)
#define WM_KICKIDLE         0x036A  // (params unused) causes idles to kick in
#define WM_QUERYCENTERWND   0x036B  // lParam = HWND to use as centering parent
#define WM_DISABLEMODAL     0x036C  // lResult = 0, disable during modal state
// lResult = 1, don't disable
#define WM_FLOATSTATUS      0x036D  // wParam combination of FS_* flags below

// WM_ACTIVATETOPLEVEL is like WM_ACTIVATEAPP but works with hierarchies
//   of mixed processes (as is the case with OLE in-place activation)
#define WM_ACTIVATETOPLEVEL 0x036E  // wParam = nState (like WM_ACTIVATE)
// lParam = pointer to HWND[2]
//  lParam[0] = hWnd getting WM_ACTIVATE
//  lParam[1] = hWndOther

#define WM_QUERY3DCONTROLS  0x036F  // lResult != 0 if 3D controls wanted

// Note: Messages 0x0370, 0x0371, and 0x372 were incorrectly used by
//  some versions of Windows.  To remain compatible, MFC does not
//  use messages in that range.
#define WM_RESERVED_0370    0x0370
#define WM_RESERVED_0371    0x0371
#define WM_RESERVED_0372    0x0372

// WM_SOCKET_NOTIFY and WM_SOCKET_DEAD are used internally by MFC's
// Windows sockets implementation.  For more information, see sockcore.cpp
#define WM_SOCKET_NOTIFY    0x0373
#define WM_SOCKET_DEAD      0x0374

// same as WM_SETMESSAGESTRING except not popped if IsTracking()
#define WM_POPMESSAGESTRING 0x0375

// WM_HELPPROMPTADDR is used internally to get the address of
//  m_dwPromptContext from the associated frame window. This is used
//  during message boxes to setup for F1 help while that msg box is
//  displayed. lResult is the address of m_dwPromptContext.
#define WM_HELPPROMPTADDR   0x0376

// Constants used in DLGINIT resources for OLE control containers
// NOTE: These are NOT real Windows messages they are simply tags
// used in the control resource and are never used as 'messages'
#define WM_OCC_LOADFROMSTREAM           0x0376
#define WM_OCC_LOADFROMSTORAGE          0x0377
#define WM_OCC_INITNEW                  0x0378
#define WM_OCC_LOADFROMSTREAM_EX        0x037A
#define WM_OCC_LOADFROMSTORAGE_EX       0x037B

// Marker used while rearranging the message queue
#define WM_QUEUE_SENTINEL   0x0379

// Note: Messages 0x037C - 0x37E reserved for future MFC use.
#define WM_RESERVED_037C    0x037C
#define WM_RESERVED_037D    0x037D
#define WM_RESERVED_037E    0x037E

// WM_FORWARDMSG - used by ATL to forward a message to another window for processing
//  WPARAM - DWORD dwUserData - defined by user
//  LPARAM - LPMSG pMsg - a pointer to the MSG structure
//  return value - 0 if the message was not processed, nonzero if it was
#define WM_FORWARDMSG       0x037F

// like ON_MESSAGE but no return value
#define ON_MESSAGE_VOID(message, memberFxn) \
{ message, 0, 0, 0, AfxSig_vv, \
	(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(void))&memberFxn },


typedef bool (*PFN_AfxDispatchCmdMsg)(CCmdTarget *pTarget, UINT nID, int nCode, AFX_PMSG pfn, void* pExtra, UINT_PTR nSig, AFX_CMDHANDLERINFO *pHandlerInfo);
extern EXT_DATA PFN_AfxDispatchCmdMsg g_pfnAfxDispatchCmdMsg;

struct CGuiHooks {
	typedef int (AFXAPI *PFN_OnToolHitTest)(HWND hWnd, CPoint point, TOOLINFO* pTI);
	PFN_OnToolHitTest OnToolHitTest;

	typedef bool (AFXAPI *PFN_OnCommand)(CWnd *pWnd, WPARAM wParam, LPARAM lParam);
	PFN_OnCommand OnCommand;

	typedef int (AFXAPI *PFN_OnMouseActivate)(CWnd *pWnd, CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	PFN_OnMouseActivate OnMouseActivate;

	typedef void (AFXAPI *PFN_OnMeasureItem)(CWnd *pWnd, int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	PFN_OnMeasureItem OnMeasureItem;
};

extern EXT_DATA CGuiHooks g_guiHooks;

class CSuspendRedraw {		//!!!
	CWnd& m_wnd;

	CSuspendRedraw(CSuspendRedraw&) = delete;
public:
	CSuspendRedraw(CWnd& wnd)
		: m_wnd(wnd)
	{
		m_wnd.SetRedraw(false);
	}

	~CSuspendRedraw() {
		m_wnd.SetRedraw();
	}
};

class AFX_CLASS CTimer {
	observer_ptr<CWnd> m_pWnd;
public:
	UINT_PTR m_ID;

	CTimer();
	virtual ~CTimer();
	void Set(CWnd *pWnd = 0, UINT_PTR nID = 1, UINT uElapse = 1000, TIMERPROC lpTimerFunc = 0);
	virtual void OnTimer();
	void SetEx();
	void Kill();
};



} // Ext::

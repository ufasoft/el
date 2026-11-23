namespace Ext {

/////////////////////////////////////////////////////////////////////////////
// useful message ranges

#define WM_SYSKEYFIRST  WM_SYSKEYDOWN
#define WM_SYSKEYLAST   WM_SYSDEADCHAR

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  WM_NCMBUTTONDBLCLK

union MessageMapFunctions
{
	AFX_PMSG pfn;   // generic member function pointer

#if UCFG_EXTENDED
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_b_D)(CDC*);
	HBRUSH (AFX_MSG_CALL CCmdTarget::*pfn_B_D_W_u)(CDC*, CWnd*, UINT);
	HBRUSH (AFX_MSG_CALL CCmdTarget::*pfn_B_D_u)(CDC*, UINT);
	LRESULT (AFX_MSG_CALL CWnd::*pfn_l_u_u_M)(UINT, UINT, Gui::Menu*);
	void (AFX_MSG_CALL CWnd::*pfn_v_D)(CDC*);
	void (AFX_MSG_CALL CWnd::*pfn_v_M)(Gui::Menu*);
	void (AFX_MSG_CALL CWnd::*pfn_v_M_u_b)(Gui::Menu*, UINT, BOOL);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_u_M)(UINT, UINT, Gui::Menu*);
	void (AFX_MSG_CALL CCmdTarget::*pfnCmdUI_v_C)(CCmdUI*);
	void (AFX_MSG_CALL CCmdTarget::*pfnCmdUI_v_C_u)(CCmdUI*, UINT);
#endif

	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_b_b)(BOOL);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_b_u)(UINT);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_b_h)(HANDLE);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_b_W_u_u)(CWnd*, UINT, UINT);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_b_W_COPYDATASTRUCT)(CWnd*, COPYDATASTRUCT*);
	int (AFX_MSG_CALL CCmdTarget::*pfn_i_u_W_u)(UINT, CWnd*, UINT);
	int (AFX_MSG_CALL CCmdTarget::*pfn_i_u_u)(UINT, UINT);
	int (AFX_MSG_CALL CCmdTarget::*pfn_i_W_u_u)(CWnd*, UINT, UINT);
	int (AFX_MSG_CALL CWnd::*pfn_i_s)(LPTSTR);
	LRESULT (AFX_MSG_CALL CWnd::*pfn_l_w_l)(WPARAM, LPARAM);
	void (AFX_MSG_CALL CWnd::*pfn_v_b_h)(BOOL, HANDLE);
	void (AFX_MSG_CALL CWnd::*pfn_v_h)(HANDLE);
	void (AFX_MSG_CALL CWnd::*pfn_v_h_h)(HANDLE,HANDLE);
	void (AFX_MSG_CALL CWnd::*pfn_v_v)();
	int (AFX_MSG_CALL CWnd::*pfn_i_u)(UINT);
	HCURSOR (AFX_MSG_CALL CWnd::*pfn_C_v)();
	UINT (AFX_MSG_CALL CWnd::*pfn_u_u)(UINT);
	BOOL (AFX_MSG_CALL CWnd::*pfn_b_v)();
	void (AFX_MSG_CALL CWnd::*pfn_v_u)(UINT);
	void (AFX_MSG_CALL CWnd::*pfn_v_up)(UINT_PTR);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_u)(UINT, UINT);
	void (AFX_MSG_CALL CWnd::*pfn_v_i_i)(int, int);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_u_u)(UINT, UINT, UINT);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_i_i)(UINT, int, int);
	void (AFX_MSG_CALL CWnd::*pfn_v_w_l)(WPARAM, LPARAM);
	void (AFX_MSG_CALL CWnd::*pfn_v_b_W_W)(BOOL, CWnd*, CWnd*);
	void (AFX_MSG_CALL CWnd::*pfn_v_W)(CWnd*);
	void (AFX_MSG_CALL CWnd::*pfn_v_W_u_u)(CWnd*, UINT, UINT);
	void (AFX_MSG_CALL CWnd::*pfn_v_W_p)(CWnd*, Point);
	void (AFX_MSG_CALL CWnd::*pfn_v_W_h)(CWnd*, HANDLE);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_W)(UINT, CWnd*);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_W_b)(UINT, CWnd*, BOOL);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_u_W)(UINT, UINT, CWnd*);
	void (AFX_MSG_CALL CWnd::*pfn_v_s)(LPTSTR);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_cs)(UINT, LPCTSTR);
	void (AFX_MSG_CALL CWnd::*pfn_v_i_s)(int, LPTSTR);
	int (AFX_MSG_CALL CWnd::*pfn_i_i_s)(int, LPTSTR);
	UINT (AFX_MSG_CALL CWnd::*pfn_u_p)(Point);
	LRESULT (AFX_MSG_CALL CWnd::*pfn_l_p)(Point);
	UINT (AFX_MSG_CALL CWnd::*pfn_u_v)();
	void (AFX_MSG_CALL CWnd::*pfn_v_v_WINDOWPOS)(WINDOWPOS*);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_p)(UINT, Point);
	void (AFX_MSG_CALL CWnd::*pfn_v_u_pr)(UINT, LPRECT);
	BOOL (AFX_MSG_CALL CWnd::*pfn_b_u_s_p)(UINT, short, Point);
	LRESULT (AFX_MSG_CALL CWnd::*pfn_l_v)();

	// type safe variant for thread messages
	void (AFX_MSG_CALL CWinThread::*pfn_THREAD)(WPARAM, LPARAM);

	// specific type safe variants for WM_COMMAND and WM_NOTIFY messages
	void (AFX_MSG_CALL CCmdTarget::*pfnCmd_v_v)();
	BOOL (AFX_MSG_CALL CCmdTarget::*pfnCmd_b_v)();
	void (AFX_MSG_CALL CCmdTarget::*pfnCmd_v_u)(UINT);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfnCmd_b_u)(UINT);

	void (AFX_MSG_CALL CCmdTarget::*pfnNotify_v_NMHDR_pl)(NMHDR*, LRESULT*);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfnNotify_b_NMHDR_pl)(NMHDR*, LRESULT*);
	void (AFX_MSG_CALL CCmdTarget::*pfnNotify_v_u_NMHDR_pl)(UINT, NMHDR*, LRESULT*);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfnNotify_b_u_NMHDR_pl)(UINT, NMHDR*, LRESULT*);

	void (AFX_MSG_CALL CCmdTarget::*pfnCmd_v_pv)(void*);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfnCmd_b_pv)(void*);


	//OLD
	// specific type safe variants for WM-style messages
	//	BOOL    (AFX_MSG_CALL CWnd::*pfn_bD)(CDC*);
	//	BOOL    (AFX_MSG_CALL CWnd::*pfn_bb)(BOOL);
	//	BOOL    (AFX_MSG_CALL CWnd::*pfn_bWww)(CWnd*, UINT, UINT);
	//	BOOL    (AFX_MSG_CALL CWnd::*pfn_bHELPINFO)(HELPINFO*);
	//	BOOL    (AFX_MSG_CALL CWnd::*pfn_bWCDS)(CWnd*, COPYDATASTRUCT*);
	//	HBRUSH  (AFX_MSG_CALL CWnd::*pfn_hDWw)(CDC*, CWnd*, UINT);
	//	HBRUSH  (AFX_MSG_CALL CWnd::*pfn_hDw)(CDC*, UINT);
	//	int     (AFX_MSG_CALL CWnd::*pfn_iwWw)(UINT, CWnd*, UINT);
	//	int     (AFX_MSG_CALL CWnd::*pfn_iww)(UINT, UINT);
	//	int     (AFX_MSG_CALL CWnd::*pfn_iWww)(CWnd*, UINT, UINT);
	//	int     (AFX_MSG_CALL CWnd::*pfn_is)(LPTSTR);
	//	LRESULT (AFX_MSG_CALL CWnd::*pfn_lwl)(WPARAM, LPARAM);
	//	LRESULT (AFX_MSG_CALL CWnd::*pfn_lwwM)(UINT, UINT, CMenu*);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vv)(void);

	//	void    (AFX_MSG_CALL CWnd::*pfn_vw)(UINT);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vww)(UINT, UINT);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vvii)(int, int);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vwww)(UINT, UINT, UINT);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vwii)(UINT, int, int);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vwl)(WPARAM, LPARAM);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vbWW)(BOOL, CWnd*, CWnd*);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vD)(CDC*);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vM)(CMenu*);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vMwb)(CMenu*, UINT, BOOL);

	//	void    (AFX_MSG_CALL CWnd::*pfn_vW)(CWnd*);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vWww)(CWnd*, UINT, UINT);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vWp)(CWnd*, Point);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vWh)(CWnd*, HANDLE);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vwW)(UINT, CWnd*);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vwWb)(UINT, CWnd*, BOOL);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vwwW)(UINT, UINT, CWnd*);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vwwx)(UINT, UINT);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vs)(LPTSTR);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vOWNER)(int, LPTSTR);   // force return TRUE
	//	int     (AFX_MSG_CALL CWnd::*pfn_iis)(int, LPTSTR);
	//	UINT    (AFX_MSG_CALL CWnd::*pfn_wp)(Point);
	//	UINT    (AFX_MSG_CALL CWnd::*pfn_wv)(void);
	void    (AFX_MSG_CALL CWnd::*pfn_vPOS)(WINDOWPOS*);
	void    (AFX_MSG_CALL CWnd::*pfn_vwp)(UINT, Point);
	void    (AFX_MSG_CALL CWnd::*pfn_vwwh)(UINT, UINT, HANDLE);
	BOOL    (AFX_MSG_CALL CWnd::*pfn_bwsp)(UINT, short, Point);
	//	void    (AFX_MSG_CALL CWnd::*pfn_vws)(UINT, LPCTSTR);

#if !UCFG_WCE
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_b_HELPINFO)(LPHELPINFO);
	void (AFX_MSG_CALL CWnd::*pfn_v_b_NCCALCSIZEPARAMS)(BOOL, NCCALCSIZE_PARAMS*);
	void    (AFX_MSG_CALL CWnd::*pfn_vCALC)(BOOL, NCCALCSIZE_PARAMS*);
#else
	void (AFX_MSG_CALL CCmdTarget::*pfn_COMMAND)();
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_bCOMMAND)();
	void (AFX_MSG_CALL CCmdTarget::*pfn_COMMAND_RANGE)(UINT);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_COMMAND_EX)(UINT);
	void (AFX_MSG_CALL CCmdTarget::*pfn_UPDATE_COMMAND_UI)(CCmdUI*);
	void (AFX_MSG_CALL CCmdTarget::*pfn_UPDATE_COMMAND_UI_RANGE)(CCmdUI*, UINT);
	void (AFX_MSG_CALL CCmdTarget::*pfn_OTHER)(void*);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_OTHER_EX)(void*);

	void (AFX_MSG_CALL CCmdTarget::*pfn_NOTIFY)(NMHDR*, LRESULT*);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_bNOTIFY)(NMHDR*, LRESULT*);
	void (AFX_MSG_CALL CCmdTarget::*pfn_NOTIFY_RANGE)(UINT, NMHDR*, LRESULT*);
	BOOL (AFX_MSG_CALL CCmdTarget::*pfn_NOTIFY_EX)(UINT, NMHDR*, LRESULT*);
#endif

};
/*!!!
union MessageMapFunctions
{
AFX_PMSG pfn;   // generic member function pointer

// specific type safe variants for WM_COMMAND and WM_NOTIFY messages


// type safe variant for thread messages

void (AFX_MSG_CALL CWinThread::*pfn_THREAD)(WPARAM, LPARAM);

// specific type safe variants for WM-style messages
BOOL    (AFX_MSG_CALL CWnd::*pfn_bD)(CDC*);
BOOL    (AFX_MSG_CALL CWnd::*pfn_bb)(BOOL);
BOOL    (AFX_MSG_CALL CWnd::*pfn_bWww)(CWnd*, UINT, UINT);
BOOL    (AFX_MSG_CALL CWnd::*pfn_bHELPINFO)(HELPINFO*);
BOOL    (AFX_MSG_CALL CWnd::*pfn_bWCDS)(CWnd*, COPYDATASTRUCT*);
HBRUSH  (AFX_MSG_CALL CWnd::*pfn_hDWw)(CDC*, CWnd*, UINT);
HBRUSH  (AFX_MSG_CALL CWnd::*pfn_hDw)(CDC*, UINT);
int     (AFX_MSG_CALL CWnd::*pfn_iwWw)(UINT, CWnd*, UINT);
int     (AFX_MSG_CALL CWnd::*pfn_iww)(UINT, UINT);
int     (AFX_MSG_CALL CWnd::*pfn_iWww)(CWnd*, UINT, UINT);
int     (AFX_MSG_CALL CWnd::*pfn_is)(LPTSTR);
LRESULT (AFX_MSG_CALL CWnd::*pfn_lwl)(WPARAM, LPARAM);
LRESULT (AFX_MSG_CALL CWnd::*pfn_lwwM)(UINT, UINT, CMenu*);
void    (AFX_MSG_CALL CWnd::*pfn_vv)(void);

void    (AFX_MSG_CALL CWnd::*pfn_vw)(UINT);
void    (AFX_MSG_CALL CWnd::*pfn_vww)(UINT, UINT);
void    (AFX_MSG_CALL CWnd::*pfn_vvii)(int, int);
void    (AFX_MSG_CALL CWnd::*pfn_vwww)(UINT, UINT, UINT);
void    (AFX_MSG_CALL CWnd::*pfn_vwii)(UINT, int, int);
void    (AFX_MSG_CALL CWnd::*pfn_vwl)(WPARAM, LPARAM);
void    (AFX_MSG_CALL CWnd::*pfn_vbWW)(BOOL, CWnd*, CWnd*);
void    (AFX_MSG_CALL CWnd::*pfn_vD)(CDC*);
void    (AFX_MSG_CALL CWnd::*pfn_vM)(CMenu*);
void    (AFX_MSG_CALL CWnd::*pfn_vMwb)(CMenu*, UINT, BOOL);

void    (AFX_MSG_CALL CWnd::*pfn_vW)(CWnd*);
void    (AFX_MSG_CALL CWnd::*pfn_vWww)(CWnd*, UINT, UINT);
void    (AFX_MSG_CALL CWnd::*pfn_vWp)(CWnd*, Point);
void    (AFX_MSG_CALL CWnd::*pfn_vWh)(CWnd*, HANDLE);
void    (AFX_MSG_CALL CWnd::*pfn_vwW)(UINT, CWnd*);
void    (AFX_MSG_CALL CWnd::*pfn_vwWb)(UINT, CWnd*, BOOL);
void    (AFX_MSG_CALL CWnd::*pfn_vwwW)(UINT, UINT, CWnd*);
void    (AFX_MSG_CALL CWnd::*pfn_vwwx)(UINT, UINT);
void    (AFX_MSG_CALL CWnd::*pfn_vs)(LPTSTR);
void    (AFX_MSG_CALL CWnd::*pfn_vOWNER)(int, LPTSTR);   // force return TRUE
int     (AFX_MSG_CALL CWnd::*pfn_iis)(int, LPTSTR);
UINT    (AFX_MSG_CALL CWnd::*pfn_wp)(Point);
UINT    (AFX_MSG_CALL CWnd::*pfn_wv)(void);
void    (AFX_MSG_CALL CWnd::*pfn_vPOS)(WINDOWPOS*);
void    (AFX_MSG_CALL CWnd::*pfn_vCALC)(BOOL, NCCALCSIZE_PARAMS*);
void    (AFX_MSG_CALL CWnd::*pfn_vwp)(UINT, Point);
void    (AFX_MSG_CALL CWnd::*pfn_vwwh)(UINT, UINT, HANDLE);
BOOL    (AFX_MSG_CALL CWnd::*pfn_bwsp)(UINT, short, Point);
void    (AFX_MSG_CALL CWnd::*pfn_vws)(UINT, LPCTSTR);
};
*/



void AfxDeleteObject(HGDIOBJ* pObject);
const AFX_MSGMAP_ENTRY *AfxFindMessageEntry(const AFX_MSGMAP_ENTRY* lpEntry, UINT nMsg, UINT nCode, UINT nID);

#ifndef _DEBUG
#define _DEBUG_SUFFIX
#else
#define _DEBUG_SUFFIX _T("d")
#endif

#define AFX_WNDCLASS(s) _T("Ext") _T(s) _T("4") _DEBUG_SUFFIX

#define AFX_WND             AFX_WNDCLASS("Wnd")
#define AFX_WNDCONTROLBAR   AFX_WNDCLASS("ControlBar")
#define AFX_WNDMDIFRAME     AFX_WNDCLASS("MDIFrame")
#define AFX_WNDFRAMEORVIEW  AFX_WNDCLASS("FrameOrView")
#define AFX_WNDOLECONTROL   AFX_WNDCLASS("OleControl")

#define AFX_WIN95CTLS_MASK              0x03FC0 // UPDOWN -> ANIMATE
#define AFX_WNDCOMMCTLSALL_REG          0x3C010 // COMMCTLS|INTERNET|COOL|USEREX|DATE
#define AFX_WNDCOMMCTLSNEW_REG          0x3C000 // INTERNET|COOL|USEREX|DATE

#define CX_BORDER   1
#define CY_BORDER   1

struct AUX_DATA
{
	// system metrics
	int cxVScroll, cyHScroll;
	int cxIcon, cyIcon;

	int cxBorder2, cyBorder2;

	// device metrics for screen
	int cxPixelsPerInch, cyPixelsPerInch;

	// convenient system color
	HBRUSH hbrWindowFrame;
	HBRUSH hbrBtnFace;

	// color values of system colors used for CToolBar
	COLORREF clrBtnFace, clrBtnShadow, clrBtnHilite;
	COLORREF clrBtnText, clrWindowFrame;

	// standard cursors
	HCURSOR hcurWait;
	HCURSOR hcurArrow;
	HCURSOR hcurHelp;       // cursor used in Shift+F1 help

	// special GDI objects allocated on demand
	HFONT   hStatusFont;
	HFONT   hToolTipsFont;
//!!!R	HBITMAP hbmMenuDot;

	// other system information
	//!!!R	UINT    nWinVer;        // Major.Minor version numbers
//!!!R	BOOL    bWin95;         // TRUE if Windows 95 (not NT)
	//!!!R	BOOL    bWin4;          // TRUE if Windows 4.0
	//!!!R	BOOL    bNotWin4;       // TRUE if not Windows 4.0
	//!!!R	BOOL    bSmCaption;     // TRUE if WS_EX_SMCAPTION is supported
	//!!!R	BOOL    bMarked4;       // TRUE if marked as 4.0

	// Implementation
	AUX_DATA();
	~AUX_DATA();
	void UpdateSysColors();
	void UpdateSysMetrics();
};

class _AFX_CTL3D_STATE
{
public:
	virtual ~_AFX_CTL3D_STATE();

	// setup during initialization
	BOOL m_bCtl3dInited;
	HINSTANCE m_hCtl3dLib;

	// CTL3D32 entry points
	BOOL (WINAPI* m_pfnRegister)(HINSTANCE);
	BOOL (WINAPI* m_pfnUnregister)(HINSTANCE);
	BOOL (WINAPI* m_pfnAutoSubclass)(HINSTANCE);
	BOOL (WINAPI* m_pfnUnAutoSubclass)();
	BOOL (WINAPI* m_pfnColorChange)();
	BOOL (WINAPI* m_pfnSubclassDlgEx)(HWND, DWORD);
	void (WINAPI* m_pfnWinIniChange)();
	BOOL (WINAPI* m_pfnSubclassCtl)(HWND);
	BOOL (WINAPI* m_pfnSubclassCtlEx)(HWND, int);
};


extern AFX_GLOBAL_DATA AUX_DATA afxData;
extern AFX_GLOBAL_DATA _AFX_CTL3D_STATE _afxCtl3dState;

extern AFX_GLOBAL_DATA const String _afxWnd,           // simple child windows/controls
_afxWndControlBar, // controls with gray backgrounds
_afxWndMDIFrame,
_afxWndFrameOrView,
_afxWndOleControl;


// WM_NCDESTROY is not a supported message on WINCE, but we want to keep the logic
// (OnNcDestroy mainly) behind it... 
#ifndef WM_NCDESTROY
#	define WM_NCDESTROY            (WM_APP - 1)
#endif


} // Ext::

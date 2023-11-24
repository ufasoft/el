#pragma once

//!!!? #include <afxres.h>

#include <el/gui/gui.h>

#include <el/libext/win32/ext-wnd.h>
#include <el/libext/win32/extmfc.h>


namespace Ext {

class IPAddress;

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

class AFX_CLASS CDialogTemplate {
protected:
	void SetTemplate(const DLGTEMPLATE *pTemplate, UINT cb);
public:
	DLGTEMPLATE *m_pTemplate;

	CDialogTemplate(const DLGTEMPLATE *pTemplate = 0);
	~CDialogTemplate();
	void Load(const CResID& resID);
};

class AFX_CLASS CDialog : public CWnd {
	DECLARE_DYNCREATE(CDialog)
public:
	CResID m_templateName;

	CDialog();
	CDialog(LPCTSTR lpszTemplateName, CWnd *pParentWnd = 0);
	CDialog(UINT nIDTemplate, CWnd* pParentWnd = 0);
	~CDialog();
	void Create2(const CResID& resID, CWnd *pParentWnd = 0);
	void CreateIndirect(LPCDLGTEMPLATE lpDialogTemplate, CWnd *pParentWnd = 0, void *lpDialogInit = 0, HINSTANCE hInst = 0);
	BOOL OnCmdMsg(UINT nID, int nCode, void *pExtra, AFX_CMDHANDLERINFO *pHandlerInfo);
	virtual int DoModal();
	virtual bool ContinueModal();
	virtual void EndModalLoop(int nResult);
	void RunModalLoop(DWORD dwFlags);
	void End(int nResult);
	bool PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
protected:
	int m_nModalResult;
	observer_ptr<CWnd> m_pParentWnd;
	void *m_lpDialogInit;
#if UCFG_OLE
	observer_ptr<_AFX_OCC_DIALOG_INFO> m_pOccDialogInfo;
#endif
	UINT m_nIDHelp;                 // Help ID (0 for none, see HID_BASE_RESOURCE)

	HWND PreModal();
	void PostModal();
	virtual void OnOK();
	virtual void OnCancel();
	//!!!  LRESULT DefWindowProc(const MSG& msg);
	afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};


class AFX_CLASS CDialogQueryString : public CDialog {
protected:
	//{{AFX_VIRTUAL(CDialogQueryString)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CDialogQueryString)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
public:
	CDialogQueryString(CWnd* pParent = NULL);   // standard constructor

	//{{AFX_DATA(CDialogQueryString)
	String	m_string;
	String m_promt;
	//}}AFX_DATA

	String m_caption;

	DECLARE_MESSAGE_MAP()
};



class AFX_CLASS CScrollBar : public CWnd {
};

class CListBox : public CWnd {
	DECLARE_DYNAMIC(CListBox)
protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs);
#if UCFG_EXTENDED
	BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
#endif
public:
	CListBox() { m_className = "LISTBOX"; }

	static LONG_PTR AFXAPI Check(LRESULT r);

	void Create4(DWORD dwStyle, const RECT& rect, CWnd *pParentWnd, UINT nID);
	int AddString(RCString item);
	int InsertString(int nIndex, RCString lpszItem);
	int DeleteString(int nIndex);

	int get_Count() const { return (int)Check(SendMessage(LB_GETCOUNT)); }
	DEFPROP_GET_CONST(int, Count);

	void ResetContent(){ SendMessage(LB_RESETCONTENT); }

	DWORD_PTR GetItemData(int nIndex) const {
		return Check(SendMessage(LB_GETITEMDATA, nIndex));
	}

	int SetItemData(int nIndex, DWORD_PTR dwItemData) {
		return (int)SendMessage(LB_SETITEMDATA, nIndex, dwItemData);
	}

	int GetItemHeight(int nIndex) const;
	int SetItemHeight(int nIndex, UINT cyItemHeight);
	Rectangle GetItemRect(int nIndex) const;

	int get_CaretIndex() const;
	int put_CaretIndex(int nIndex, bool bScroll = true);
	DEFPROP_CONST(int, CaretIndex);

	int get_CurSel() const { return (int)SendMessage(LB_GETCURSEL); }
	void put_CurSel(int nIndex);
	DEFPROP_CONST(int, CurSel);

	int GetSelCount() const;
	int GetSelItems(int nMaxItems, LPINT rgIndex) const;

	bool GetSel(int nIndex) const;
	void SetSel(int nIndex, bool bSelect = true);
	//!!!  __declspec(property(get=GetSel, put=SetSel)) bool Sels[];

	int get_TopIndex() const { return (int)SendMessage(LB_GETTOPINDEX); }
	void put_TopIndex(int nIndex);
	DEFPROP_CONST(int, TopIndex);

	String GetText(int nIndex) const;
	//!!!  __declspec(property(get=GetText)) String Texts[];

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	virtual void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);
	virtual int VKeyToItem(UINT nKey, UINT nIndex);
	virtual int CharToItem(UINT nKey, UINT nIndex);
};

class AFX_CLASS CComboBox : public CWnd {
public:
	static int AFXAPI Check(LRESULT r);

	int AddString(RCString item);
	int InsertString(int nIndex, LPCTSTR lpszItem);
	int DeleteString(int nIndex);
	void ResetContent() { SendMessage(CB_RESETCONTENT); }
	String GetText(int nIndex) const;

	int get_Count() const { return Check(SendMessage(CB_GETCOUNT)); }
	DEFPROP_GET(int, Count);

	DWORD_PTR GetItemData(int nIndex) const {
		return Check(SendMessage(CB_GETITEMDATA, nIndex));
	}

	int SetItemData(int nIndex, DWORD_PTR dwItemData) {
		return (int)SendMessage(CB_SETITEMDATA, nIndex, dwItemData);
	}

	int get_CurSel() const { return (int)SendMessage(CB_GETCURSEL); }
	void put_CurSel(int nIndex);
	DEFPROP_CONST(int, CurSel);
};

class AFX_CLASS CEdit : public CWnd {
	DECLARE_DYNAMIC(CEdit)
public:
	CEdit();
	~CEdit();
	void Create4(DWORD dwStyle, const RECT& rect, CWnd *pParentWnd, UINT nID);
	int GetLineCount() const { return (int)SendMessage(EM_GETLINECOUNT); }
	bool GetModify() const { return SendMessage(EM_GETMODIFY); }
	void SetModify(bool bModified = true);
	void GetRect(LPRECT lpRect) const;
	DWORD GetSel() const;
	void GetSel(int& nStartChar, int& nEndChar) const;
	HLOCAL GetHandle() const;
	void SetHandle(HLOCAL hBuffer);
	void SetMargins(UINT nLeft, UINT nRight);
	DWORD GetMargins() const { return (DWORD)SendMessage(EM_GETMARGINS); }
	void SetLimitText(UINT nMax);
	UINT GetLimitText() const;
	Point PosFromChar(UINT nChar) const;
	int CharFromPos(const Point& pt) const;

	// NOTE: first word in lpszBuffer must contain the size of the buffer!
	int GetLine(int nIndex, LPTSTR lpszBuffer) const;
	int GetLine(int nIndex, LPTSTR lpszBuffer, int nMaxLength) const;

	// Operations

	bool FmtLines(bool bAddEOL);

	void LimitText(int nChars = 0);
	int LineFromChar(int nIndex = -1) const { return (int)SendMessage(EM_LINEFROMCHAR, nIndex); }
	int LineIndex(int nLine = -1) const;
	int LineLength(int nLine = -1) const { return (int)SendMessage(EM_LINELENGTH, nLine); }
	void LineScroll(int nLines, int nChars = 0);
	void ReplaceSel(RCString newText, bool bCanUndo = false);
	void SetRect(LPCRECT lpRect);
	void SetRectNP(LPCRECT lpRect);
	void SetSel(DWORD dwSelection, bool bNoScroll = false);
	void SetSel(int nStartChar, int nEndChar, bool bNoScroll = false);
	bool SetTabStops(int nTabStops, LPINT rgTabStops);
	void SetTabStops();
	bool SetTabStops(const int& cxEachStop);    // takes an 'int'

	bool get_CanUndo() const { return SendMessage(EM_CANUNDO); }
	DEFPROP_GET(bool, CanUndo);

	bool Undo() { return SendMessage(EM_UNDO); }
	void ClearUndo() { SendMessage(EM_EMPTYUNDOBUFFER); } // was EmptyUndoBuffer

	// Clipboard operations
	void Clear() { SendMessage(WM_CLEAR); }
	void Copy() { SendMessage(WM_COPY); }
	void Cut() { SendMessage(WM_CUT); }
	void Paste() { SendMessage(WM_PASTE); }

	bool get_ReadOnly() { return GetLong(GWL_STYLE) & ES_READONLY; }
	void put_ReadOnly(bool bReadOnly = true) { SendMessage(EM_SETREADONLY, bReadOnly); }
	DEFPROP(bool, ReadOnly);

	int GetFirstVisibleLine() const;

	TCHAR get_PasswordChar() const { return (TCHAR)SendMessage(EM_GETPASSWORDCHAR); }
	void put_PasswordChar(TCHAR ch) const { SendMessage(EM_SETPASSWORDCHAR, ch); }
	DEFPROP_CONST(TCHAR, PasswordChar);
};

class AFX_CLASS CStatic : public CWnd { //!!!
public:
	HENHMETAFILE SetEnhMetaFile(HENHMETAFILE hMetaFile);
};

class CButton : public CWnd { //!!!
public:
	virtual BOOL Create5(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) {
		return CWnd::Create(_T("BUTTON"), lpszCaption, dwStyle, rect, pParentWnd, nID);
	}

	AFX_API UINT GetState() const { return (UINT)SendMessage(BM_GETSTATE); }
	AFX_API void SetState(bool bHighlight) { SendMessage(BM_SETSTATE, bHighlight); }

	int get_Check() const { return (int)SendMessage(BM_GETCHECK); }
	void put_Check(int nCheck) { SendMessage(BM_SETCHECK, nCheck); }
	DEFPROP_CONST(int, Check);
};

#if UCFG_GUI

class CUrlLabel : public CEdit {
public:
	afx_msg BOOL OnSetCursor( CWnd* pWnd, UINT nHitTest, UINT message );
	afx_msg void OnLButtonDown( UINT nFlags, Point point);
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);

	DECLARE_MESSAGE_MAP()
};


#endif

class /*!!!AFX_CLASS*/ CControlBarBase : public CWnd {
public:
	enum StateFlags
	{ delayHide = 1, delayShow = 2, tempHide = 4, statusSet = 8 };

	UINT m_nStateFlags;

	CControlBarBase();
	void ResetTimer(UINT_PTR nEvent, UINT nTime);
	virtual BOOL SetStatusText(int nHit);
};

class AFX_CLASS CToolTipCtrl : public CWnd {
public:
	void Create2(CWnd *pParentWnd, DWORD dwStyle = 0);
protected:
	std::unordered_map<String, void*> m_mapString;

	void OnEnable(BOOL bEnable);

	afx_msg LRESULT OnAddTool(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDisableModal(WPARAM, LPARAM);
	afx_msg LRESULT OnWindowFromPoint(WPARAM, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};


#include "extcolor.h"


class CCommCtrl : public CWnd {
	typedef CWnd base;
	DECLARE_DYNAMIC(CCommCtrl)
public:
	virtual void Create4(DWORD dwStyle, const RECT& rect, CWnd *pParentWnd, UINT nID) {
		CWnd::Create(m_strClass, nullptr, dwStyle, rect, pParentWnd, nID);
	}
protected:
	String m_strClass;

	BOOL PreCreateWindow(CREATESTRUCT& cs);
};

class /*!!!AFX_CLASS*/ CIPAddressCtrl : public CCommCtrl {
	DECLARE_DYNAMIC(CIPAddressCtrl)
public:
	CIPAddressCtrl();
	void Create4(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	bool IsBlank() const;
	void ClearAddress() { SendMessage(IPM_CLEARADDRESS); }
	int GetAddress(IPAddress& ip) const;
	void SetAddress(const IPAddress& ip);
	void SetFieldFocus(WORD nField);
	void SetFieldRange(int nField, BYTE nLower, BYTE nUpper);
};

class DateTimePicker : public CCommCtrl
{
	DECLARE_DYNAMIC(DateTimePicker)
public:
	DateTimePicker();

	DateTime get_Value();
	void put_Value(DateTime dt);
	DEFPROP(DateTime, Value);
};

class ProgressBar : public CCommCtrl {
public:
	ProgressBar() {
		m_strClass = PROGRESS_CLASS;
		AfxDeferRegisterClass(AFX_WNDCOMMCTL_PROGRESS_REG);
	}

	void SetPos(int nNewPos) { SendMessage(PBM_SETPOS, nNewPos); }
	void SetRange(int nMin, int nMax) { SendMessage(PBM_SETRANGE32, nMin, nMax); }
};

class CPropertyPage : public CDialog {
	DECLARE_DYNAMIC(CPropertyPage)

	CDialogTemplate m_dialogTemplate;
	HGLOBAL m_hDialogTemplate;//!!!
	String m_strHeaderTitle,
		m_strHeaderSubTitle;
protected:
	std::unique_ptr<PROPSHEETPAGE> m_pPSP;

	const PROPSHEETPAGE & get_m_psp() const { return *m_pPSP; }
	PROPSHEETPAGE & get_m_psp() { return *m_pPSP; }
	DEFPROP_GET(PROPSHEETPAGE, m_psp);

	String m_strCaption;
	bool m_bFirstSetActive;

	void AllocPSP(DWORD dwSize);
	void CommonConstruct(const CResID& resID, UINT nIDCaption, RCString headerTitle = "", DWORD dwSize = sizeof(PROPSHEETPAGE));
	virtual void PreProcessPageTemplate(PROPSHEETPAGE& psp, bool bWizard);
	virtual String LoadCaption() { return ""; }
	bool OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	void OnOK() {}
	void OnCancel() {}
	virtual bool OnSetActive();
	virtual bool OnKillActive();
	virtual bool OnApply();
	virtual void OnReset();
	virtual bool OnQueryCancel();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual bool OnWizardFinish();
	LRESULT MapWizardResult(LRESULT lToMap);
	bool IsButtonEnabled(int iButton);
public:
	CPropertyPage(const CResID& resID, UINT nIDCaption = 0, RCString headerTitle = "")
	{
		CommonConstruct(resID, nIDCaption, headerTitle);
	}

	friend class CPropertySheet;
};

class CPropertySheet : public CWnd {
	DECLARE_DYNAMIC(CPropertySheet)
public:
	PROPSHEETHEADER m_psh;

	int m_nModalResult;
	std::vector<CPropertyPage*> m_pages;

	CPropertySheet();
	CPropertySheet(UINT nIDCaption, CWnd* pParentWnd = 0, UINT iSelectPage = 0);
	CPropertySheet(RCString pszCaption, CWnd* pParentWnd = 0, UINT iSelectPage = 0);
	~CPropertySheet();
	void CommonConstruct(CWnd* pParentWnd, UINT iSelectPage);
	virtual BOOL OnInitDialog();
	CPropertyPage* GetPage(int nPage) const { return m_pages[nPage]; }
	void AddPage(CPropertyPage *pPage);
	int DoModal();
	virtual PROPSHEETHEADER *GetPropSheetHeader();
	virtual void BuildPropPageArray();
	bool IsWizard() const {return ((((CPropertySheet*)this)->GetPropSheetHeader()->dwFlags & (PSH_WIZARD | PSH_WIZARD97)) != 0);}
	void SetTitle(RCString text, UINT nStyle = 0);
	void SetWizardMode() { m_psh.dwFlags |= PSH_WIZARD; }
	void SetWizardButtons(DWORD dwFlags) { PostMessage(PSM_SETWIZBUTTONS, 0, dwFlags); }

	CWnd *GetTabControl() const { return CWnd::FromHandle((HWND)SendMessage(PSM_GETTABCONTROL)); }
protected:
	String m_strCaption;   // caption of the pseudo-dialog
	CWnd* m_pParentWnd;     // parent window of property sheet
	bool m_bStacked;        // EnableStackedTabs sets this
	bool m_bModeless;       // TRUE when Create called instead of DoModal

	afx_msg void OnClose();
	afx_msg LRESULT HandleInitDialog(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

	friend class CPropertyPage;
};

class AFX_CLASS CCheckListBox : public CListBox
{
	DECLARE_DYNAMIC(CCheckListBox)
protected:
	int m_cyText;
	UINT m_nStyle;

	afx_msg void OnLButtonDown(UINT nFlags, Point point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, Point point);
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBAddString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBFindString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBFindStringExact(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBGetItemData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBGetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBInsertString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBSelectString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBSetItemData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLBSetItemHeight(WPARAM wParam, LPARAM lParam);

	BOOL PreCreateWindow(CREATESTRUCT& cs);
	void PreDrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	void PreMeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	int PreCompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	void PreDeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);

	BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
	void SetSelectionCheck( int nCheck );
	int CalcMinimumItemHeight();
	void InvalidateCheck(int nIndex);
	void InvalidateItem(int nIndex);
	int CheckFromPoint(Point point, BOOL& bInCheck);
public:
	CCheckListBox()
		:	m_cyText(0),
		m_nStyle(0)
	{}

	~CCheckListBox();

	void Create4(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	void SetCheckStyle(UINT nStyle);
	UINT GetCheckStyle();

	int GetCheck(int nIndex);
	void SetCheck(int nIndex, int nCheck);
	//!!!  __declspec(property(get=GetCheck, put=SetCheck)) int Checks[];

	void Enable(int nIndex, BOOL bEnabled = TRUE);
	BOOL IsEnabled(int nIndex);
	virtual Rectangle OnGetCheckPosition(Rectangle rectItem, Rectangle rectCheckBox);
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	DECLARE_MESSAGE_MAP()
};

class CAppDialog : public CDialog {
public:
	CAppDialog(UINT nIDTemplate, CWnd* pParentWnd = 0)
		:	CDialog(nIDTemplate, pParentWnd)
	{}

	void OnOK();
	void OnCancel();
	void OnClose();
	void OnRegister();

	DECLARE_MESSAGE_MAP()
};

#undef AFX_DATA
#define AFX_DATA

#if UCFG_EXTENDED

class COlePropertyPage : public CDialog {
public:
	COlePropertyPage(UINT idDlg, UINT idCaption);
};

#endif // UCFG_EXTENDED


} // Ext::



#include <el/ext.h>

#include <el/gui/listview.h>
#include <el/gui/image.h>

namespace Ext::Gui {
using namespace std;

IMPLEMENT_DYNAMIC(ColumnHeaderCollection, CCommCtrl)
IMPLEMENT_DYNAMIC(ListView, CCommCtrl)

HWND ColumnHeaderCollection::get_Handle() const {
	if (!m_hWnd)
		m_hWnd = (HWND)_lv.Send(LVM_GETHEADER);
	return m_hWnd;
}

void ColumnHeaderCollection::Add(const String& text) {
	LVCOLUMN col = {
		LVCF_TEXT
		, 0
		, 0
		, (LPWSTR)(LPCWSTR)text
	};
	if (_lv.Send(LVM_INSERTCOLUMN, size(), &col) < 0)
		Throw(E_FAIL);
}

void ColumnHeaderCollection::Add(const String& text, int width) {
	LVCOLUMN col = {
		LVCF_TEXT | LVCF_WIDTH
		, 0
		, width
		, (LPWSTR)(LPCWSTR)text
	};
	if (_lv.Send(LVM_INSERTCOLUMN, size(), &col) < 0)
		Throw(E_FAIL);
}

void ColumnHeaderCollection::Add(const String& text, int width, HorizontalAlignment textAlign) {
	LVCOLUMN col = {
		LVCF_TEXT | LVCF_WIDTH | LVCF_FMT
		, (int)textAlign
		, width
		, (LPWSTR)(LPCWSTR)text
	};
	if (_lv.Send(LVM_INSERTCOLUMN, size(), &col) < 0)
		Throw(E_FAIL);
}

void ColumnHeaderCollection::clear() {
	for (auto nCol = size(); nCol-- > 0; )
		_lv.DeleteColumn(nCol);
}

ColumnHeader ColumnHeaderCollection::operator[](int idx) const {
	TCHAR buf[256];
	LV_COLUMN col = { .mask = LVCF_TEXT, .pszText = buf, .cchTextMax = sizeof(buf) };
	Win32Check(ListView_GetColumn(_lv, idx, &col));
	return { .Text = buf };
}

ListViewItem ListViewItemCollection::operator[](int idx) const {
	TCHAR text[260];
	LVITEM lvitem = { LVIF_TEXT, idx };
	lvitem.pszText = text;
	lvitem.cchTextMax = std::size(text);
	Win32Check(lv_.Send(LVM_GETITEM, 0, &lvitem));
	ListViewItem r(lv_);
	r.Text = text;
	return r;
}

size_t ListViewItemCollection::size() const {
	return lv_.Send(LVM_GETITEMCOUNT);
}

void ListViewItemCollection::clear() {
	Win32Check(lv_.Send(LVM_DELETEALLITEMS));
}

void ListViewItemCollection::Add(const ListViewItem& item) {
	LVITEM lvitem = { LVIF_TEXT | LVIF_PARAM };
	lvitem.pszText = (LPWSTR)(LPCWSTR)item.Text;
	lvitem.lParam = item.Tag;
	lvitem.iItem = lv_.InsertItem(lvitem);
	lvitem.mask = LVIF_TEXT;
	for (int i = 0, n = item.SubItems.size(); i < n; ++i) {
		lvitem.iSubItem = i + 1;
		lvitem.pszText = (LPWSTR)(LPCWSTR)item.SubItems[i].Text;
		lv_.SetItem(lvitem);
	}
}

void ListViewItemCollection::Add(const String& text) {
	ListViewItem item;
	item.Text = text;
	Add(item);
}

BEGIN_MESSAGE_MAP(ListView, CWnd)
	ON_WM_NCDESTROY()
END_MESSAGE_MAP()

ListView::ListView()
	: Columns(*this)
	, Items(*this) {
	m_strClass = WC_LISTVIEW;
	AfxEndDeferRegisterClass(AFX_WNDCOMMCTL_LISTVIEW_REG);
}

void ListView::Create4(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) {
	CWnd::Create(WC_LISTVIEW, nullptr, dwStyle, rect, pParentWnd, nID);
}

void ListView::DeleteItem(int nItem) {
	Win32Check(SendMessage(LVM_DELETEITEM, nItem));
}

void ListView::EnsureVisible(int index) {
	Win32Check(SendMessage(LVM_ENSUREVISIBLE, index, TRUE));
}

void ListView::SetItemCount(int iCount, DWORD dwFlags) {
	SendMessage(LVM_SETITEMCOUNT, iCount, dwFlags);
}

vector<ListViewItem> ListView::GetItems(LONG mask) {
	vector<ListViewItem> r;
	UINT iIndex = 0;
	UINT iTopIndex = 0;

	for (int nItem = -1; (nItem = ListView_GetNextItem(m_hWnd, nItem, mask)) != -1; ++iIndex) {
		LVITEM lvi = { 0 };
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
		lvi.iItem = nItem;
		lvi.stateMask = 0; // uSortStateFirst;
		TCHAR textBuf[256];
		lvi.pszText = textBuf;
		lvi.cchTextMax = size(textBuf);
		if (!ListView_GetItem(m_hWnd, &lvi))
			break;
		ListViewItem item(*this);
		item.Index = nItem;
		item.Text = lvi.pszText;
		item.Tag = lvi.lParam;
		r.push_back(item);
	}
	return r;
}

void ListView::put_View(Ext::Gui::View v) {
	SendMessage(LVM_SETVIEW, (DWORD)v);

	// Pre-Vista ListView API
	DWORD style = LVS_ICON;
	switch (v) {
	case View::LargeIcon: style = LVS_ICON; break;
	case View::SmallIcon: style = LVS_SMALLICON; break;
	case View::List: style = LVS_LIST; break;
	case View::Details:
	case View::Tile:
		style = LVS_REPORT;
		break;
	}
	Style = style | (Style & ~LVS_TYPEMASK);
}

UINT ListView::GetItemState(int nItem, UINT nMask) const {
	return (UINT)SendMessage(LVM_GETITEMSTATE, nItem, nMask);
}

void ListView::SetItemState(int nItem, UINT data, UINT mask) {
	LVITEM item;
	item.stateMask = mask;
	item.state = data;
	Win32Check(Send(LVM_SETITEMSTATE, nItem, (LPARAM)&item));
}

void ListView::SetItem(const LVITEM& item) {
	Win32Check(Send(LVM_SETITEM, 0, &item));
}

int ListView::InsertColumn(int nCol, const LVCOLUMN* pColumn) {
	int r = (int)Send(LVM_INSERTCOLUMN, nCol, (LPARAM)pColumn);
	Win32Check(r != -1);
	return r;
}

int ListView::InsertColumn(int nCol, RCString lpszColumnHeading, int nFormat, int nWidth, int nSubItem) {
	LVCOLUMN column;
	column.mask = LVCF_TEXT | LVCF_FMT;
	column.pszText = (LPTSTR)(LPCTSTR)lpszColumnHeading;
	column.fmt = nFormat;
	if (nWidth != -1) {
		column.mask |= LVCF_WIDTH;
		column.cx = nWidth;
	}
	if (nSubItem != -1) {
		column.mask |= LVCF_SUBITEM;
		column.iSubItem = nSubItem;
	}
	return ListView::InsertColumn(nCol, &column);
}

int ListView::InsertItem(const LVITEM& item) {
	int r = (int)Send(LVM_INSERTITEM, 0, &item);
	Win32Check(r != -1);
	return r;
}

int ListView::InsertItem(int nItem, RCString lpszItem) {
	return InsertItem(LVIF_TEXT, nItem, lpszItem, 0, 0, 0, 0);
}

int ListView::InsertItem(int nItem, RCString lpszItem, int nImage) {
	return InsertItem(LVIF_TEXT | LVIF_IMAGE, nItem, lpszItem, 0, 0, nImage, 0);
}

int ListView::InsertItem(UINT nMask, int nItem, RCString lpszItem, UINT nState, UINT nStateMask, int nImage, LPARAM lParam) {
	LVITEM item;
	item.mask = nMask;
	item.iItem = nItem;
	item.iSubItem = 0;
	item.pszText = (LPTSTR)(LPCTSTR)lpszItem;
	item.state = nState;
	item.stateMask = nStateMask;
	item.iImage = nImage;
	item.lParam = lParam;
	return InsertItem(item);
}

String ListView::GetItemText(int nItem, int nSubItem) const {
	LVITEM lvi; ZeroStruct(lvi);
	lvi.iSubItem = nSubItem;
	unique_ptr<TCHAR> p;
	int nLen = 128;
	int nRes;
	do {
		nLen *= 2;
		lvi.cchTextMax = nLen;
		p.reset(new TCHAR[nLen]);
		lvi.pszText = p.get();
		memset(lvi.pszText, 0, nLen);
		nRes = (int)SendMessage(LVM_GETITEMTEXT, nItem, (LPARAM)&lvi);
	} while (nRes == nLen - 1);
	return lvi.pszText;
}

void ListView::SetItemText(int nItem, int nSubItem, RCString lpszText) {
	LVITEM lvi;
	lvi.iSubItem = nSubItem;
	lvi.pszText = (LPTSTR)(LPCTSTR)lpszText;
	Win32Check(Send(LVM_SETITEMTEXT, nItem, (LPARAM)&lvi));
}

void ListView::SetColumnWidth(int nCol, int cx) {
	Win32Check(Send(LVM_SETCOLUMNWIDTH, nCol, MAKELPARAM(cx, 0)));
}

Rectangle ListView::GetItemRect(int iItem, UINT nCode) const {
	RECT r;
	r.left = nCode;
	Win32Check(SendMessage(LVM_GETITEMRECT, iItem, (LPARAM)&r));
	return r;
}

Rectangle ListView::GetSubItemRect(int iItem, int iSubItem, int nArea) const {
	RECT r;
	r.top = iSubItem;
	r.left = nArea;
	Win32Check(SendMessage(LVM_GETSUBITEMRECT, iItem, (LPARAM)&r));
	return r;
}

#if UCFG_THREAD_MANAGEMENT
ImageList* ListView::GetImageList(int nImageListType) const {
	return ImageList::FromHandle((HIMAGELIST)SendMessage(LVM_GETIMAGELIST, nImageListType));
}

ImageList* ListView::SetImageList(ImageList* pImageList, int nImageListType) {
	return ImageList::FromHandle((HIMAGELIST)SendMessage(LVM_SETIMAGELIST, nImageListType, (LPARAM)pImageList->GetSafeHandle()));
}

#endif // UCFG_THREAD_MANAGEMENT

void ListView::RemoveImageList(int nImageList) {
	HIMAGELIST h = (HIMAGELIST)SendMessage(LVM_GETIMAGELIST, (WPARAM)nImageList);
#if UCFG_THREAD_MANAGEMENT
	if (ImageList::FromHandlePermanent(h))
		SendMessage(LVM_SETIMAGELIST, nImageList);
#endif
}

DWORD ListView::put_ExtendedStyle(DWORD dwNewStyle) {
	return (DWORD)SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwNewStyle);
}

DWORD ListView::SetExtendedStyle(DWORD dwMask, DWORD dwStyle) {
	return (DWORD)SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, dwMask, dwStyle);
}

void ListView::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) {
}

void ListView::Sort(void* userData) {
	Win32Check(SendMessage(LVM_SORTITEMS, (WPARAM)userData, (LPARAM)ListViewItemSorter));
}

BOOL ListView::PreCreateWindow(CREATESTRUCT& cs) {
	cs.style |= VirtualMode ? LVS_OWNERDATA : 0;
	return base::PreCreateWindow(cs);
}

#if UCFG_EXTENDED
BOOL ListView::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) {
	if (message != WM_DRAWITEM)
		return CWnd::OnChildNotify(message, wParam, lParam, pResult);
	DrawItem((LPDRAWITEMSTRUCT)lParam);
	return TRUE;
}
#endif // UCFG_EXTENDED

void ListView::OnNcDestroy() {
	RemoveImageList(LVSIL_NORMAL);
	RemoveImageList(LVSIL_SMALL);
	RemoveImageList(LVSIL_STATE);
	CWnd::OnNcDestroy();
}

bool ListViewItem::get_Cut() const {
	return plv->GetItemState(Index, LVIS_CUT);
}

void ListViewItem::put_Cut(bool v) {
	return plv->SetItemState(Index, v ? LVIS_CUT : 0, LVIS_CUT);
}

bool ListViewItem::get_Focused() const {
	return plv->GetItemState(Index, LVIS_FOCUSED);
}

void ListViewItem::put_Focused(bool v) {
	return plv->SetItemState(Index, v ? LVIS_FOCUSED : 0, LVIS_FOCUSED);
}

Point ListViewItem::get_Position() const {
	POINT r;
	Win32Check(ListView_GetItemPosition(plv->m_hWnd, Index, &r));
	return r;
}

void ListViewItem::put_Position(const Point pt) {
	Win32Check(ListView_SetItemPosition(plv->m_hWnd, Index, pt.x, pt.y));
}

bool ListViewItem::get_Selected() const {
	return plv->GetItemState(Index, LVIS_SELECTED);
}

void ListViewItem::put_Selected(bool v) {
	return plv->SetItemState(Index, v ? LVIS_SELECTED : 0, LVIS_SELECTED);
}

Rectangle ListViewItem::get_Bounds() const {
	RECT rc = { 0 };
	ListView_GetItemRect(plv->m_hWnd, Index, &rc, LVIR_ICON);
	return rc;
}

void ListViewItem::BeginEdit() {
	ListView_EditLabel(plv->m_hWnd, Index);
}

void ListViewItem::EnsureVisible() {
	plv->EnsureVisible(Index);
}

void ListViewItem::Remove() {
	plv->DeleteItem(Index);
}

} // Ext::Gui

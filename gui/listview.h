#pragma once

#include <el/gui/controls.h>

#include <commctrl.h>

namespace Ext::Gui {

class ImageList;
class ListView;

enum class View {
	LargeIcon = LV_VIEW_ICON
	, Details = LV_VIEW_DETAILS
	, SmallIcon = LV_VIEW_SMALLICON
	, List = LV_VIEW_LIST
	, Tile = LV_VIEW_TILE
};

enum class HorizontalAlignment {
	Left = LVCFMT_LEFT
	, Right = LVCFMT_RIGHT
	, Center = LVCFMT_CENTER
};

enum class SoftOrder {
	None = 0
	, Ascending = 1
	, Descending = 2
};

class ColumnHeader {
public:
	String Text;
};

class ColumnHeaderCollection : public CCommCtrl {
	DECLARE_DYNAMIC(ColumnHeaderCollection)

	ListView& _lv;
protected:
	HWND get_Handle() const override;
public:
	void Add(const String& text);
	void Add(const String& text, int width);
	void Add(const String& text, int width, HorizontalAlignment textAlign);

	void clear();
	size_t size() const { return Send(HDM_GETITEMCOUNT); }

	ColumnHeader operator[](int idx) const;

	ColumnHeaderCollection(ListView& lv)
		: _lv(lv) {
		m_strClass = WC_HEADER;
	}
};

class ListViewSubItem {
public:
	String Text;
};

class ListViewSubItemCollection : public vector<ListViewSubItem> {
public:
	void Add(const String& text) {
		ListViewSubItem subItem;
		subItem.Text = text;
		push_back(subItem);
	}
};

class ListViewItem {
	ListView *plv = nullptr;
public:
	String Text;
	ListViewSubItemCollection SubItems;
	intptr_t Tag = 0;
	int Index = 0;
	bool Checked = false;	//!!!TODO

	bool get_Cut() const;
	void put_Cut(bool v);
	DEFPROP(bool, Cut);

	bool get_Focused() const;
	void put_Focused(bool v);
	DEFPROP(bool, Focused);

	Point get_Position() const;
	void put_Position(const Point pt);
	DEFPROP(Point, Position);

	bool get_Selected() const;
	void put_Selected(bool v);
	DEFPROP(bool, Selected);

	Rectangle get_Bounds() const;
	DEFPROP(Rectangle, Bounds);

	void BeginEdit();
	void EnsureVisible();
	void Remove();

	ListViewItem() {}
	ListViewItem(ListView& lv) : plv(&lv) {}
};

class ListViewItemCollection {
	ListView& lv_;
public:
	ListViewItem operator[](int idx) const;
	size_t size() const;
	void clear();

	void Add(const ListViewItem& item);
	void Add(const String& text);

	ListViewItemCollection(ListView& lv)
		: lv_(lv) {}
};

class AFX_CLASS ListView : public CCommCtrl {
	typedef CCommCtrl base;

	DECLARE_DYNAMIC(ListView)
public:
	ColumnHeaderCollection Columns;
	ListViewItemCollection Items;

	PFNLVCOMPARE ListViewItemSorter = nullptr;

	bool VirtualMode = false;

	void Create4(DWORD dwStyle, const RECT& rect, CWnd *pParentWnd, UINT nID);
	void DeleteItem(int nItem);
	void EnsureVisible(int index);

	void SetItemCount(int iCount, DWORD dwFlags = LVSICF_NOINVALIDATEALL);

	int get_VirtualListSize() { return Items.size(); }
	void put_VirtualListSize(int v) { SetItemCount(v, 0); }
	DEFPROP(int, VirtualListSize);

	bool get_FullRowSelect() { return Send(LVM_GETEXTENDEDLISTVIEWSTYLE) & LVS_EX_FULLROWSELECT; }
	void put_FullRowSelect(bool b) { Send(LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, b ? LVS_EX_FULLROWSELECT : 0); }
	DEFPROP(bool, FullRowSelect);

	bool get_AutoArrange() { return get_Style() & LVS_AUTOARRANGE; }
	void put_AutoArrange(bool b) {
		SetLong(GWL_STYLE, (b ? LVS_AUTOARRANGE : 0) | get_Style() & ~LVS_AUTOARRANGE);
	}
	DEFPROP(bool, AutoArrange);

	/*
	vector<ListViewItem> get_Items() { return GetItems(0); }
	DEFPROP(vector<ListViewItem>, Items);
	*/

	vector<ListViewItem> get_Selected() { return GetItems(LVNI_SELECTED); }
	DEFPROP(vector<ListViewItem>, Selected);

	Ext::Gui::View get_View() { return (Ext::Gui::View)SendMessage(LVM_GETVIEW); }
	void put_View(Ext::Gui::View v);
	DEFPROP(Ext::Gui::View, View);

	UINT GetItemState(int nItem, UINT nMask) const;
	void SetItemState(int nItem, UINT data, UINT mask);
	void SetItem(const LVITEM& item);
	void DeleteColumn(int nCol) { Win32Check((int)SendMessage(LVM_DELETECOLUMN, nCol)); } //!!!? LONG_PTR
	int InsertColumn(int nCol, const LVCOLUMN* pColumn);
	int InsertColumn(int nCol, RCString lpszColumnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1);
	int InsertItem(const LVITEM& item);
	int InsertItem(int nItem, RCString lpszItem);
	int InsertItem(int nItem, RCString lpszItem, int nImage);
	int InsertItem(UINT nMask, int nItem, RCString lpszItem, UINT nState, UINT nStateMask, int nImage, LPARAM lParam);
	String GetItemText(int nItem, int nSubItem) const;
	void SetItemText(int nItem, int nSubItem, RCString lpszText);
	void SetColumnWidth(int nCol, int cx);
	Rectangle GetItemRect(int iItem, UINT nCode) const;
	Rectangle GetSubItemRect(int iItem, int iSubItem, int nArea) const;
	ImageList *GetImageList(int nImageListType) const;
	ImageList *SetImageList(ImageList* pImageList, int nImageListType);
	void Sort(void *userData = 0);

	DWORD get_ExtendedStyle() const { return (DWORD)SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE); }
	DWORD put_ExtendedStyle(DWORD dwNewStyle);
	DEFPROP_CONST(DWORD, ExtendedStyle);

	DWORD SetExtendedStyle(DWORD dwMask, DWORD dwStyle);

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
	void OnNcDestroy();
	void RemoveImageList(int nImageList);
private:
	vector<ListViewItem> GetItems(LONG mask);
public:
	ListView();
};



} // Ext::Gui

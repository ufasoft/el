#pragma once

namespace Ext::Gui {

class Menu;
class MenuItem;

class MenuItemCollection {
	Menu& menu_;
public:
	int size() const;

	MenuItem FindById(uint32_t id);

	MenuItem operator[](int idx) const;

	MenuItemCollection(Menu& menu)
		: menu_(menu)
	{
	}
};

class AFX_CLASS Menu
//	: public Object
{
public:
	HMENU hMenu_;
	bool bOwn_ = true;

	MenuItemCollection MenuItems;

	operator HMENU() const {
		return hMenu_;
	}

	void Attach(HMENU hMenu);
	void Delete(UINT nPosition, UINT nFlags = MF_BYCOMMAND);
	UINT EnableItem(UINT nIDEnableItem, UINT nEnable);
	int CheckItem(UINT nItem, UINT nCheck);
	void CheckRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags = MF_BYCOMMAND);
	void Destroy();
	HMENU Detach();
	void CreatePopupMenu() { Attach(::CreatePopupMenu()); }
	void SetHandle(HANDLE h);
	void Append(UINT nFlags, UINT nIDNewItem=0, RCString newItem=nullptr);
	void Load(const CResID& resID);
	void TrackPopup(UINT nFlags, int x, int y, CWnd& wnd);

	void AppendMenu(UINT nFlags, UINT nIDNewItem=0, LPCTSTR newItem=0) {	//!!!comp
		Append(nFlags);
	}

	void LoadMenu(const CResID& resID) {	//!!!comp
		Load(resID);
	}

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	void RemoveById(uint32_t id);
	void Insert(UINT nPosition, UINT nFlags, UINT nIDNewItem = 0, RCString newItem = nullptr);
	void Modify(UINT nPosition, UINT nFlags, UINT nIDNewItem = 0, RCString newItem = nullptr);
	MENUITEMINFO GetItemInfo(UINT nItem, bool byPosition = false);
	void SetItemInfo(UINT nItem, const MENUITEMINFO& ii, bool byPosition = false);
	static Menu* AFXAPI FromHandle(HMENU hMenu);

#if !UCFG_WCE	
	UINT GetItemID(int nPos) const;
#endif

	Menu(HMENU hMenu = 0, bool bOwn = true);
	~Menu();
};

class MenuItem : public Menu {
	typedef Menu base;

	UINT get_State() const;

	HMENU hParentMenu;
	int index_;
public:
	int MenuID = 0;

	MenuItem(HMENU hMenu = 0, HMENU hParentMenu = 0, int index = -1, bool bOwn = true)
		: base(hMenu, bOwn)
		, hParentMenu(hParentMenu)
		, index_(index)
	{
	}

	MenuItem& operator=(const MenuItem o) {
		hMenu_ = o.hMenu_;
		bOwn_ = o.bOwn_;
		index_ = o.index_;
		MenuID = o.MenuID;
		return *this;
	}

	bool get_BarBreak() const { return get_State() & MF_MENUBARBREAK; }
	DEFPROP_GET(bool, BarBreak)

	bool get_Break() const { return get_State() & MF_MENUBREAK; }
	DEFPROP_GET(bool, Break)

	bool get_Checked() const { return get_State() & MF_CHECKED; }
	DEFPROP_GET(bool, Checked)

	bool get_DefaultItem() const;
	void put_DefaultItem(bool v) const;
	DEFPROP(bool, DefaultItem)

	bool get_Enabled() const { return !(get_State() & (MF_GRAYED | MF_DISABLED)); }
	DEFPROP_GET(bool, Enabled)

	int get_Index() const { return index_; }
	DEFPROP_GET(int, Index)

	bool get_IsParent() const { return get_State() & MF_POPUP; }
	DEFPROP_GET(bool, IsParent)

	bool get_IsSeparator() const { return get_State() & MF_SEPARATOR; }
	DEFPROP_GET(bool, IsSeparator)

	String get_Text() const;
	void put_Text(RCString text);
	DEFPROP(String, Text);


	/*
	Menu* get_SubMenu();
	DEFPROP_GET(Menu*, SubMenu);
	*/
};




} // Ext::Gui

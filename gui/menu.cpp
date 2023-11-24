#include <el/ext.h>

#include <windows.h>

#include <el/libext/win32/ext-win.h>
#include <el/libext/win32/ext-wnd.h>
#include <el/gui/menu.h>

namespace Ext::Gui {
using namespace std;


#if UCFG_THREAD_MANAGEMENT
CHandleMap<Menu>* afxMapHMENU() {
	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
	return &pState->GetHandleMaps().m_mapHMENU;
}
#endif

/*!!!R
Menu *MenuItem::get_SubMenu() {
	MENUITEMINFO mi = { sizeof mi, MIIM_SUBMENU };
	Win32Check(::GetMenuItemInfo(_hMenu, Position, TRUE, &mi));
	return mi.hSubMenu ? Menu::FromHandle(mi.hSubMenu) : 0;
}
*/

Menu::Menu(HMENU hMenu, bool bOwn)
	: hMenu_(hMenu)
	, bOwn_(bOwn)
	, MenuItems(*this) {
}

Menu::~Menu() {
	Destroy();
}

void Menu::Attach(HMENU hMenu) {
	hMenu_ = hMenu;
#if UCFG_THREAD_MANAGEMENT
	afxMapHMENU()->SetPermanent(m_hMenu, this);
#endif
}

void Menu::Delete(UINT nPosition, UINT nFlags) {
	Win32Check(::DeleteMenu(hMenu_, nPosition, nFlags));
}

UINT Menu::EnableItem(UINT nIDEnableItem, UINT nEnable) {
	return EnableMenuItem(hMenu_, nIDEnableItem, nEnable);
}

int Menu::CheckItem(UINT nItem, UINT nCheck) {
	return ::CheckMenuItem(hMenu_, nItem, nCheck);
}

void Menu::CheckRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags) {
	Win32Check(::CheckMenuRadioItem(hMenu_, nIDFirst, nIDLast, nIDItem, nFlags));
}

HMENU Menu::Detach() {
	HMENU hMenu = exchange(hMenu_, (HMENU)0);
#if UCFG_THREAD_MANAGEMENT
	if (hMenu)
		afxMapHMENU()->RemoveHandle(hMenu);
#endif
	return hMenu;
}

void Menu::Destroy() {
	if (HMENU hMenu = Detach())
		if (bOwn_)
			Win32Check(::DestroyMenu(hMenu));
}

void Menu::Append(UINT nFlags, UINT nIDNewItem, RCString newItem) {
	Win32Check(::AppendMenu(hMenu_, nFlags, nIDNewItem, newItem));
}

void Menu::Load(const CResID& resID) {
	HMENU hMenu = ::LoadMenu(AfxFindResourceHandle(resID, RT_MENU), resID);
	Win32Check(hMenu != 0);
	Attach(hMenu);
}

void Menu::TrackPopup(UINT nFlags, int x, int y, CWnd& wnd) {
#if UCFG_WCE
	Throw(E_FAIL);
	BOOL b = 0;
	//!!!	BOOL b = ::TrackPopupMenuEx(_hMenu, nFlags, x, y, wnd.GetSafeHwnd(), lpRect);
#else
	BOOL b = ::TrackPopupMenu(hMenu_, nFlags, x, y, 0, wnd.GetSafeHwnd(), nullptr);
#endif
	if (!(nFlags & TPM_RETURNCMD))
		Win32Check(b);
}

void Menu::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) {
}

void Menu::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) {
}

MenuItem MenuItemCollection::FindById(uint32_t id) {
	MenuItem r;
#if UCFG_WIN32_FULL
	int n = size();
#else
	int n = 100;
	try {
#endif
		for (int i = 0; i < n; ++i) {
			MENUITEMINFO mii = { sizeof mii, MIIM_ID };
			Win32Check(::GetMenuItemInfo(menu_, i, TRUE, &mii));
			if (mii.wID == id)
				return MenuItem(0, menu_, i, false);
		}
#if !UCFG_WIN32_FULL
	} catch (RCExc) {	//!!! check ex
	}
#endif
	return r;
}

void Menu::RemoveById(uint32_t id) {
	if (MenuItem item = MenuItems.FindById(id))
		Delete(item.Index, MF_BYPOSITION);
}

void Menu::Insert(UINT nPosition, UINT nFlags, UINT nIDNewItem, RCString newItem) {
	Win32Check(::InsertMenu(hMenu_, nPosition, nFlags, nIDNewItem, newItem));
}

MENUITEMINFO Menu::GetItemInfo(UINT nItem, bool byPosition) {
	MENUITEMINFO ii; ZeroStruct(ii);
	ii.cbSize = sizeof ii;
	ii.fMask = MIIM_STATE;
	Win32Check(::GetMenuItemInfo(hMenu_, nItem, byPosition, &ii));
	return ii;
}

void Menu::SetItemInfo(UINT nItem, const MENUITEMINFO& ii, bool byPosition) {
	Win32Check(::SetMenuItemInfo(hMenu_, nItem, byPosition, &ii));
}

#if UCFG_THREAD_MANAGEMENT
Menu* Menu::FromHandle(HMENU hMenu) {
	return afxMapHMENU()->FromHandle(hMenu);
}
#endif

void Menu::SetHandle(HANDLE h) {
	hMenu_ = (HMENU)h;
}

#if !UCFG_WCE

UINT Menu::GetItemID(int nPos) const {
	return ::GetMenuItemID(hMenu_, nPos);
}

void Menu::Modify(UINT nPosition, UINT nFlags, UINT nIDNewItem, RCString newItem) {
	Win32Check(::ModifyMenu(hMenu_, nPosition, nFlags, nIDNewItem, newItem));
}

int MenuItemCollection::size() const {
	int count = ::GetMenuItemCount(menu_);
	Win32Check(count != -1);
	return count;
}

MenuItem MenuItemCollection::operator[](int idx) const {
	UINT state = ::GetMenuState(menu_, idx, MF_BYPOSITION);
	Win32Check(state != UINT(-1));
	MenuItem r(state & MF_POPUP ? ::GetSubMenu(menu_, idx) : 0, menu_, idx, false);
	r.MenuID = ::GetMenuItemID(menu_, idx);
	return r;
}

UINT MenuItem::get_State() const {
	TRC(1, "Index: " << Index);
	UINT r = ::GetMenuState(hParentMenu, Index, MF_BYPOSITION);
	Win32Check(r != UINT(-1));
	return r;
}

bool MenuItem::get_DefaultItem() const {
	UINT r = ::GetMenuDefaultItem(hParentMenu, true, 0);
	Win32Check(r != -1);
	return r == Index;
}

void MenuItem::put_DefaultItem(bool v) const {
	Win32Check(::SetMenuDefaultItem(hParentMenu, Index, true));
}

String MenuItem::get_Text() const {
	TRC(1, "Index: " << Index);
	MENUITEMINFO ii { sizeof ii, MIIM_STRING };
	Win32Check(::GetMenuItemInfo(hParentMenu, Index, true, &ii));
	if (ii.cch == 0)
		return String();
	TRC(1, "allocating " << ii.cch + 1 << " TCHARs");
	ii.dwTypeData = (TCHAR*)alloca(sizeof(TCHAR) * ++ii.cch);
	Win32Check(::GetMenuItemInfo(hParentMenu, Index, true, &ii));
	return ii.dwTypeData;
}

void MenuItem::put_Text(RCString text) {
	MENUITEMINFO ii{ .cbSize = sizeof ii, .fMask = MIIM_STRING, .dwTypeData = (LPWSTR)(const TCHAR*)text };
	Win32Check(::SetMenuItemInfo(hParentMenu, MenuID, false, &ii));
}


#endif // !UCFG_WCE




} // Ext::Gui::

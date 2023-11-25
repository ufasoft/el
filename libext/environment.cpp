/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <windows.h>
#	include <shlobj.h>

#	include <el/libext/win32/ext-win.h>
#endif

#if UCFG_WIN32_FULL
#	include <el/libext/win32/ext-full-win.h>
#endif


namespace Ext {
using namespace std;

int Environment::ExitCode;

String Environment::GetEnvironmentVariable(RCString s) {
#if UCFG_USE_POSIX
	return ::getenv(s);
#elif UCFG_WCE
	return nullptr;
#else
	_TCHAR* p = (_TCHAR*)alloca(256 * sizeof(_TCHAR));
	DWORD dw = ::GetEnvironmentVariable(s, p, 256);
	if (dw > 256) {
		p = (_TCHAR*)alloca(dw * sizeof(_TCHAR));
		dw = ::GetEnvironmentVariable(s, p, dw);
	}
	if (dw)
		return p;
	Win32Check(GetLastError() == ERROR_ENVVAR_NOT_FOUND);
	return nullptr;
#endif
}

int Environment::get_ProcessorCount() {
#if UCFG_WIN32
	return GetSystemInfo().dwNumberOfProcessors;
#elif UCFG_USE_POSIX
	return std::max((int)sysconf(_SC_NPROCESSORS_ONLN), 1);
#else
	return 1;
#endif
}


path Environment::GetFolderPath(SpecialFolder folder) {
#if UCFG_USE_POSIX
	path homedir = ToPath(GetEnvironmentVariable("HOME"));
	switch (folder) {
	case SpecialFolder::Desktop: 			return homedir / "Desktop";
	case SpecialFolder::ApplicationData: 	return homedir / ".config";
	default:
		Throw(E_NOTIMPL);
	}

#elif UCFG_WIN32
	TCHAR path[_MAX_PATH];
#	if UCFG_OLE
	switch (folder) {
	case SpecialFolder::Downloads:
	{
		typedef HRESULT(STDAPICALLTYPE* PFN_SHGetKnownFolderPath)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);
		static DlProcWrap<PFN_SHGetKnownFolderPath> pfn("SHELL32.DLL", "SHGetKnownFolderPath");
		if (pfn) {
			COleString oleStr;
			OleCheck(pfn(FOLDERID_Downloads, 0, 0, &oleStr));
			return ToPath(oleStr);
		}
	}
	return GetFolderPath(SpecialFolder::UserProfile) / "Downloads";
	default:
		LPITEMIDLIST pidl;
		OleCheck(::SHGetSpecialFolderLocation(0, (int)folder, &pidl));
		Win32Check(::SHGetPathFromIDList(pidl, path));
		CComPtr<IMalloc> aMalloc;
		OleCheck(::SHGetMalloc(&aMalloc));
		aMalloc->Free(pidl);
	}
#	else
	if (!SHGetSpecialFolderPath(0, path, (int)folder, false))
		Throw(E_FAIL);
#	endif
	return path;
#else
	Throw(E_NOTIMPL);
#endif
}

} // Ext::

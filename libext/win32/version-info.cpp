/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#pragma comment(lib, "version")

#if UCFG_WIN32
#	include <windows.h>

#	include <el/libext/win32/ext-win.h>
#endif

#if UCFG_WIN32_FULL
#	include <el/libext/win32/ext-full-win.h>
#endif


namespace Ext {
using namespace std;

#if UCFG_EXTENDED
FileVersionInfo::FileVersionInfo(RCString fileName) {
	String s = fileName != nullptr ? fileName : AfxGetModuleState()->FileName.native();
	DWORD dw;
	int size = GetFileVersionInfoSize((TCHAR*)(const TCHAR*)s, &dw);
	if (!size && !fileName)
		return; //!!!
	Win32Check(size);
	m_blob.resize(size);
	Win32Check(GetFileVersionInfo((TCHAR*)(const TCHAR*)s, 0, size, m_blob.data()));
}
#endif

String FileVersionInfo::GetStringFileInfo(RCString s) {
	UINT size;
	void *p, *q;
	Win32Check(::VerQueryValue(m_blob.data(), _T("\\VarFileInfo\\Translation"), &q, &size));
	Win32Check(::VerQueryValue(m_blob.data(),
		(TCHAR*)(const TCHAR*)(_T("\\StringFileInfo\\")+Convert::ToString((*(WORD*)q << 16) | *(WORD*)((char*)q+2), "X8")+_T("\\")+s),
		&p, &size));
	return (TCHAR*)p;
}

const VS_FIXEDFILEINFO& FileVersionInfo::get_FixedInfo() {
	UINT size;
	void *p;
	Win32Check(::VerQueryValue(m_blob.data(), _T("\\"), &p, &size));
	return *(VS_FIXEDFILEINFO*)p;
}


String AFXAPI TryGetVersionString(const FileVersionInfo& vi, RCString name, RCString val) {
	//!!! FileVersionInfo vi(System.ExeFilePath);
	UINT size;
	void *p, *q;
	Win32Check(::VerQueryValue((void*)vi.m_blob.data(), _T("\\VarFileInfo\\Translation"), &q, &size));
	BOOL b = ::VerQueryValue((void*)vi.m_blob.data(), (TCHAR*)(const TCHAR*)(_T("\\StringFileInfo\\")+Convert::ToString((*(WORD*)q << 16) | *(WORD*)((char*)q+2), "X8")+_T("\\")+name), &p, &size);
	if (b)
		return (TCHAR*)p;
	else
		return val;
}

COperatingSystem::COsVerInfo COperatingSystem::get_Version() {
#	if UCFG_WDM
	RTL_OSVERSIONINFOEXW r = { sizeof r };
	NtCheck(::RtlGetVersion((RTL_OSVERSIONINFOW*)&r));
#	elif UCFG_WCE
	OSVERSIONINFO r = { sizeof r };
	Win32Check(::GetVersionEx(&r));
#	else
	OSVERSIONINFOEX r = { sizeof r };
	Win32Check(::GetVersionEx((OSVERSIONINFO*)&r));
#endif
	return r;
}

} // Ext::

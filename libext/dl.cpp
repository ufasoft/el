/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_USE_POSIX
#	include <dlfcn.h>
#endif

#if UCFG_WIN32
#	include <windows.h>
#endif

namespace Ext { 
using namespace std;

#if UCFG_USE_POSIX

DlException::DlException()
	:	base(E_EXT_Dynamic_Library, dlerror())
{
}

#elif UCFG_WIN32

DlProcWrapBase::DlProcWrapBase(RCString dll, RCString funname) {
	Init(::GetModuleHandle(dll), funname);
}

void DlProcWrapBase::Init(HMODULE hModule, RCString funname) {
	m_p = ::GetProcAddress(hModule, funname);
}
#endif // UCFG_WIN32


CDynamicLibrary::~CDynamicLibrary() {
	Free();
}

void CDynamicLibrary::Load(RCString path) const {
#if UCFG_USE_POSIX
	m_hModule = ::dlopen(path, 0);
	DlCheck(m_hModule == 0);
#else
	Win32Check((m_hModule = LoadLibrary(path)) != 0);
#endif
}

void CDynamicLibrary::Free() {
	if (m_hModule) {
		HMODULE h = m_hModule;
		m_hModule = 0;
#if UCFG_USE_POSIX
		DlCheck(::dlclose(h));
#else
		Win32Check(::FreeLibrary(h));
#endif
	}
}

FARPROC CDynamicLibrary::GetProcAddress(const CResID& resID) {
	ExcLastStringArgKeeper argKeeper(resID.ToString());

	FARPROC proc;
#if UCFG_USE_POSIX
	proc = (FARPROC)::dlsym(_self, resID);
	DlCheck(proc == 0);
#else
	proc = ::GetProcAddress(_self, resID);
	Win32Check(proc != 0);
#endif
	return proc;
}


} // Ext::


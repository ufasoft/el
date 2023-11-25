/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <wincon.h>

#include <el/libext/win32/ext-win.h>
#include <el/libext/win32/ext-full-win.h>

#pragma warning(disable: 4073)
#pragma init_seg(lib)

#if !UCFG_WCE
//#	include <AccCtrl.h>
#	include <sddl.h>
//#	include <aclapi.h>
#	include <winternl.h>
#endif

#if UCFG_WIN32_FULL
#	pragma comment(lib, "kernel32")
#endif

namespace Ext {

using namespace std;

#if !UCFG_WCE

void NamedPipe::Create(const NamedPipeCreateInfo& ci) {
	HANDLE h = ::CreateNamedPipe(ci.Name, ci.OpenMode, ci.PipeMode, ci.MaxInstances, 4096, 4096, 0, ci.Sa);
	Win32Check(h != INVALID_HANDLE_VALUE);
	Attach((intptr_t)h);
}

bool NamedPipe::Connect(LPOVERLAPPED ovl) {
	if (ovl)
		Win32Check(::ResetEvent(ovl->hEvent));
	bool r = ::ConnectNamedPipe((HANDLE)(intptr_t)Handle(_self), ovl) || ::GetLastError() == ERROR_PIPE_CONNECTED;
	if (ovl) {
		if (!r) {
			int dw = ::WaitForSingleObjectEx(ovl->hEvent, INFINITE, TRUE);
			if (dw == WAIT_IO_COMPLETION)
				Throw(ExtErr::ThreadInterrupted);		//!!!?
			if (dw != 0)
				Throw(E_FAIL);
			dw = GetOverlappedResult(*ovl);		//!!!
			r = true;
		}
	} else
		Win32Check(r, ERROR_PIPE_LISTENING);
	return r;
}

#endif


bool AFXAPI IsConsole() {
	BYTE *base = (BYTE*)GetModuleHandle(0);
	IMAGE_DOS_HEADER *dh = (IMAGE_DOS_HEADER*)base;
	IMAGE_OPTIONAL_HEADER32 *oh = (IMAGE_OPTIONAL_HEADER32 *)(base + dh->e_lfanew + IMAGE_SIZEOF_FILE_HEADER + 4);
	return oh->Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI;
}





/*!!!D
String ExtractFilePath(RCString path)
{
CSplitPath sp = SplitPath(path);
return sp.m_drive+sp.m_dir;
}*/



/*!!!
HRESULT ProcessOleException(CException *e)
{
HRESULT hr = COleException::Process(e);
e->Delete();
return hr;
}*/

void AFXAPI SelfRegisterDll(const path& p) {
	CDynamicLibrary lib;
	lib.Load(p);
	typedef HRESULT (STDAPICALLTYPE *CTLREGPROC)();
	CTLREGPROC proc = (CTLREGPROC)lib.GetProcAddress("DllRegisterServer");
	OleCheck(proc());
}



CStringVector AsciizArrayToStringArray(const TCHAR *p) {
	vector<String> vec;
	for (; *p; p+=_tcslen(p)+1)
		vec.push_back(p);
	return vec;
}




#ifdef WIN32

#if UCFG_EXTENDED
String AFXAPI AfxLoadString(uint32_t nIDS) {
	HINSTANCE h = AfxFindResourceHandle(MAKEINTRESOURCE(nIDS/16+1), RT_STRING);
	TCHAR buf[255];
	int nLen = ::LoadString(h, nIDS , buf, size(buf));
	Win32Check(nLen != 0);
	return buf;
}
#else
String AFXAPI AfxLoadString(uint32_t nIDS) {
	HINSTANCE h = GetModuleHandle(0);		//!!! only this module
	TCHAR buf[255];
	int nLen = ::LoadString(h, nIDS , buf, size(buf));
	Win32Check(nLen != 0);
	return buf;
}
#endif // UCFG_EXTENDED

#	if UCFG_COM

void String::Load(UINT nID) {
	_self = AfxLoadString(nID);
}

/*!!!R
bool String::LoadString(UINT nID) {			//!!!comp
	try {
		Load(nID);
		return true;
	} catch (RCExc) {
		return false;
	}
}*/

#	endif
#endif // WIN32


DWORD COperatingSystem::GetSysColor(int nIndex) {
	SetLastError(0);
	DWORD r = ::GetSysColor(nIndex);
	Win32Check(!GetLastError());
	return r;
}


void AFXAPI WinExecAndWait(RCString name) {
	STARTUPINFO info = { sizeof info };
	//!!!	STARTUPINFO *pinfo = &info;
	PROCESS_INFORMATION procInfo;
#if UCFG_WCE
	STARTUPINFO *psi = 0;
#else
	info.dwFlags = STARTF_USESHOWWINDOW;
	info.wShowWindow = SW_NORMAL;
	STARTUPINFO *psi = &info;
#endif
	vector<TCHAR> bufName(_tcslen(name)+1);
	_tcscpy(&bufName[0], name);
	if (::CreateProcess(0, &bufName[0], 0, 0, false, 0, 0, 0, psi, &procInfo)) {
		CloseHandle(procInfo.hThread);
		WaitForSingleObject(procInfo.hProcess, INFINITE);
		CloseHandle(procInfo.hProcess);
	}
}


COsVersion AFXAPI GetOsVersion() {
	COperatingSystem::COsVerInfo vi = System.Version;
	switch (vi.dwPlatformId) {
#if !UCFG_WCE
	case VER_PLATFORM_WIN32_NT:
		switch (vi.dwMajorVersion) {
		case 4:
			return OSVER_NT4;
		case 5:
			switch (vi.dwMinorVersion)
			{
			case 0: return OSVER_2000;
			case 1: return OSVER_XP;
			default: return OSVER_SERVER_2003;
			}
		case 6:
			switch (vi.dwMinorVersion) {
			case 0:
				switch (vi.wProductType) {
				case VER_NT_DOMAIN_CONTROLLER:
				case VER_NT_SERVER:
					return OSVER_2008;
				}
				return OSVER_VISTA;
			case 1:
				switch (vi.wProductType) {
				case VER_NT_DOMAIN_CONTROLLER:
				case VER_NT_SERVER:
					return OSVER_2008_R2;
				}
				return OSVER_7;
			case 2:
				return OSVER_8;
			case 3:
			default:
				return OSVER_8_1;
			}
		case 10:
			return OSVER_10;
		default:
			return OSVER_FUTURE;
		}
	case VER_PLATFORM_WIN32_WINDOWS:
		switch (vi.dwMajorVersion) {
		case 4:
			return OSVER_98; //!!! maybe 95;
		default:
			return OSVER_ME;
		}
#else
	case VER_PLATFORM_WIN32_CE:
		switch (vi.dwMajorVersion) {
		case 3: return OSVER_CE;
		case 4: return OSVER_CE_4;
		case 5: return OSVER_CE_5;
		case 6: return OSVER_CE_6;
		default: return OSVER_CE_FUTURE;
		}
#endif
	default:
		Throw(E_FAIL);

	}
}

/*!!!R
void *AlignedMalloc(size_t size, int align)
{
BYTE *pb = new BYTE[size+align+4-1];
void *r = (void*)(DWORD_PTR(pb+align+4-1) / align * align);
*((void**)r-1) = pb;
return r;
}

void AlignedFree(void *p)
{
if (p)
delete[] *((BYTE**)p-1);
}
*/

#if UCFG_EXTENDED

class CSetNewHandler {
	_PNH m_prev;

	static int __cdecl MyNewHandler(size_t) {
		Throw(E_OUTOFMEMORY);
		return 0; //!!!
	}
public:
	CSetNewHandler() {
		m_prev = _set_new_handler(MyNewHandler);
	}

	~CSetNewHandler() {
		_set_new_handler(m_prev);
	}
} g_setNewHandler;

#endif

/*!!!
void * __cdecl operator new(size_t size)
{
if (void *p = ::operator new(size))
return p;
Throw(E_OUTOFMEMORY);
}
*/


#if UCFG_USE_OLD_MSVCRTDLL
extern "C" {

	extern "C" _CRTIMP void __cdecl _assert(const char *, const char *, unsigned);

	void __stdcall my_wassert(const wchar_t *_Message, const wchar_t *_File, unsigned _Line) {
		_assert(String(_Message), String(_File), _Line);
	}
}
#endif



} // Ext::

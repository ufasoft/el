/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <el/libext/win32/ext-win.h>
#include <el/libext/win32/ext-full-win.h>

namespace Ext {
using namespace std;


ProcessStartInfo::ProcessStartInfo(const path& fileName, RCString arguments)
	: Flags(0)
	, FileName(fileName)
	, Arguments(arguments)
#if !UCFG_WCE
	, EnvironmentVariables(Environment::GetEnvironmentVariables())
#endif
{}

ProcessObj::ProcessObj()
	: SafeHandle(0, false)
	, m_stat_loc(0)
{
	CommonInit();
}

ProcessObj::ProcessObj(pid_t pid, DWORD dwAccess, bool bInherit)
	: SafeHandle(0, false)
	, m_pid(pid) {
	CommonInit();
	if (pid)
		Attach((intptr_t)::OpenProcess(dwAccess, bInherit, pid));
	/*!!!
	else
	{
		Attach(::GetCurrentProcess());
		m_bOwn = false;
		m_ID = GetCurrentProcessId();
	}*/
}

#if UCFG_WIN32
ProcessObj::ProcessObj(HANDLE handle, bool bOwn)
	: SafeHandle(0, false)
	, m_stat_loc(0)
{
	Attach(handle, bOwn);
	CommonInit();
}
#endif

void ProcessObj::CommonInit() {
#if !UCFG_WCE
	StandardInput.m_pFile.reset(&m_fileIn);
	StandardOutput.m_pFile.reset(&m_fileOut);
	StandardError.m_pFile.reset(&m_fileErr);
#endif
}

DWORD ProcessObj::get_ID() const {
#if UCFG_WIN32
	if (!m_pid) {
#if UCFG_WCE
		Throw(E_FAIL);
#else
		typedef DWORD (WINAPI *PFN_GetProcessId)(HANDLE);
		DlProcWrap<PFN_GetProcessId> pfn("KERNEL32.DLL", "GetProcessId");
		if (pfn)
			m_pid = pfn((HANDLE)(intptr_t)Handle(_self));
		else {
			/*!!!R
			typedef enum _PROCESSINFOCLASS {
				ProcessBasicInformation = 0,
				ProcessWow64Information = 26
			} PROCESSINFOCLASS;

			typedef void *PPEB;

			typedef struct _PROCESS_BASIC_INFORMATION {
				PVOID Reserved1;
				PPEB PebBaseAddress;
				PVOID Reserved2[2];
				ULONG_PTR UniqueProcessId;
				PVOID Reserved3;
			} PROCESS_BASIC_INFORMATION;
			typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;
*/
			typedef NTSTATUS (WINAPI * PFN_QueryInformationProcess)(
				HANDLE ProcessHandle,
				PROCESSINFOCLASS ProcessInformationClass,
				PVOID ProcessInformation,
				ULONG ProcessInformationLength,
				PULONG ReturnLength);

			DlProcWrap<PFN_QueryInformationProcess> ntQIP("NTDLL.DLL", "NtQueryInformationProcess");
			if (!ntQIP)
				Throw(E_FAIL);

			PROCESS_BASIC_INFORMATION info;
			ULONG returnSize;
			ntQIP((HANDLE)(intptr_t)Handle(_self), ProcessBasicInformation, &info, sizeof(info), &returnSize);  // Get basic information.
			m_pid = (DWORD)info.UniqueProcessId;
		}
#endif
	}
#endif
	return m_pid;
}

DWORD ProcessObj::get_ExitCode() const {
#if UCFG_WIN32
	DWORD r;
	Win32Check(::GetExitCodeProcess((HANDLE)(intptr_t)HandleAccess(*this), &r));
	return r;
#else
	return m_stat_loc;
#endif
}

void ProcessObj::Kill() {
#if UCFG_WIN32
	Win32Check(::TerminateProcess((HANDLE)(intptr_t)HandleAccess(*this), 1));
#else
	CCheck(::kill(m_pid, SIGKILL));
#endif
}

void ProcessObj::WaitForExit(DWORD ms) {
#if UCFG_WIN32
	DWORD r = WaitForSingleObject((HANDLE)(intptr_t)HandleAccess(*this), ms);
	Win32Check(r != WAIT_FAILED);
#else
	::waitpid(m_pid, &m_stat_loc, 0);
#endif
}

bool ProcessObj::get_HasExited() {
#if UCFG_WIN32
	return get_ExitCode() != STILL_ACTIVE;
#else
	int stat_loc = 0;
	::waitpid(m_pid, &stat_loc, WUNTRACED);
	return WIFEXITED(stat_loc);
#endif
}

bool ProcessObj::Start() {
#if UCFG_USE_POSIX
	vector<const char *> argv;
	String filename = StartInfo.FileName.native();
	argv.push_back(filename);
	vector<String> v = ParseCommandLine(StartInfo.Arguments);
	TRC(5, "ARGs:\n" << v << "---");
	for (size_t i=0; i<v.size(); ++i)
		argv.push_back(v[i]);
	argv.push_back(nullptr);
	if (!(m_pid = CCheck(::fork()))) {
		execv(filename, (char**)&argv.front());
		_exit(errno);           						// don't use any TRC() here, danger of Deadlock
	}
#elif UCFG_WIN32
	STARTUPINFO si = {sizeof si};
	static_assert(SW_SHOWNORMAL == 1, "Invalid SW SHOWNORMAL");
	si.wShowWindow = SW_SHOWNORMAL; // same as SW_NORMAL Critical for running iexplore
	SafeHandle hI, hO, hE;
#	if UCFG_WCE
	STARTUPINFO *psi = 0;
	bool bInheritHandles = false;
	LPCTSTR pCurrentDirectory = 0;
#	else
	if (StartInfo.RedirectStandardInput || StartInfo.RedirectStandardOutput || StartInfo.RedirectStandardError) {
		si.hStdInput = StdHandle::Get(STD_INPUT_HANDLE);
		si.hStdOutput = StdHandle::Get(STD_OUTPUT_HANDLE);
		si.hStdError = StdHandle::Get(STD_ERROR_HANDLE);
		si.dwFlags |= STARTF_USESTDHANDLES;
		HANDLE hRead, hWrite;
		SECURITY_ATTRIBUTES sattr = { sizeof(sattr), 0, TRUE };
		if (StartInfo.RedirectStandardInput) {
			Win32Check(::CreatePipe(&hRead, &hWrite, &sattr, 0));
			si.hStdInput = hRead;
			hI.Attach(hRead);
			StandardInput.m_pFile->Duplicate((intptr_t)hWrite, DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS);
		}
		if (StartInfo.RedirectStandardOutput) {
			Win32Check(::CreatePipe(&hRead, &hWrite, &sattr, 0));
			si.hStdOutput = hWrite;
			hO.Attach(hWrite);
			StandardOutput.m_pFile->Duplicate((intptr_t)hRead, DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS);
		}
		if (StartInfo.RedirectStandardError) {
			Win32Check(::CreatePipe(&hRead, &hWrite, &sattr, 0));
			si.hStdError = hWrite;
			hE.Attach(hWrite);
			StandardError.m_pFile->Duplicate((intptr_t)hRead, DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS);
		}
	}
	STARTUPINFO *psi = &si;
	bool bInheritHandles = bool(si.dwFlags & STARTF_USESTDHANDLES);
	String dir = StartInfo.WorkingDirectory.empty() ? String(nullptr) : String(StartInfo.WorkingDirectory);
	LPCTSTR pCurrentDirectory = dir;
#	endif

	PROCESS_INFORMATION pi;
	String cls = StartInfo.FileName;
	if (!StartInfo.Arguments.empty())
		cls += " " + StartInfo.Arguments;
	size_t len = (cls.length()+1)*sizeof(TCHAR);
	TCHAR *cl = (TCHAR*)alloca(len);
	memcpy(cl, (const TCHAR*)cls, len);
	String fileName = StartInfo.FileName.empty() ? String(nullptr) : String(StartInfo.FileName);
	DWORD flags = StartInfo.Flags;
	void *pEnvironment = 0;
#	if !UCFG_WCE
	if (StartInfo.CreateNoWindow)
		flags |= CREATE_NO_WINDOW;
	String sEnv;
	for (map<String, String>::iterator it=StartInfo.EnvironmentVariables.begin(), e=StartInfo.EnvironmentVariables.end(); it!=e; ++it)
		sEnv += it->first + "=" + it->second + String('\0', 1);
	pEnvironment = (void*)(const TCHAR*)sEnv;
#		ifdef _UNICODE
	flags |= CREATE_UNICODE_ENVIRONMENT;
#		endif
#	endif
	Win32Check(::CreateProcess(0, cl, 0, 0, bInheritHandles, flags, pEnvironment, (LPTSTR)pCurrentDirectory, psi, &pi)); //!!! BOOL is not bool
	Attach(pi.hProcess);
	m_pid = pi.dwProcessId;

	//ptr<CWinThread> pThread(new CWinThread);
	//pThread->Attach((intptr_t)pi.hThread, pi.dwThreadId);
	Win32Check(::CloseHandle(pi.hThread));

#else // UCFG_WIN32
	Throw(E_NOTIMPL);
#endif
	TRC(4, "PID: " << m_pid.Value());
	return true;
}

Process AFXAPI Process::Start(const ProcessStartInfo& psi) {
	TRC(2, psi.FileName << " " << psi.Arguments);

	Process r;
	r.m_pimpl = new ProcessObj;
	r->StartInfo = psi;
	r->Start();
	return r;
}

String Process::get_ProcessName() {
#if UCFG_USE_POSIX
	char szModule[PATH_MAX];
	memset(szModule, 0, sizeof szModule);
	CCheck(::readlink(String(EXT_STR("/proc/" << get_ID() << "/exe")), szModule, sizeof(szModule)));
	return path(szModule).filename().native();
#else
	TCHAR buf[MAX_PATH];
	Win32Check(::GetModuleBaseName((HANDLE)(intptr_t)Handle(*m_pimpl), 0, buf, size(buf)));
	path r = buf;
	String ext = ToLower(String(r.extension().native()));
	return ext==".exe" || ext==".bat" || ext==".com" || ext==".cmd" ? r.parent_path() / r.stem() : r;
#endif
}

Process AFXAPI Process::GetProcessById(pid_t pid) {
	Process r;
#if UCFG_USE_POSIX
	Throw(E_NOTIMPL);
#else
	r.m_pimpl = new ProcessObj(pid);
#endif
	return r;
}

Process AFXAPI Process::GetCurrentProcess() {
	return GetProcessById(getpid());
}

#if UCFG_WIN32
Process AFXAPI Process::FromHandle(HANDLE h, bool bOwn) {
	Process r;
	r.m_pimpl = new ProcessObj(h, bOwn);
	return r;
}
#endif // UCFG_WIN32

vector<Process> AFXAPI Process::GetProcesses() {
#if UCFG_USE_POSIX
	Throw(E_NOTIMPL);
#else
	vector<DWORD> pids(10);
	size_t n = pids.size();
	for (DWORD cbReturned; n==pids.size(); n=cbReturned/sizeof(DWORD)) {
		pids.resize(pids.size()*2);
		Win32Check(::EnumProcesses(&pids[0], pids.size()*sizeof(DWORD), &cbReturned));
	}
	vector<Process> r;
	for (size_t i=0; i<n; ++i) {
		if (pid_t pid = pids[i]) {
			if (HANDLE h = ::OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, false, pid)) {
				Process p;
				p.m_pimpl = new ProcessObj(h, true);
				r.push_back(p);
			} else
				Win32Check(false, ERROR_ACCESS_DENIED);
		}
	}
	return r;
#endif
}

vector<Process> AFXAPI Process::GetProcessesByName(RCString name) {
	vector<Process> procs = GetProcesses(),
		r;
	DBG_LOCAL_IGNORE_CONDITION(errc::bad_file_descriptor);
	DBG_LOCAL_IGNORE_WIN32(ERROR_PARTIAL_COPY);
	EXT_FOR (Process& p, procs) {
		try {
			if (p.ProcessName == name)
				r.push_back(p);
		} catch (system_error& ex) {
			const error_code& ec = ex.code();
			if (ec != errc::bad_file_descriptor
				&& ec != errc::permission_denied
#ifdef WIN32
				&& ec != error_code(ERROR_PARTIAL_COPY, system_category())
#endif
				)
				throw;
		}
	}
	return r;
}

void POpen::Wait() {
	switch (int rc = pclose(exchange(m_stream, nullptr))) {
	case -1:
		CCheck(-1);
	case 0:
		return;
	default:
		ThrowImp(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_PSTATUS, (uint8_t)rc));
	}
}


static String ToDosPath(RCString lpath) {
	vector<String> lds = System.LogicalDriveStrings;
	for (size_t i = 0; i < lds.size(); ++i) {
		String ld = lds[i];
		String dd = ld.Right(1) == "\\" ? ld.Left(ld.length() - 1) : ld;
		vector<String> v = System.QueryDosDevice(dd);
		if (v.size()) {
			String lp = v[0];
			if (lp.length() < lpath.length() && !lp.CompareNoCase(lpath.Left(lp.length()))) {
				return ((ld.Right(1) == "\\" && lpath.at(0) == '\\') ? dd : ld) + lpath.substr(lp.length());
			}
		}
	}
	return lpath;
}


typedef DWORD(WINAPI* PFN_GetProcessImageFileName)(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);

static CDynamicLibrary s_dllPsapi("psapi.dll");

struct SMainWindowHandleInfo {
	DWORD m_pid;
	HWND m_hwnd;
};

static BOOL CALLBACK EnumWindowsProc_MainWindowHandle(HWND hwnd, LPARAM lParam) {
	SMainWindowHandleInfo& info = *(SMainWindowHandleInfo*)lParam;
	if ((::GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE)) {
		DWORD pidwin;
		::GetWindowThreadProcessId(hwnd, &pidwin);
		if (pidwin == info.m_pid) {
			info.m_hwnd = hwnd;
			::SetLastError(0);
			return FALSE;
		}
	}
	return TRUE;
}

HWND ProcessObj::get_MainWindowHandle() const {
	SMainWindowHandleInfo info = { get_ID() };
	Win32Check(::EnumWindows(&EnumWindowsProc_MainWindowHandle, (LPARAM)&info), 0);
	return info.m_hwnd;
}

String ProcessObj::get_MainModuleFileName() {
	TCHAR buf[MAX_PATH];
	try {
		DBG_LOCAL_IGNORE_WIN32(ERROR_PARTIAL_COPY);
		DBG_LOCAL_IGNORE_WIN32(ERROR_INVALID_HANDLE);

		Win32Check(::GetModuleFileNameEx((HANDLE)(intptr_t)Handle(_self), 0, buf, size(buf)));
		return buf;
	} catch (RCExc) {
	}
	DlProcWrap<PFN_GetProcessImageFileName> pfnGetProcessImageFileName(s_dllPsapi, EXT_WINAPI_WA_NAME(GetProcessImageFileName));
	if (pfnGetProcessImageFileName) {
		Win32Check(pfnGetProcessImageFileName((HANDLE)(intptr_t)Handle(_self), buf, size(buf)));
		return ToDosPath(buf);
	}
	else
		ThrowWin32(ERROR_PARTIAL_COPY);
}

#if !UCFG_WCE


DWORD ProcessObj::get_Version() const {
	DWORD r = GetProcessVersion(get_ID());
	Win32Check(r);
	return r;
}

typedef WINBASEAPI BOOL(WINAPI* PFN_IsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);

bool ProcessObj::get_IsWow64() {
	BOOL r = FALSE;
	DlProcWrap<PFN_IsWow64Process> pfnIsWow64Process("KERNEL32.DLL", "IsWow64Process");
	if (pfnIsWow64Process)
		Win32Check(pfnIsWow64Process((HANDLE)(intptr_t)HandleAccess(_self), &r));
	return r;
}

CTimesInfo ProcessObj::get_Times() const {
	CTimesInfo r;
	Win32Check(::GetProcessTimes((HANDLE)(intptr_t)HandleAccess(_self), &r.m_tmCreation, &r.m_tmExit, &r.m_tmKernel, &r.m_tmUser));
	return r;
}

#endif  // !UCFG_WCE

#if UCFG_THREAD_MANAGEMENT
unique_ptr<CWinThread> ProcessObj::Create(RCString commandLine, DWORD dwFlags, const char *dir, bool bInherit, STARTUPINFO *psi) {
	STARTUPINFO si = {sizeof si};
	si.wShowWindow = SW_SHOWNORMAL; // Critical for running iexplore
	PROCESS_INFORMATION pi;
	String sDir = dir;
	LPCTSTR pszDir = sDir;
#if UCFG_WCE
	pszDir = nullptr;
#endif
	Win32Check(::CreateProcess(0, (LPTSTR)(LPCTSTR)commandLine, 0, 0, bInherit, dwFlags, 0, (LPTSTR)pszDir, psi?psi:&si, &pi));
	Attach((intptr_t)pi.hProcess);
	m_pid = pi.dwProcessId;
	unique_ptr<CWinThread> pThread(new CWinThread);
	pThread->Attach((intptr_t)pi.hThread, pi.dwThreadId);
	return pThread;
}
#endif // UCFG_THREAD_MANAGEMENT

SIZE_T ProcessObj::ReadMemory(LPCVOID base, LPVOID buf, SIZE_T size) {
	SIZE_T r;
	Win32Check(::ReadProcessMemory((HANDLE)(intptr_t)HandleAccess(_self), base, buf, size, &r));
	return r;
}

SIZE_T ProcessObj::WriteMemory(LPVOID base, LPCVOID buf, SIZE_T size) {
	SIZE_T r;
	Win32Check(::WriteProcessMemory((HANDLE)(intptr_t)HandleAccess(_self), base, (void*)buf, size, &r)); //!!!CE
	return r;
}

DWORD ProcessObj::VirtualProtect(void *addr, size_t size, DWORD flNewProtect) {
	DWORD r;
	if (GetCurrentProcessId() == get_ID())
		Win32Check(::VirtualProtect(addr, size, flNewProtect, &r));
	else
#if UCFG_WCE
		Throw(E_FAIL);
#else
		Win32Check(::VirtualProtectEx((HANDLE)(intptr_t)HandleAccess(_self), addr, size, flNewProtect, &r));
#endif
	return r;
}

MEMORY_BASIC_INFORMATION ProcessObj::VirtualQuery(const void *addr) {
	MEMORY_BASIC_INFORMATION r;
	SIZE_T size;
	if (GetCurrentProcessId() == get_ID())
		size = ::VirtualQuery(addr, &r, sizeof r);
	else
#if UCFG_WCE
		Throw(E_FAIL);
#else
		size = ::VirtualQueryEx((HANDLE)(intptr_t)HandleAccess(_self), addr, &r, sizeof r);
#endif
	if (!size)
		ZeroStruct(r);
	return r;
}




} // Ext::

#include <el/ext.h>

#include "ext-fw.h"

#if UCFG_WIN32
#	include <el/libext/win32/ext-win.h>
#endif

#if UCFG_COUT_REDIRECTOR=='R' && !defined(_AFXDLL)
	extern int _ext_crt_module_write;             		// enforce replacing write(), fputwc() functions
	extern int _ext_crt_module_fputwc;

	int *pModuleWite = &_ext_crt_module_write,
		*pModuleFputwc = &_ext_crt_module_fputwc;
#endif //  UCFG_COUT_REDIRECTOR=='R'



namespace Ext {
using namespace std;

CCommandLineInfo::CCommandLineInfo() {
	m_bShowSplash = true;
	m_bRunEmbedded = false;
	m_bRunAutomated = false;
	m_nShellCommand = FileNew;
}

void CCommandLineInfo::ParseParamFlag(RCString pszParam) {
	String p = pszParam;
	p.MakeUpper();
	if (p == "PT")
		m_nShellCommand = FilePrintTo;
	else if (p == "P")
		m_nShellCommand = FilePrint;
	else if (p == "REGISTER" || p == "REGSERVER")
		m_nShellCommand = AppRegister;
	else if (p == "UNREGISTER" || p == "UNREGSERVER")
		m_nShellCommand = AppUnregister;
}

void CCommandLineInfo::ParseParamNotFlag(RCString pszParam) {
	if (m_strFileName.empty())
		m_strFileName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPrinterName.empty())
		m_strPrinterName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strDriverName.empty())
		m_strDriverName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPortName.empty())
		m_strPortName = pszParam;
}

void CCommandLineInfo::ParseLast(bool bLast) {
	if (bLast) {
		if ((m_nShellCommand == FileNew || m_nShellCommand == FileNothing)&& !m_strFileName.empty())
			m_nShellCommand = FileOpen;
		m_bShowSplash = !m_bRunEmbedded && !m_bRunAutomated;
	}
}

void CCommandLineInfo::ParseParam(RCString pszParam, bool bFlag, bool bLast) {
	if (bFlag)
		ParseParamFlag(pszParam);
	else
		ParseParamNotFlag(pszParam);
	ParseLast(bLast);
}




#if !UCFG_COMPLEX_WINAPP
CAppBase *CAppBase::I;
#endif

bool CAppBase::s_bSigBreak;

#if UCFG_COMPLEX_WINAPP

CAppBase::CAppBase()
	: m_bPrintLogo(true) {
	Argc = __argc;
#if UCFG_ARGV_UNICODE
	ProcessArgv(__argc, __wargv);
#else
	ProcessArgv(__argc, __argv);
#endif
	if (!Argv && __wargv)
		ProcessArgv(__argc, __wargv);
	AFX_MODULE_STATE* pModuleState = _AFX_CMDTARGET_GETSTATE();
	pModuleState->m_pCurrentCApp = this;

#if UCFG_USE_RESOURCES
	{
		FileVersionInfo vi;
		if (vi.m_blob.size()) {
			FileDescription = vi.FileDescription;
			SVersion = vi.GetProductVersionN().ToString(2);
			LegalCopyright = vi.LegalCopyright;
			Url = TryGetVersionString(vi, "URL");
		} else
			m_bPrintLogo = false;
	}
#endif
	AttachSelf();
}

String CAppBase::GetCompanyName() {
	String companyName = UCFG_MANUFACTURER;
	DWORD dw;
	try {
		if (GetFileVersionInfoSize((_TCHAR*)(const _TCHAR*)String(AfxGetModuleState()->FileName.native()), &dw)) //!!!
			companyName = FileVersionInfo().CompanyName;
	} catch (RCExc) {
	}
	return companyName;
}

String CAppBase::GetRegPath() {
	return "Software\\"+GetCompanyName()+"\\"+GetInternalName();
}

RegistryKey& CAppBase::get_KeyLM() {
	if (!m_keyLM.Valid())
		m_keyLM.Open(HKEY_LOCAL_MACHINE, GetRegPath(), true);
	return m_keyLM;
}

RegistryKey& CAppBase::get_KeyCU() {
	if (!m_keyCU.Valid())
		m_keyCU.Open(HKEY_CURRENT_USER, GetRegPath(), true);
	return m_keyCU;
}

#endif // UCFG_COMPLEX_WINAPP

#if UCFG_WIN32_FULL

path COperatingSystem::get_WindowsDirectory() {
	TCHAR szPath[_MAX_PATH];
	Win32Check(::GetWindowsDirectory(szPath, size(szPath)));
	return szPath;
}

#endif // UCFG_WIN32_FULL


#if UCFG_WIN32

path AFXAPI GetAppDataManufacturerFolder() {
	path r;
#	if !UCFG_WCE
	try {
		r = path(Environment::GetEnvironmentVariable("APPDATA").c_str());
		if (r.empty())
			r = Environment::GetFolderPath(SpecialFolder::ApplicationData);
	} catch (RCExc) {
		r = System.WindowsDirectory / "Application Data";
	}
#	endif
#if UCFG_COMPLEX_WINAPP
	r /= path(AfxGetApp()->GetCompanyName().c_str());
#else
	r /= UCFG_MANUFACTURER;
#endif
	create_directory(r);
	return r;
}
#endif


String CAppBase::GetInternalName() {
	if (!m_internalName.empty())
		return m_internalName;
#if UCFG_WIN32
	DWORD dw;
	if (GetFileVersionInfoSize((_TCHAR*)(const _TCHAR*)String(AfxGetModuleState()->FileName.native()), &dw))
		return FileVersionInfo().InternalName;
#endif
#ifdef VER_INTERNALNAME_STR
	return VER_INTERNALNAME_STR;
#else
	return "InternalName";
#endif
}

path CAppBase::get_AppDataDir() {
	if (m_appDataDir.empty()) {
#if UCFG_WIN32
		path dir = GetAppDataManufacturerFolder() / path(GetInternalName().c_str());
#elif UCFG_USE_POSIX
		path dir = Environment::GetFolderPath(SpecialFolder::ApplicationData) / ("."+GetInternalName());
#endif
		create_directory(m_appDataDir = dir);
	}
	return m_appDataDir;
}


void CAppBase::ProcessArgv(int argc, argv_char_t *argv[]) {
	Argc = argc;
#if UCFG_WCE
	if (argv) {
		for (int i=0; i<=argc; ++i) {
			String arg = argv[i];
			if (!arg.IsEmpty() && arg.Length>=2 && arg[0]=='\"' && arg[arg.Length-1]=='\"')
				arg = arg.Substring(1, arg.Length-2);
			m_argv.push_back(arg);
			m_argvp.push_back((argv_char_t*)(const argv_char_t*)m_argv[i]);
		}
		Argv = &m_argvp[0];
	}
#else
	Argv.reset(argv);
#endif
}

#if !UCFG_ARGV_UNICODE
void CAppBase::ProcessArgv(int argc, String::value_type *argv[]) {
	Argc = argc;
	if (argv) {
		for (int i=0; i<=argc; ++i) {
			String arg = argv[i];
#if UCFG_WCE
			if (arg.Length>=2 && arg[0]=='\"' && arg[arg.Length-1]=='\"')
				arg = arg.Substring(1, arg.Length-2);
#endif
			m_argv.push_back(arg);
			m_argvp.push_back((argv_char_t*)(const argv_char_t*)m_argv[i]);
		}
		Argv.reset(&m_argvp[0]);
	}
}
#endif // !UCFG_ARGV_UNICODE

void CAppBase::ParseCommandLine(CCommandLineInfo& rCmdInfo) {
	for (int i = 1; i < Argc; i++)
	{
		bool bFlag = false;
		bool bLast = ((i + 1) == Argc);
#ifdef WIN32
		if (__wargv) {
			LPCWSTR pszParam = __wargv[i];
			if (pszParam[0] == '-' || pszParam[0] == '/')
			{
				// remove flag specifier
				bFlag = TRUE;
				++pszParam;
			}
			rCmdInfo.ParseParam(pszParam, bFlag, bLast);
		} else
#endif
		{
			const argv_char_t *pszParam = Argv[i];
			if (pszParam[0] == '-' || pszParam[0] == '/') {
				// remove flag specifier
				bFlag = true;
				++pszParam;
				if (pszParam[0] == '-')
					++pszParam;
			}
			rCmdInfo.ParseParam(pszParam, bFlag, bLast);
		}
	}
}

CAppBase * AFXAPI AfxGetCApp() {
#if UCFG_COMPLEX_WINAPP
	return AfxGetModuleState()->m_pCurrentCApp;
#else
	return CAppBase::I;
#endif
}

#if UCFG_WIN32

class OutputForwarderBuffer : public basic_streambuf<char>, noncopyable {
	typedef basic_streambuf<char> base;
public:
	typedef base StreamBuffer;
	typedef basic_streambuf<wchar_t> WideStreamBuffer;

	OutputForwarderBuffer(StreamBuffer& existingBuffer, WideStreamBuffer* pWideStreamBuffer)
		: base(existingBuffer)
		, m_wideStreamBuf(pWideStreamBuffer)
		, m_state() {
	}
protected:
	streamsize xsputn(const char* s, streamsize n) override {
		size_t bufSize = std::min(size_t(100), size_t(n));
		wchar_t *wbuf = (wchar_t*)alloca(bufSize * sizeof(wchar_t)), *wnext;
		for (const char *next, *fb=s, *fe=s+n;;) {
			Cvt::result rc = m_cvt.in(m_state, fb, fe, next, wbuf, wbuf+bufSize, wnext);
			if (wnext != wbuf)
				m_wideStreamBuf->sputn(wbuf, wnext-wbuf);
			if (exchange(fb, next) == next)
				return next-s;
		}
	}

	int_type overflow(int_type c) override {
		const bool cIsEOF = traits_type::eq_int_type(c, traits_type::eof());
		const int_type failureValue = traits_type::eof();
		const int_type successValue = cIsEOF ? traits_type::not_eof(c) : c;

		if (!cIsEOF) {
			const char_type ch = traits_type::to_char_type(c);
			const streamsize nCharactersWritten  = xsputn(&ch, 1);
			return (nCharactersWritten == 1 ? successValue : failureValue);
		}
		return successValue;
	}
private:
	WideStreamBuffer *m_wideStreamBuf;

	typedef codecvt_utf8_utf16<wchar_t> Cvt;
	Cvt m_cvt;
	Cvt::state_type	m_state;
};

class DirectOutputBuffer : public basic_streambuf<wchar_t>, noncopyable {
public:
	enum StreamId { outputStreamId, errorStreamId, logStreamId };

	DirectOutputBuffer(StreamId streamId = outputStreamId)
		: streamId_(streamId) {
	}
protected:
	streamsize xsputn(const wchar_t* s, streamsize n) override {
		static HANDLE const outputStreamHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		static HANDLE const errorStreamHandle = GetStdHandle(STD_ERROR_HANDLE);

		HANDLE const streamHandle = (streamId_ == outputStreamId ? outputStreamHandle : errorStreamHandle);
		DWORD nCharactersWritten = 0;
		bool writeSucceeded = ::WriteConsole(streamHandle, s, (DWORD)n, &nCharactersWritten, 0);
		return writeSucceeded ? (streamsize)nCharactersWritten : 0;
	}
private:
	StreamId streamId_;
};
#endif // UCFG_WIN32


CConApp::CConApp() {
	s_pApp = this;
}



CConApp *CConApp::s_pApp;

bool CConApp::OnSignal(int sig) {
	s_bSigBreak = true;
	return false;
}

#if !UCFG_WCE

void CConApp::SetSignals() {
	signal(SIGINT, OnSigInt);
#if UCFG_USE_POSIX
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, OnSigInt);
#endif
#ifdef WIN32
	signal(SIGBREAK, OnSigInt);
#endif
}

void __cdecl CConApp::OnSigInt(int sig) {
	TRC(2, sig);
	if (s_pApp->OnSignal(sig))
		signal(sig, OnSigInt);
	else {
		raise(sig);
#ifdef WIN32
		AfxEndThread(0, true); //!!!
#endif
	}
}
#endif // !UCFG_WCE


int CConApp::Main(int argc, argv_char_t *argv[]) {
#if UCFG_WIN32_FULL && UCFG_COUT_REDIRECTOR!='R'
	InitOutRedirectors();
#else
	setlocale(LC_CTYPE, "");
#endif
	if (const char *slevel = getenv("UCFG_TRC")) {
		if (!CTrace::GetOStream())
			CTrace::SetOStream(new CIosStream(clog));
		CTrace::s_nLevel = atoi(slevel);
	}

#if UCFG_USE_POSIX
	cerr << " \b";		// mix of std::cerr && std::wcerr requires this hack
#endif
	ProcessArgv(argc, argv);
	try {
#ifdef WIN32
		AfxWinInit(::GetModuleHandle(0), NULL, ::GetCommandLine(), 0);
#endif
#if UCFG_COMPLEX_WINAPP || !UCFG_EXTENDED
		InitInstance();
#endif
#if !UCFG_WCE
		SetSignals();
#endif
		ParseCommandLine(_self);
		if (m_bPrintLogo && (!FileDescription.empty() || !LegalCopyright.empty() || !Url.empty())) {
			wcerr << FileDescription << ' ' << SVersion << "  " << LegalCopyright;
			if (!Url.empty())
				wcerr << "  " << Url;
			wcerr << endl;
		}
		Execute();
	} catch (const Exception& ex) {
		if (ex.code() != ExtErr::NormalExit) {
			if (ex.code() == ExtErr::SignalBreak)
				Environment::ExitCode = 1;
			else {
				switch (ex.code().value()) {
				case 1:
					Usage();
				case 0:
				case 3:
					Environment::ExitCode = 3;  // Compilation error
					break;
				default:
					wcerr << "\n" << ex.what() << endl;
					Environment::ExitCode = 2;
				}
			}
		}
	} catch (const exception& ex) {
		cerr << "\n" << ex.what() << endl;
		Environment::ExitCode = 2;
	}
#if UCFG_COMPLEX_WINAPP || !UCFG_EXTENDED
	ExitInstance();
#endif
	return Environment::ExitCode;
}


} // Ext::



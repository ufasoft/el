/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <random>

#if UCFG_WIN32
#	include <windows.h>
#	include <wininet.h>
#	include <shlobj.h>

#	include <el/libext/win32/ext-win.h>
#endif

#if UCFG_WIN32_FULL
#	include <el/libext/win32/ext-full-win.h>
#endif


#if UCFG_USE_POSIX
#	include <pwd.h>
#	include <sys/utsname.h>
#	include <sys/wait.h>
#endif

#pragma warning(disable: 4073)
#pragma init_seg(lib)

#if UCFG_WIN32_FULL
#	pragma comment(lib, "shell32")
#endif

#if UCFG_WCE
#	pragma comment(lib, "ceshell")
#endif

#ifndef WIN32
	const GUID GUID_NULL = { 0 };
#endif


namespace Ext { 
using namespace std;

static bool s_bSilentUI;

bool AFXAPI GetSilentUI() { return s_bSilentUI; }
void AFXAPI SetSilentUI(bool b) { s_bSilentUI = b; }

mutex g_mfcCS;

void AFXAPI CCheckErrcode(int en) {
	if (en) {
#if UCFG_HAVE_STRERROR
		ThrowS(HRESULT_FROM_C(en), strerror(en));
#else
		Throw(E_NOTIMPL); //!!!
#endif
	}
}

int AFXAPI CCheck(int i, int allowableError) {
	if (i >= 0)
		return i;
	ASSERT(i == -1);
	int en = errno;
	if (en == allowableError)
		return i;
	Throw(error_code(en, generic_category()));
}

int AFXAPI NegCCheck(int rc) {
	if (rc >= 0)
		return rc;
	Throw(error_code(-rc, generic_category()));
}

EXTAPI void AFXAPI CFileCheck(int i) {
	if (i) {
		Throw(error_code(i, generic_category()));
	}
}


#if UCFG_USE_POSIX
CMapStringRes& MapStringRes() {
	static CMapStringRes s_mapStringRes;
	return s_mapStringRes;
}

static CMapStringRes& s_mapStringRes_not_used = MapStringRes();	// initialize while one thread, don't use

String AFXAPI AfxLoadString(UINT nIDS) {
	CMapStringRes::iterator it = MapStringRes().find(nIDS);
	if (it == MapStringRes().end())
		Throw(E_FAIL);	//!!!
	return it->second;
}
#endif



#if UCFG_WIN32_FULL
CStringVector AFXAPI COperatingSystem::QueryDosDevice(RCString dev) {
	for(int size=_MAX_PATH;; size*=2) {
		TCHAR *buf = (TCHAR*)alloca(sizeof(TCHAR)*size);
		DWORD dw = ::QueryDosDevice(dev, buf, size);
		if (dw && dw < size)
			return AsciizArrayToStringArray(buf);
		Win32Check(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
	}
}
#endif

#ifdef _WIN32
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
#endif

String Environment::GetMachineType() {
#if UCFG_USE_POSIX
	utsname u;
	CCheck(::uname(&u));
	return u.machine;
#elif defined(WIN32)
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL: 	return "x86";
	case PROCESSOR_ARCHITECTURE_MIPS:	return "MIPS";
	case PROCESSOR_ARCHITECTURE_SHX:	return "SHX";
	case PROCESSOR_ARCHITECTURE_ARM:	return "ARM";
	case PROCESSOR_ARCHITECTURE_IA64:	return "IA-64";
#	if UCFG_WIN32_FULL
	case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:	return "IA-32 on Win64";
	case PROCESSOR_ARCHITECTURE_AMD64:	return "x64";
#	endif
	case PROCESSOR_ARCHITECTURE_UNKNOWN:
	default:
		return "Unknown";
	}
#else
	Throw(E_NOTIMPL);
#endif
}

String Environment::GetMachineVersion() {
#if UCFG_USE_POSIX
	utsname u;
	CCheck(::uname(&u));
	return u.machine;
#elif defined(WIN32)
	String s;	
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL:
		switch (si.dwProcessorType) {
		case 586:
			switch (si.wProcessorRevision) {
			case 5895:
				s = "Core 2 Quad";
				break;
			default:
				s = "586";
			}
			break;
		default:
			s = "Intel";
		};
		break;
#if UCFG_WIN32_FULL
	case PROCESSOR_ARCHITECTURE_AMD64:
		s = "AMD64";
		break;
#endif
	case PROCESSOR_ARCHITECTURE_ARM:
		s = "ARM";
		break;
	default:
		s = "Unknown CPU";
		break;
	}
	return s;
#else
	Throw(E_NOTIMPL);
#endif
}

/*!!!

ULONGLONG AFXAPI StrToVersion(RCString s) {
	LONGLONG r = 0;
	istringstream is(s.c_str());
	for (int i=4; i--;) {
		WORD w;
		if (is >> w)
			((WORD*)&r)[i] = w;
		else
			break;
		if (is.get() != '.')
			break;
	}
	return r;
}

String AFXAPI VersionToStr(const LONGLONG& v, int n) {
	WORD ar[4];
	(LONGLONG&)ar[0] = v;
	ostringstream os;
	for (int i=3; i>0 && 3-i < n; i--) {
		if (i != 3)
			os << '.';
		os << ar[i];
	}
	return os.str();
}*/

#if UCFG_USE_REGEX
static StaticRegex s_reVersion("^(\\d+)\\.(\\d+)(\\.(\\d+)(\\.(\\d+))?)?$");

Version::Version(RCString s) {
	cmatch m;
	if (regex_search(s.c_str(), m, *s_reVersion)) {
		Major = atoi(String(m[1]));
		Minor = atoi(String(m[2]));
		Build = m[4].matched ? atoi(String(m[4])) : -1;
		Revision = m[6].matched ? atoi(String(m[6])) : -1;
	} else
		Throw(E_INVALIDARG);
}
#endif

#if UCFG_WIN32

Version Version::FromFileInfo(int ms, int ls, int fieldCount) {
	int ar[4] = { uint16_t(ms>>16), uint16_t(ms), uint16_t(ls>>16), uint16_t(ls) };
	for (int i=4; i-- > fieldCount;)
		ar[i] = -1;
	return Version(ar[0], ar[1], ar[2], ar[3]);
}

#endif

String Version::ToString(int fieldCount) const {
	if (fieldCount<0 || fieldCount>4)
		Throw(E_INVALIDARG);
	int ar[4] = { Major, Minor, Build, Revision };
	ostringstream os;
	for (int i=0; i<fieldCount; ++i) {
		if (ar[i] == -1)
			Throw(E_INVALIDARG);
		if (i)
			os << ".";
		os << ar[i];
	}
	return os.str();
}

String Version::ToString() const {
	return ToString(Revision!=-1 ? 4 : Build!=-1 ? 3 : 2);
}

OperatingSystem::OperatingSystem() {
	Platform = PlatformID::Unix;
#ifdef _WIN32
	COperatingSystem::COsVerInfo vi = System.Version;
	switch (vi.dwPlatformId) {
	case VER_PLATFORM_WIN32s: Platform = PlatformID::Win32S; break;
	case VER_PLATFORM_WIN32_WINDOWS: Platform = PlatformID::Win32Windows; break;
	case VER_PLATFORM_WIN32_NT: Platform = PlatformID::Win32NT; break;
#	if UCFG_WCE
	case VER_PLATFORM_WIN32_CE: Platform = PlatformID::WinCE; break;
#	endif
	}		
	Version = Ext::Version(vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber);
	ServicePack = vi.szCSDVersion;
#endif
}

String OperatingSystem::get_PlatformName() const {
#ifdef _WIN32
#	ifdef UCFG_WIN32
	switch (Platform) {
	case PlatformID::Win32S: return "Win32s";
	case PlatformID::Win32Windows: return "Windows 9x";
	case PlatformID::Win32NT: return "Windows NT";
	case PlatformID::WinCE: return "Windows CE";
	}
#	endif
	return "Windows Native NT";
#else
	utsname u;
	CCheck(::uname(&u));
	return u.sysname;
#endif
}

String OperatingSystem::get_VersionString() const {
	String r = get_PlatformName()+" "+get_VersionName();
	r += " "+Version.ToString();
	if (!ServicePack.empty())
		r += " "+ServicePack;
	return r;
}

String OperatingSystem::get_VersionName() const {
#ifdef _WIN32
	String s;
	switch (int osver = GetOsVersion()) {
#if UCFG_WIN32_FULL
	case OSVER_95:		s = "95";			break;
	case OSVER_98:		s = "98";			break;
	case OSVER_ME:		s = "Millenium";	break;
	case OSVER_NT4:		s = "NT 4";			break;
	case OSVER_2000:	s = "2000";			break;
	case OSVER_XP:		s = "XP";			break;
	case OSVER_SERVER_2003:	s = "Server 2003";	break;
	case OSVER_VISTA:	s = "Vista";		break;
	case OSVER_2008:	s = "Server 2008";	break;
	case OSVER_7:		s = "7";			break;
	case OSVER_2008_R2:	s = "Server 2008 R2";	break;
	case OSVER_8:		s = "8";			break;
	case OSVER_8_1:		s = "8.1";			break;
	case OSVER_10:		s = "10";			break;
	case OSVER_FUTURE:	s = "Unknown Future Version";	break;
#endif
	case OSVER_CE_5:	s = "CE 5";		break;
	case OSVER_CE_6:	s = "CE 5";		break;
	case OSVER_CE_FUTURE:	s = "CE Future";	break;
	default:			s = "Unknown new";	break;
	}
	return s;
#else
	utsname u;
	CCheck(::uname(&u));
	return u.release;
#endif
}

#if UCFG_WIN32
int32_t AFXAPI Environment::TickCount() {
	return (int32_t)::GetTickCount();
}
#endif // UCFG_WIN32

#if UCFG_WIN32_FULL

Environment::CStringsKeeper::CStringsKeeper()
	:	m_p(0)
{
	if (!(m_p = (LPTSTR)::GetEnvironmentStrings()))
		Throw(E_EXT_UnknownWin32Error);
}

Environment::CStringsKeeper::~CStringsKeeper() {
	Win32Check(::FreeEnvironmentStrings(m_p));
}

uint32_t AFXAPI Environment::GetLastInputInfo() {
	LASTINPUTINFO lii = { sizeof lii };
	Win32Check(::GetLastInputInfo(&lii));
	return lii.dwTime;
}

#endif // UCFG_WIN32_FULL

int Environment::ExitCode;
const OperatingSystem Environment::OSVersion;

class Environment Environment;

path Environment::GetFolderPath(SpecialFolder folder) {
#if UCFG_USE_POSIX
	path homedir = GetEnvironmentVariable("HOME");
	switch (folder) {
	case SpecialFolder::Desktop: 			return homedir / "Desktop";
	case SpecialFolder::ApplicationData: 	return homedir / ".config";
	default:
		Throw(E_NOTIMPL);
	}

#elif UCFG_WIN32
	TCHAR path[_MAX_PATH];
#	if UCFG_OLE
	LPITEMIDLIST pidl;
	OleCheck(SHGetSpecialFolderLocation(0, (int)folder, &pidl));
	Win32Check(SHGetPathFromIDList(pidl, path));
	CComPtr<IMalloc> aMalloc;
	OleCheck(::SHGetMalloc(&aMalloc));
	aMalloc->Free(pidl);
#	else
	if (!SHGetSpecialFolderPath(0, path, (int)folder, false))
		Throw(E_FAIL);
#	endif
	return path;
#else
	Throw(E_NOTIMPL);
#endif
}

int Environment::get_ProcessorCount() {
#if UCFG_WIN32
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
#elif UCFG_USE_POSIX
	return std::max((int)sysconf(_SC_NPROCESSORS_ONLN), 1);
#else
	return 1;
#endif
}

String Environment::GetEnvironmentVariable(RCString s) {
#if UCFG_USE_POSIX
	return ::getenv(s);
#elif UCFG_WCE
	return nullptr;
#else
	_TCHAR *p = (_TCHAR*)alloca(256*sizeof(_TCHAR));
	DWORD dw = ::GetEnvironmentVariable(s, p, 256);
	if (dw > 256) {
		p = (_TCHAR*)alloca(dw*sizeof(_TCHAR));
		dw = ::GetEnvironmentVariable(s, p, dw);
	}
	if (dw)
		return p;
	Win32Check(GetLastError() == ERROR_ENVVAR_NOT_FOUND);
	return nullptr;
#endif
}

vector<String> ParseCommandLine(RCString s) {
	vector<String> r;
	bool bQuoting = false, bSingleQuote = false, bHasArg = false;
	String arg;
	for (const char *p=s; *p; ++p) {
		if (exchange(bSingleQuote, false))
			arg += String(*p);
		else {
			switch (*p) {
			case '\"':
				bQuoting = !bQuoting;
				bHasArg = true;
				break;
			case '\\':
				bSingleQuote = true;
				break;			
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				if (!bQuoting) {
					if (bHasArg || !arg.empty()) {
						r.push_back(exchange(arg, String()));
						bHasArg = false;
					}
					break;
				}
			default:
				arg += String(*p);
			}
		}
	}
	if (bHasArg || !arg.empty())
		r.push_back(arg);
	return r;
}

#if !UCFG_WCE

String AFXAPI Environment::CommandLine() {
#if UCFG_USE_POSIX
	return File::ReadAllText("/proc/self/cmdline");
#else
	return GetCommandLineW();
#endif
}

vector<String> AFXAPI Environment::GetCommandLineArgs() {
	return ParseCommandLine(CommandLine());
}

path Environment::SystemDirectory() {
#if UCFG_USE_POSIX
	return "";
#else
	TCHAR szPath[_MAX_PATH];
	Win32Check(::GetSystemDirectory(szPath, size(szPath)));
	return szPath;
#endif
}

void Environment::SetEnvironmentVariable(RCString name, RCString val) {
#if UCFG_USE_POSIX
	String s = name+"="+val;
	CCheck(::putenv(strdup(s)) != 0 ? -1 : 0);
#else
	Win32Check(::SetEnvironmentVariable(name, val));
#endif
}

String Environment::ExpandEnvironmentVariables(RCString name) {
#if UCFG_USE_POSIX
	Throw(E_NOTIMPL);
#else
	vector<TCHAR> buf(32768);																	// in Heap to save Stack
	DWORD r = Win32Check(::ExpandEnvironmentStrings(name, &buf[0], buf.size()));
	if (r > buf.size())
		Throw(E_FAIL);
	return buf;
#endif
}

#if UCFG_USE_POSIX
extern "C" {
	extern char **environ;
}
#endif

map<String, String> Environment::GetEnvironmentVariables() {
	map<String, String> m;
#if UCFG_USE_POSIX
	for (char **p = environ; *p; ++p) {
		if (char *q = strchr(*p, '='))
			m[String(*p, q-*p)] = q + 1;
		else
			m[*p] = "";			
	}
#else
	CStringsKeeper sk;
	for (LPTSTR p=sk.m_p; *p; p+=_tcslen(p)+1) {
		if (TCHAR *q = _tcschr(p, '='))
			m[String(p, q-p)] = q + 1;
		else
			m[p] = "";			
	}
#endif
	return m;
}


int Environment::get_ProcessorCoreCount() {
#if UCFG_WIN32_FULL
	typedef BOOL (WINAPI *PFN_GetLogicalProcessorInformationEx)(LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer, PDWORD ReturnedLength);

	DlProcWrap<PFN_GetLogicalProcessorInformationEx> pfn("KERNEL32.DLL", "GetLogicalProcessorInformationEx");
	if (pfn) {
		DWORD nBytes = 0;
		Win32Check(pfn(RelationProcessorCore, 0, &nBytes), ERROR_INSUFFICIENT_BUFFER);
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX  *buf = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)alloca(nBytes);
		Win32Check(pfn(RelationProcessorCore, buf, &nBytes));
		return nBytes/buf->Size;
	}
#endif
#if UCFG_CPU_X86_X64
	int a[4];
	Cpuid(a, 0);
	if (a[0] >= 4) {
		Cpuid(a, 4);
		return ((a[0] >> 26) & 0x3F) + 1;
	}
#endif
	return ProcessorCount;
}

String COperatingSystem::get_UserName() {
#if UCFG_USE_POSIX
	return getpwuid(getuid())->pw_name;
#else
	TCHAR buf[255];
	DWORD len = size(buf);
	Win32Check(::GetUserName(buf, &len));
	return buf;
#endif
}

#endif  // !UCFG_WCE

String NameValueCollection::ToString() const {
	ostringstream os;
	for (const_iterator i=begin(); i!=end(); ++i) {
		os << i->first << ": " << String::Join(",", i->second) << "\r\n";
	}
	return os.str();
}

String HttpUtility::UrlEncode(RCString s, Encoding& enc) {
	Blob blob = enc.GetBytes(s);
	ostringstream os;
	for (size_t i=0; i<blob.Size; ++i)	{
		char ch = blob.constData()[i];
		if (ch == ' ')
			os << '+';
		else if (isalnum(ch) || strchr("!'()*-._", ch))
			os << ch;
		else
			os << '%' << setw(2) << setfill('0') << hex << (int)(byte)ch;
	}		
	return os.str();
}

String HttpUtility::UrlDecode(RCString s, Encoding& enc) {
	Blob blob = enc.GetBytes(s);
	ostringstream os;
	for (size_t i=0; i<blob.Size; ++i)	 {
		char ch = blob.constData()[i];
		switch (ch) {
		case '+':
			os << ' ';
			break;
		case '%':
			{
				if (i+2 > blob.Size)
					Throw(E_FAIL);
				String c1((char)blob.constData()[++i]),
					c2((char)blob.constData()[++i]);
				os << (char)Convert::ToUInt32(c1+c2, 16);
			}
			break;
		default:
			os << ch;
		}
	}		
	return os.str();
}

#if UCFG_USE_REGEX
static StaticRegex s_reNameValue("([^=]+)=(.*)");

NameValueCollection HttpUtility::ParseQueryString(RCString query) {
	vector<String> params = query.Split("&");
	NameValueCollection r;
	for (size_t i=0; i<params.size(); ++i) {
		cmatch m;
		if (!regex_search(params[i].c_str(), m, *s_reNameValue))
			Throw(E_FAIL);
		String name = UrlDecode(m[1]),
			value = UrlDecode(m[2]);
		r[name.ToUpper()].push_back(value);
	}
	return r;
}
#endif

// RFC 4648

class CBase64Table : public vector<int> {
public:
	static const char s_toBase64[];

	CBase64Table()
		:	vector<int>((size_t)256, EOF)
	{
		for (byte i=(byte)strlen(s_toBase64); i--;) // to eliminate trailing zero
			_self[s_toBase64[i]] = i;
	}
};

static InterlockedSingleton<CBase64Table> s_pBase64Table;

const char CBase64Table::s_toBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int GetBase64Val(istream& is) {
	for (char ch; is.get(ch);)
		if (!isspace(ch))
			return (*s_pBase64Table)[ch];
	return EOF;
}

Blob Convert::FromBase64String(RCString s) {  
	istringstream is(s.c_str());
	MemoryStream ms;
	BinaryWriter wr(ms);
	while (true) {
		int ch0 = GetBase64Val(is),
			ch1 = GetBase64Val(is);
		if (ch0 == EOF || ch1 == EOF)
			break;
		wr << byte(ch0 << 2 | ch1 >> 4);
		int ch2 = GetBase64Val(is);
		if (ch2 == EOF)
			break;
		wr << byte(ch1 << 4 | ch2 >> 2);
		int ch3 = GetBase64Val(is);
		if (ch3 == EOF)
			break;
		wr << byte(ch2 << 6 | ch3);
	}
	return ms.Blob;
}

String Convert::ToBase64String(const ConstBuf& mb) {
	//!!!R	CBase64Table::s_toBase64;
	vector<String::value_type> v;
	const byte *p = mb.P;
	for (size_t i=mb.Size/3; i--; p+=3) {
		uint32_t dw = (p[0]<<16) | (p[1]<<8) | p[2];
		v.push_back(CBase64Table::s_toBase64[(dw>>18) & 0x3F]);
		v.push_back(CBase64Table::s_toBase64[(dw>>12) & 0x3F]);
		v.push_back(CBase64Table::s_toBase64[(dw>>6) & 0x3F]);
		v.push_back(CBase64Table::s_toBase64[dw & 0x3F]);
	}
	if (size_t rem = mb.Size % 3) {
		v.push_back(CBase64Table::s_toBase64[(p[0]>>2) & 0x3F]);
		switch (rem) {
		case 1:
			v.push_back(CBase64Table::s_toBase64[(p[0]<<4) & 0x3F]);
			v.push_back('=');
			break;
		case 2:
			v.push_back(CBase64Table::s_toBase64[((p[0]<<4) | (p[1]>>4)) & 0x3F]);
			v.push_back(CBase64Table::s_toBase64[(p[1]<<2) & 0x3F]);
		}
		v.push_back('=');
	}
	return String(&v[0], v.size());
}

class BitWriteStream {
public:
	Stream& Base;
	int Offset;

	BitWriteStream(Stream& bas)
	:	Base(bas)
	,	Offset(0)
	,	m_prevValue(0)
	{}

	void Write(int count, uint32_t value) {
		ASSERT(count <= 8);
		m_prevValue |= value << (8-count) >> Offset;
		if (Offset + count >= 8) {
			Base.WriteBuffer(&m_prevValue, 1);
			m_prevValue = byte(value << (16 - Offset - count));
		}			
		Offset = (Offset + count) % 8;
	}

	void Flush() {
		if (Offset)
			Base.WriteBuffer(&m_prevValue, 1);
		Offset = 0;
		m_prevValue = 0;
	}
public:
	byte m_prevValue;
};

static Blob FromBaseX(int charsInGroup, RCString s, const vector<int>& valTable) {
	MemoryStream ms;
	BitWriteStream bs(ms);
	for (size_t i=0; i<s.size(); i+=charsInGroup) {
		for (size_t j=0; j<charsInGroup; ++j) {
			wchar_t ch = s[i+j];
			if ('=' == ch) {
				bs.Offset = 0;
				bs.Flush();
				break;
			}
			if (uint16_t(ch) >= 256)
				Throw(E_INVALIDARG);
			bs.Write(5, valTable[ch]);
		}
	}
	if (bs.Offset)
		Throw(E_INVALIDARG);
	return ms;
}

static String ToBaseX(int charsInGroup, int bytesInGroup, const ConstBuf& mb, const char *table) {
	ostringstream os;
	const int bitsInGroup = bytesInGroup * 8 / charsInGroup, 
		mask = (1<<bitsInGroup) - 1;
	for (size_t i=0; i<mb.Size;) {
		int nbits = 0;
		uint64_t val = 0;
		for (int j=bytesInGroup; j-- && i<mb.Size; nbits+=8)
			val |= uint64_t(mb.P[i++]) << (j*8);
		for (int j=charsInGroup; j--; nbits-=bitsInGroup)
			os.put(nbits>0 ?table[(val >> (j*bitsInGroup)) & mask] : '=');
	}
	return os.str();
}

class CBase32Table : public vector<int> {
public:
	static const char s_toBase32[];

	CBase32Table()
		: vector<int>((size_t)256, EOF) {
		for (byte i=(byte)strlen(s_toBase32); i--;) // to eliminate trailing zero
			_self[s_toBase32[i]] = i;
	}
};

static InterlockedSingleton<CBase32Table> s_pBase32Table;

const char CBase32Table::s_toBase32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

Blob Convert::FromBase32String(RCString s) {
	return FromBaseX(8, s.ToUpper(), *s_pBase32Table);
}

String Convert::ToBase32String(const ConstBuf& mb) {
	return ToBaseX(8, 5, mb, CBase32Table::s_toBase32);
}


#if UCFG_USE_REGEX
static StaticRegex s_reGuid("\\{([0-9A-Fa-f]{8,8})-([0-9A-Fa-f]{4,4})-([0-9A-Fa-f]{4,4})-([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})-([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})\\}");

Guid::Guid(RCString s) {
#if UCFG_COM
	OleCheck(::CLSIDFromString(Bstr(s), this));	
#else
	cmatch m;
	if (regex_search(s.c_str(), m, *s_reGuid)) {
		Data1 = Convert::ToUInt32(m[1], 16);
		Data2 = Convert::ToUInt16(m[2], 16);
		Data3 = Convert::ToUInt16(m[3], 16);
		for (int i=0; i<8; ++i)
			Data4[i] = (byte)Convert::ToUInt16(m[4+i], 16);
	} else
		Throw(E_FAIL);
#endif
}
#endif

Guid Guid::NewGuid() {
	Guid guid;
#if UCFG_COM
	OleCheck(::CoCreateGuid(&guid));
#else
	Random rng;
	rng.NextBytes(Buf((byte*)&guid, sizeof(GUID)));
#endif
	return guid;	
}

String Guid::ToString(RCString format) const {
	String s;
#ifdef WIN32
	wchar_t buf[40];
	StringFromGUID2(_self, buf, size(buf));
	s = buf;
#else
	s = "{"+Convert::ToString(Data1, "X8")+"-"+Convert::ToString(Data2, "X4")+"-"+Convert::ToString(Data3, "X4")+"-";			//!!!A
	for (int i=0; i<2; ++i)
		s += Convert::ToString(Data4[i], "X2");
	s += "-";
	for (int i=2; i<8; ++i)
		s += Convert::ToString(Data4[i], "X2");
	s += "}";
#endif
	if ("B" == format)
		return s;
	s = s.substr(1, s.length()-2);
	if (format.empty() || "D"==format)
		return s;
	if ("N" == format)
		return s.substr(0, 8) + s.substr(9, 4) + s.substr(14, 4) + s.substr(19, 4)+s.substr(24);
	if ("P" == format)
		return "("+s+")";
	Throw(E_FAIL);
}


CResID::CResID(UINT nResId)
:	m_resId(nResId)
{
}

CResID::CResID(const char *lpName)
    :	m_resId(0)
{
	_self = lpName;
}

CResID::CResID(const String::value_type *lpName)
	:	m_resId(0)
{
	_self = lpName;
}

CResID& CResID::operator=(const char *resId) {
	m_name = "";
	m_resId = 0;
	if (HIWORD(resId))
		m_name = resId;
	else
		m_resId = (uint32_t)(uintptr_t)resId;
	return _self;
}

CResID& CResID::operator=(const String::value_type *resId) {
	m_name = "";
	m_resId = 0;
	if (HIWORD(resId))
		m_name = resId;
	else
		m_resId = (uint32_t)(uintptr_t)resId;
	return _self;
}

AFX_API CResID& CResID::operator=(RCString resId) {
	m_resId = 0;
	m_name = resId;
	return _self;
}

CResID::operator const char *() const {
	if (m_name.empty())
		return (const char*)m_resId;
	else
		return m_name;
}

CResID::operator const String::value_type *() const {
	if (m_name.empty())
		return (const String::value_type *)m_resId;
	else
		return m_name;
}

#ifdef WIN32
CResID::operator UINT() const {
	return (uint32_t)(uintptr_t)(operator LPCTSTR());
}
#endif

String CResID::ToString() const {
	return m_name.empty() ? Convert::ToString((DWORD)m_resId) : m_name;
}

void CResID::Read(const BinaryReader& rd) {
	rd >> m_resId >> m_name;
}

void CResID::Write(BinaryWriter& wr) const {
	wr << m_resId << m_name;
}


int AFXAPI Rand() {
#if UCFG_USE_POSIX
	return (unsigned int)time(0) % ((unsigned int)RAND_MAX+1);
#else
	return int (System.PerformanceCounter % (RAND_MAX+1));
#endif
}

Random::Random(int seed)
	: m_prngeng(new default_random_engine(seed))
{
}

static std::default_random_engine *Rngeng(Random& r) {
	return static_cast<default_random_engine*>(r.m_prngeng);
}

Random::~Random() {
	delete Rngeng(_self);
}

uint16_t Random::NextWord() {
	return (uint16_t) uniform_int_distribution<int>(0, 65535) (*Rngeng(_self));


	//!!!R return uint16_t((m_seed = m_seed * 214013L + 2531011L) >> 16);
}

void Random::NextBytes(const Buf& mb) {
	uniform_int_distribution<int> dist(0, 255);
	for (size_t i = 0; i < mb.Size; i++)
		mb.P[i] = (byte)dist(*Rngeng(_self));
}

int Random::Next() {
	return uniform_int_distribution<int>(numeric_limits<int>::min(), numeric_limits<int>::max()) (*Rngeng(_self));
}

int Random::Next(int maxValue) {
	return uniform_int_distribution<int>(0, maxValue-1) (*Rngeng(_self));
}

double Random::NextDouble() {
	return uniform_real_distribution<double>(0, 1) (*Rngeng(_self));

/*!!!R
	STATIC_ASSERT(DBL_MANT_DIG < 64);

	uint64_t n;
	NextBytes(Buf((byte*)&n, sizeof n));

	n = (n >> (64 - (DBL_MANT_DIG-1))) | (uint64_t(1) << (DBL_MANT_DIG-1));
	return ldexp(double(n), -(DBL_MANT_DIG-1)) - 1.0;
	*/
}

void CAnnoyer::OnAnnoy() {
	if (m_iAnnoy)
		m_iAnnoy->OnAnnoy();
}

void CAnnoyer::Request() {
	DateTime now = DateTime::UtcNow();
	if (now-m_prev > m_period) {
		OnAnnoy();
		m_prev = now;
		m_period += m_period;
	}
}

template <typename W>
hashval ComputeHashImp(HashAlgorithm& algo, Stream& stm) {
	W hash[8];
	algo.InitHash(hash);
	byte buf[16*sizeof(W)];
	uint64_t len = 0, counter;
	bool bLast = false;
	while (true) {
		ZeroStruct(buf);
		if (bLast) {
			counter = 0;
			break;
		}
		size_t cb = stm.Read(buf, sizeof buf);
		if (cb < sizeof buf) {
			buf[cb] = 0x80;
			bLast = true;
		}
		len += cb;
		counter = len << 3;
		if (bLast && (len & (sizeof buf - 1)) < sizeof buf - int(bool(algo.IsHaifa)) - sizeof(W)*2) {
			if (!(len & (sizeof buf - 1)))
				counter = 0;
			break;
		}
		for (int j=0; j<16; ++j)
			((W*)buf)[j] = betoh(((W*)buf)[j]);
		algo.HashBlock(hash, buf, counter);
	}
	if (algo.IsHaifa)
		buf[sizeof buf - sizeof(W)*2- 1] = 1;
	len <<= 3;
	for (int i=0; i<8; ++i)
		buf[sizeof buf - 1 -i] = byte(len>>(i*8));
	for (int j=0; j<16; ++j)
		((W*)buf)[j] = betoh(((W*)buf)[j]);
	algo.HashBlock(hash, buf, counter);
	for (int j=0; j<8; ++j)
		hash[j] = htobe(hash[j]);
	return hashval((const byte*)hash, algo.HashSize);
}

hashval HashAlgorithm::ComputeHash(Stream& stm) {
	return Is64Bit ? ComputeHashImp<uint64_t>(_self, stm) : ComputeHashImp<uint32_t>(_self, stm);
}

hashval HashAlgorithm::ComputeHash(const ConstBuf& mb) {
	CMemReadStream stm(mb);
	return ComputeHash(stm);
}

// RFC 2104
hashval HMAC(HashAlgorithm& halgo, const ConstBuf& key, const ConstBuf& text) {
	ConstBuf k = key;
	hashval hk;
	if (key.Size > halgo.BlockSize) {
		hk = halgo.ComputeHash(key);
		k = hk;
	}
	size_t size = halgo.BlockSize + max(halgo.BlockSize, text.Size);
	byte *buf = (byte*)alloca(size);
	for (size_t i=0; i<halgo.BlockSize; ++i)
		buf[i] = i<k.Size ? k.P[i] ^ 0x36 : 0x36;
	memcpy(buf+halgo.BlockSize, text.P, text.Size);
	hashval hv = halgo.ComputeHash(ConstBuf(buf, halgo.BlockSize+text.Size));
	for (size_t i=0; i<halgo.BlockSize; ++i)
		buf[i] = i<k.Size ? k.P[i] ^ 0x5C : 0x5C;
	memcpy(buf+halgo.BlockSize, hv.constData(), hv.size());
	return halgo.ComputeHash(ConstBuf(buf, halgo.BlockSize+hv.size()));
}


static uint32_t s_crcTable[256];

static bool Crc32GenerateTable() {
	uint32_t poly = 0xEDB88320;
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t r = i;
		for (int j = 0; j < 8; j++)
			r = (r >> 1) ^ (poly & ~((r & 1) - 1));
		s_crcTable[i] = r;
	}
	return true;
}

hashval Crc32::ComputeHash(Stream& stm) {
	static once_flag once;
	call_once(once, &Crc32GenerateTable);

	uint32_t val = 0xFFFFFFFF;
	for (int v; (v=stm.ReadByte())!=-1;)
		val = s_crcTable[(val ^ v) & 0xFF] ^ (val >> 8);
	val = ~val;
	return hashval((const byte*)&val, sizeof val);
}

CMessageProcessor g_messageProcessor;

const DWORD //!!!E_EXT_BASE = 0x80040000 | 10000,
	E_EXT_UPPER = E_EXT_BASE+0xFFFF;

CMessageProcessor::CMessageProcessor() {
	m_default.Init(0, uint32_t(-1), System.get_ExeFilePath().stem());
#ifdef X_AFXDLL	 //!!!
	RegisterModule(E_EXT_BASE, E_EXT_UPPER, Path::GetFileName(AfxGetModuleState()->FileName));
#endif
}

CMessageRange::CMessageRange() {
	g_messageProcessor.m_ranges.push_back(this);  
}

CMessageRange::~CMessageRange() {
	g_messageProcessor.m_ranges.erase(std::remove(g_messageProcessor.m_ranges.begin(), g_messageProcessor.m_ranges.end(), this), g_messageProcessor.m_ranges.end());
}

void CMessageProcessor::CModuleInfo::Init(uint32_t lowerCode, uint32_t upperCode, const path& moduleName) {
	m_lowerCode = lowerCode;
	m_upperCode = upperCode;
	m_moduleName = moduleName;
	if (!m_moduleName.has_extension()) {
#if UCFG_USE_POSIX
		m_moduleName += ".cat";
#elif UCFG_WIN32
		m_moduleName += ".dll";
#endif
	}
}

String CMessageProcessor::CModuleInfo::GetMessage(HRESULT hr) {
	if (uint32_t(hr) < m_lowerCode || uint32_t(hr) >= m_upperCode)
		return nullptr;
#if UCFG_USE_POSIX
	if (!m_mcat) {
		if (exchange(m_bCheckedOpen, true) || !exists(m_moduleName) && !exists(System.GetExeDir() / m_moduleName))
			return nullptr;
		try {
			m_mcat.Open(m_moduleName.c_str());
		} catch (RCExc) {
			if (!m_moduleName.parent_path().empty())
				return nullptr;
			try {
				m_mcat.Open((System.get_ExeFilePath().parent_path() / m_moduleName).c_str());
			} catch (RCExc) {
				return nullptr;
			}
		}		
	}
	return m_mcat.GetMessage((hr>>16) & 0xFF, hr & 0xFFFF);
#elif UCFG_WIN32
	TCHAR buf[256];
	if (::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY, LPCVOID(GetModuleHandle(m_moduleName.native())), hr, 0, buf, sizeof buf, 0))
		return buf;
	else
		return nullptr;
#else
	return nullptr;
#endif
}

void CMessageProcessor::SetParam(RCString s) {
	String *ps = g_messageProcessor.m_param;
	if (!ps)
		g_messageProcessor.m_param.reset(ps = new String);
	*ps = s;
}

#if UCFG_COM

HRESULT AFXAPI AfxProcessError(HRESULT hr, EXCEPINFO *pexcepinfo) {
	return CMessageProcessor::Process(hr, pexcepinfo);
}

HRESULT CMessageProcessor::Process(HRESULT hr, EXCEPINFO *pexcepinfo) {
	//!!!  HRESULT hr = ProcessOleException(e);
	if (pexcepinfo) {
		pexcepinfo->bstrDescription = HResultToMessage(hr).AllocSysString();
		pexcepinfo->wCode = 0;
		pexcepinfo->scode = hr;
		return DISP_E_EXCEPTION;
	} else
		return hr;
}
#endif

void CMessageProcessor::RegisterModule(DWORD lowerCode, DWORD upperCode, RCString moduleName) {
	g_messageProcessor.CheckStandard();
	CModuleInfo info;
	info.Init(lowerCode, upperCode, moduleName);
	g_messageProcessor.m_vec.push_back(info);
}

String CMessageProcessor::ProcessInst(HRESULT hr) {
	CheckStandard();
	char hex[30];
	sprintf(hex, "Error %8.8X", hr);
	TCHAR buf[256];
	char *ar[5] = {0, 0, 0, 0, 0};
	char **p = 0;
	String *ps = m_param;
	if (ps && !ps->empty()) {
		ar[0] = (char*)ps->c_str();
		p = ar;
	}
	for (size_t i = 0; i<m_ranges.size(); i++) {
		String msg = m_ranges[i]->CheckMessage(hr);
		if (!msg.empty())
			return String(hex) + L":  " + msg;
	}

#if UCFG_WIN32
	if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, 0, hr, 0, buf, sizeof buf, p))
		return String(hex)+ ":  " + buf;
	switch (HRESULT_FACILITY(hr)) {
	case FACILITY_INTERNET:
		if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_IGNORE_INSERTS, LPCVOID(GetModuleHandle(_T("urlmon.dll"))),
			hr, 0, buf, sizeof buf, 0))
			return String(hex) + ":  " + buf;
		break;
	case FACILITY_WIN32:
		return win32_category().message(WORD(HRESULT_CODE(hr)));
	}
#endif // UCFG_WIN32

	for (vector<CModuleInfo>::iterator i(m_vec.begin()); i!=m_vec.end(); ++i) {
		String msg = i->GetMessage(hr);
		if (!!msg)
			return String(hex) + ":  " + msg;
	}
	String msg = m_default.GetMessage(hr);
	if (!!msg)
		return msg;
#if UCFG_HAVE_STRERROR
	if (HRESULT_FACILITY(hr) == FACILITY_C) {
		if (const char *s = strerror(HRESULT_CODE(hr)))
			return String(hex) + ":  " + s;
	}
#endif
#if UCFG_WIN32
	if (::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_IGNORE_INSERTS, LPCVOID(_afxBaseModuleState.m_hCurrentResourceHandle), hr, 0, buf, sizeof buf, 0))
		return String(hex) + ":  " + buf;
#endif
	return hex;
}

String AFXAPI HResultToMessage(HRESULT hr) {
	return g_messageProcessor.ProcessInst(hr);
}

void CMessageProcessor::CheckStandard() {
#if UCFG_EXTENDED
	if (m_vec.size() == 0) {
		CModuleInfo info;
		info.m_lowerCode = E_EXT_BASE;
		info.m_upperCode = (DWORD)E_EXT_UPPER;

#ifdef _AFXDLL
		AFX_MODULE_STATE *pMS = AfxGetStaticModuleState();
#else
		AFX_MODULE_STATE *pMS = &_afxBaseModuleState;
#endif
		info.m_moduleName = pMS->FileName.filename();
		m_vec.push_back(info);
	}
#endif
}

bool PersistentCache::Lookup(RCString key, Blob& blob) {
	path fn = AfxGetCApp()->get_AppDataDir() / ".cache" / key;
	create_directories(fn.parent_path());
	if (!exists(fn))
		return false;
	FileStream fs(fn.c_str(), FileMode::Open, FileAccess::Read);
	blob.Size = (size_t)fs.Length;
	fs.ReadBuffer(blob.data(), blob.Size);
	return true;
}

void PersistentCache::Set(RCString key, const ConstBuf& mb) {
	path fn = AfxGetCApp()->get_AppDataDir() / ".cache" / key;
	create_directories(fn.parent_path());
	FileStream fs(fn, FileMode::Create, FileAccess::Write);
	fs.WriteBuffer(mb.P, mb.Size);

	/*!!!
#if UCFG_WIN32
	RegistryKey rk(AfxGetCApp()->KeyCU, "cache");
	rk.SetValue(key, mb);
#endif*/
}

#if UCFG_WIN32

ResourceObj::ResourceObj(HMODULE hModule, HRSRC hRsrc)
	:	m_hModule(hModule)
	,	m_hRsrc(hRsrc)
	,	m_p(0)
{
	Win32Check(bool(m_hglbResource = ::LoadResource(m_hModule, m_hRsrc)));
}

ResourceObj::~ResourceObj() {
#	if UCFG_WIN32_FULL
	Win32Check(! ::FreeResource(m_hglbResource));
#	endif
}
#endif // UCFG_WIN32

Resource::Resource(const CResID& resID, const CResID& resType, HMODULE hModule) {
#if UCFG_WIN32
	if (!hModule)
		hModule = AfxFindResourceHandle(resID, resType);
	HRSRC hRsrc = ::FindResource(hModule, resID, resType);
	Win32Check(hRsrc != 0);
	m_pimpl = new ResourceObj(hModule, hRsrc);
#else
	m_blob = File::ReadAllBytes(System.get_ExeFilePath().parent_path() / resID.ToString());
#endif
}

const void *Resource::get_Data() {
#if UCFG_WIN32
	if (!m_pimpl->m_p)
		m_pimpl->m_p = ::LockResource(m_pimpl->m_hglbResource);
	return m_pimpl->m_p;
#else
	return m_blob.constData();
#endif
}

size_t Resource::get_Size() {
#if UCFG_WIN32
	return Win32Check(::SizeofResource(m_pimpl->m_hModule, m_pimpl->m_hRsrc));
#else
	return m_blob.Size;
#endif
}

EXT_DATA const Temperature Temperature::Zero = Temperature::FromKelvin(0);

String Temperature::ToString() const {
	return Convert::ToString(ToCelsius())+" C";
}

EXT_API mutex g_mtxObjectCounter;
EXT_API CTiToCounter g_tiToCounter;

void AFXAPI LogObjectCounters(bool fFull) {
#if !defined(_DEBUG) || !defined(_M_X64)		//!!!TODO
	static int s_i;
	if (!(++s_i & 0x1FF) || fFull) {
		EXT_LOCK (g_mtxObjectCounter) {
			for (CTiToCounter::iterator it=g_tiToCounter.begin(), e=g_tiToCounter.end(); it!=e; ++it) {
				TRC(2, it->second << "\t" << it->first->name());
			}
		}
	}
#endif
}

ProcessStartInfo::ProcessStartInfo(const path& fileName, RCString arguments)
	:	Flags(0)
	,	FileName(fileName)
	,	Arguments(arguments)
#if !UCFG_WCE
	,	EnvironmentVariables(Environment::GetEnvironmentVariables())
#endif
{}

ProcessObj::ProcessObj()
	:	SafeHandle(0, false)
	,	m_stat_loc(0)
{
	CommonInit();
}

#if UCFG_WIN32
ProcessObj::ProcessObj(HANDLE handle, bool bOwn)
	:	SafeHandle(0, false)
	,	m_stat_loc(0)
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
	WaitWithMS(ms, (HANDLE)(intptr_t)HandleAccess(*this));
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
	if (!(m_pid = CCheck(::fork()))) {
		vector<const char *> argv;
		String filename = StartInfo.FileName.native();
		argv.push_back(filename);
		vector<String> v = ParseCommandLine(StartInfo.Arguments);
		TRC(5, "Args:\n" << v << "----------------");
		for (size_t i=0; i<v.size(); ++i)
			argv.push_back(v[i]);
		argv.push_back(nullptr);

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
	ptr<CWinThread> pThread(new CWinThread);
	pThread->Attach((intptr_t)pi.hThread, pi.dwThreadId);
#else // UCFG_WIN32
	Throw(E_NOTIMPL);
#endif
	return true;
}

Process AFXAPI Process::Start(const ProcessStartInfo& psi) {
	TRC(2, psi.FileName << " " << psi.Arguments);

	Process r;
	r.m_pimpl = new ProcessObj;
	r.m_pimpl->StartInfo = psi;
	r.m_pimpl->Start();
	return r;
}

String Process::get_ProcessName() {
#if UCFG_USE_POSIX
	char szModule[PATH_MAX];
	memset(szModule, 0, sizeof szModule);
	CCheck(::readlink(String(EXT_STR("/proc/" << get_ID() << "/exe")), szModule, sizeof(szModule)));
	return path(szModule).filename();
#else
	TCHAR buf[MAX_PATH];
	Win32Check(::GetModuleBaseName((HANDLE)(intptr_t)Handle(*m_pimpl), 0, buf, size(buf)));
	path r = buf;
	String ext = ToLower(r.extension());
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

HRESULT AFXAPI ToHResult(const system_error& ex) {
	const error_code& ec = ex.code();
	const error_category& cat = ec.category();
	int ecode = ec.value();
	if (cat == generic_category())
		return HRESULT_FROM_C(ecode);
#if UCFG_WIN32
	else if (cat == win32_category())
		return (HRESULT)(((ecode)& 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);
#endif
	else if (cat == system_category())														// == win32_category() on Windows
		return (HRESULT)(((ecode)& 0x0000FFFF) | (FACILITY_OS << 16) | 0x80000000);
	else if (cat == hresult_category())
		return ecode;
	else
		return (HRESULT)(((ecode)& 0x0000FFFF) | (FACILITY_UNKNOWN << 16) | 0x80000000);			//!!! category info lost here
}

HRESULT AFXAPI HResultInCatch(RCExc) {		// arg not used
	try {
		throw;
	} catch (const system_error& ex) {
		return ToHResult(ex);
	} catch (const bad_alloc&) {
		return E_OUTOFMEMORY;
	} catch (invalid_argument&) {
		return E_INVALIDARG;
	} catch (bad_cast&) {
		return E_EXT_InvalidCast;
	} catch (const out_of_range&) {
		return E_EXT_IndexOutOfRange;
	} catch (const exception&) {
		return E_FAIL;
	}
}


} // Ext::

#if UCFG_USE_REGEX

EXT_API const std::regex_constants::syntax_option_type Ext::RegexTraits::DefaultB = std::regex_constants::ECMAScript;


namespace std {

EXT_API bool AFXAPI regex_searchImpl(Ext::String::const_iterator bs,  Ext::String::const_iterator es, Ext::Smatch *m, const wregex& re, bool bMatch, regex_constants::match_flag_type flags, Ext::String::const_iterator org) {
	wcmatch wm;
#if defined(_NATIVE_WCHAR_T_DEFINED) && UCFG_STRING_CHAR/8 == __SIZEOF_WCHAR_T__
	const wchar_t *b = &*bs, *e = &*es;
#else
	wstring ws(bs, es);
	const wchar_t *b = &*ws.begin(),
		          *e = &*ws.end();
#endif
	bool r = bMatch ? regex_match<const wchar_t*, wcmatch::allocator_type, wchar_t>(b, e, wm, re, flags)
					: regex_search<const wchar_t*, wcmatch::allocator_type, wchar_t>(b, e, wm, re, flags);
	if (r && m) {
		m->m_ready = true; //!!!? wm.ready();
		m->m_org = org;
		m->m_prefix.Init(wm.prefix(), m->m_org, b);
		m->m_suffix.Init(wm.suffix(), m->m_org, b);
		m->Resize(wm.size());
		for (size_t i=0; i<wm.size(); ++i)
			m->GetSubMatch(i).Init(wm[i], bs, b);
	}
	return r;
}

} // std::
#endif // UCFG_USE_REGEX





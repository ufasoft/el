/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
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

#pragma warning(disable: 4073) // initializers put in library initialization area
#pragma init_seg(lib)

#if UCFG_LIB_DECLS && UCFG_WIN32_FULL
#	pragma comment(lib, "shell32")
#endif

#if UCFG_LIB_DECLS && UCFG_WCE
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
	for (int size = _MAX_PATH;; size *= 2) {
		TCHAR* buf = (TCHAR*)alloca(sizeof(TCHAR) * size);
		DWORD dw = ::QueryDosDevice(dev, buf, size);
		if (dw && dw < size)
			return AsciizArrayToStringArray(buf);
		Win32Check(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
	}
}
#endif


Architecture RuntimeInformation::OSArchitecture() {
#if defined _M_X64
	return Architecture::X64;
#elif defined _M_IX86
	return Architecture::X86;
#elif defined _M_ARM
	return Architecture::Arm;
#elif defined _M_ARM64
	return Architecture::Arm64;
#elif defined _M_MIPS
	return Architecture::Mips;
#elif defined _M_MIPS64
	return Architecture::Mips64;
#elif defined _M_IA64
	return Architecture::IA64;
#elif defined _M_SHX
	return Architecture::SHX;
#else
	return Architecture::Unknown;
#endif
}

String Environment::GetMachineType() {
#if UCFG_USE_POSIX
	utsname u;
	CCheck(::uname(&u));
	return u.machine;
#elif defined(WIN32)
	switch (GetSystemInfo().wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL: 	return "x86";
	case PROCESSOR_ARCHITECTURE_MIPS:	return "MIPS";
	case PROCESSOR_ARCHITECTURE_SHX:	return "SHX";
	case PROCESSOR_ARCHITECTURE_ARM:	return "ARM";
	case PROCESSOR_ARCHITECTURE_IA64:	return "IA-64";
#	if UCFG_WIN32_FULL
	case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:	return "IA-32 on Win64";
	case PROCESSOR_ARCHITECTURE_AMD64:	return "x64";
	case PROCESSOR_ARCHITECTURE_ARM64:	return "ARM-64";
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
	SYSTEM_INFO si = GetSystemInfo();
	switch (si.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_INTEL:
		switch (si.dwProcessorType) {
		case 586:
			switch (si.wProcessorRevision) {
			case 5895:
				s = "Core 2 Quad";
				break;
			case 24067:
				s = "Core i7-6700";
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
		s = EXT_STR("x64 " << si.wProcessorLevel << "." << si.wProcessorRevision);
		break;
#endif
	case PROCESSOR_ARCHITECTURE_ARM:
		s = "ARM";
		break;
	case PROCESSOR_ARCHITECTURE_ARM64:
		s = "ARM64";
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

bool Environment::Is64BitOperatingSystem() {
#if UCFG_64
	return true;
#elif UCFG_USE_POSIX
	return GetMachineType().find("X86_64") != String::npos;
#elif UCFG_WIN32_FULL
	typedef void (__stdcall *PFN)(SYSTEM_INFO *psi);
	CDynamicLibrary dll("kernel32.dll");
	if (PFN pfn = (PFN)GetProcAddress(dll.m_hModule, String("GetNativeSystemInfo"))) {
		SYSTEM_INFO si;
		pfn(&si);
		return si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
	}
#endif
	return false;
}

#if UCFG_USE_REGEX
static StaticRegex s_reVersion("^(\\d+)\\.(\\d+)(\\.(\\d+)(\\.(\\d+))?)?$");

static int ToVersionInt(const cmatch::value_type& sm) {
	return sm.matched ? atoi(sm.str().c_str()) : -1;
}

Version::Version(RCString s) {
	cmatch m;
	if (!regex_search(s.c_str(), m, *s_reVersion))
		Throw(errc::invalid_argument);
	Major = ToVersionInt(m[1]);
	Minor = ToVersionInt(m[2]);
	Build = ToVersionInt(m[4]);
	Revision = ToVersionInt(m[6]);
}
#endif // UCFG_USE_REGEX

#if UCFG_WIN32

Version Version::FromFileInfo(int ms, int ls, int fieldCount) {
	int ar[4] = {uint16_t(ms >> 16), uint16_t(ms), uint16_t(ls >> 16), uint16_t(ls)};
	for (int i = 4; i-- > fieldCount;)
		ar[i] = -1;
	return Version(ar[0], ar[1], ar[2], ar[3]);
}

#endif

String Version::ToString(int fieldCount) const {
	if (fieldCount < 0 || fieldCount > 4)
		Throw(errc::invalid_argument);
	int ar[4] = { Major, Minor, Build, Revision };
	ostringstream os;
	for (int i = 0; i < fieldCount; ++i) {
		if (ar[i] == -1)
			Throw(errc::invalid_argument);
		os << (i ? "." : "") << ar[i];
	}
	return os.str();
}

String Version::ToString() const {
	return ToString(Revision != -1 ? 4 : Build != -1 ? 3 : 2);
}

const OSPlatform
	OSPlatform::Windows("Windows")
	, OSPlatform::Linux("Linux")
	, OSPlatform::OSX("OSX")
	, OSPlatform::Unix("Unix");

bool RuntimeInformation::IsOSPlatform(const OSPlatform& platform) {
#ifdef _WIN32
	return platform == OSPlatform::Windows;
#elif defined __APPLE__
	return platform == OSPlatform::OSX;
#elif defined __linux__
	return platform == OSPlatform::Linux;
#elif defined __unix__
	return platform == OSPlatform::Unix;
#else
	return false;
#endif
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
	String r = get_PlatformName() + " " + Version.ToString() + "  " + get_VersionName();
	if (!ServicePack.empty())
		r += " " + ServicePack;
	r += (Environment::Is64BitOperatingSystem() ? "  64-bit" : "  32-bit");
	return r;
}

String OperatingSystem::get_VersionName() const {
#ifdef _WIN32
	String s;
	switch (int osver = GetOsVersion()) {
	case OSVER_CE_5:	s = "CE 5";		break;
	case OSVER_CE_6:	s = "CE 5";		break;
	case OSVER_CE_FUTURE:	s = "CE Future";	break;
#	if UCFG_WIN32_FULL
	case OSVER_95:		s = "95";			break;
	case OSVER_98:		s = "98";			break;
	case OSVER_ME:		s = "Millenium";	break;
	case OSVER_NT4:		s = "NT 4";			break;
	default:
#		if UCFG_USE_REGISTRY
		s = RegistryKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion", false).TryQueryValue("ProductName", "Unknown Windows");
#		endif
		break;
#	else
	default:			s = "Unknown Future Version";	break;
#	endif
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
	: m_p(0)
{
	if (!(m_p = (LPTSTR)::GetEnvironmentStrings()))
		Throw(ExtErr::UnknownWin32Error);
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

const OperatingSystem Environment::OSVersion;

class Environment Environment;

vector<String> ParseCommandLine(RCString s) {
	vector<String> r;
	bool bQuoting = false, bSingleQuoting = false, bBackSlashQuote = false, bHasArg = false;
	String arg;
	for (const char *p=s; *p; ++p) {
		if (exchange(bBackSlashQuote, false))
			arg += String(*p);
		else {
			switch (*p) {
			case '\"':
				if (!bSingleQuoting) {
					bQuoting = !bQuoting;
					bHasArg = true;
				} else
					arg += String(*p);
				break;
			case '\'':
				if (!bQuoting) {
					bSingleQuoting = !bSingleQuoting;
					bHasArg = true;
				} else
					arg += String(*p);
				break;
#if UCFG_USE_POSIX
			case '\\':
				if (bSingleQuoting)
					arg += String(*p);
				else
					bBackSlashQuote = true;
				break;
#endif
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				if (!bQuoting && !bSingleQuoting) {
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
	String s = name + "=" + val;
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
	for (LPTSTR p = sk.m_p; *p; p += _tcslen(p) + 1) {
		if (const TCHAR* q = _tcschr(p, '='))
			m[String(p, q - p)] = q + 1;
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
		return ((a[0] >> 26) & 0x3F) + 1;				//!!! this code results to 2*cores on i7 CPUs
	}
#endif
	return get_ProcessorCount();
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
	for (size_t i = 0; i < blob.size(); ++i) {
		char ch = blob.constData()[i];
		if (ch == ' ')
			os << '+';
		else if (isalnum(ch) || strchr("!'()*-._", ch))
			os << ch;
		else
			os << '%' << setw(2) << setfill('0') << hex << (int)(uint8_t)ch;
	}
	return os.str();
}

String HttpUtility::UrlDecode(RCString s, Encoding& enc) {
	Blob blob = enc.GetBytes(s);
	ostringstream os;
	for (size_t i = 0; i < blob.size(); ++i) {
		char ch = blob.constData()[i];
		switch (ch) {
		case '+':
			os << ' ';
			break;
		case '%': {
			if (i + 2 > blob.size())
					Throw(E_FAIL);
			String c1((char)blob.constData()[++i]),
					c2((char)blob.constData()[++i]);
			os << (char)Convert::ToUInt32(c1 + c2, 16);
		} break;
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
		r.GetRef(name.ToUpper()).push_back(value);
	}
	return r;
}
#endif

// RFC 4648

class CBase64Table : public vector<int> {
public:
	static const char s_toBase64[];

	CBase64Table()
		: vector<int>((size_t)256, EOF)
	{
		for (uint8_t i = (uint8_t)strlen(s_toBase64); i--;) // to eliminate trailing zero
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
		wr << uint8_t(ch0 << 2 | ch1 >> 4);
		int ch2 = GetBase64Val(is);
		if (ch2 == EOF)
			break;
		wr << uint8_t(ch1 << 4 | ch2 >> 2);
		int ch3 = GetBase64Val(is);
		if (ch3 == EOF)
			break;
		wr << uint8_t(ch2 << 6 | ch3);
	}
	return ms.AsSpan();
}

String Convert::ToBase64String(RCSpan mb) {
	//!!!R	CBase64Table::s_toBase64;
	vector<String::value_type> v;
	const uint8_t* p = mb.data();
	for (size_t i = mb.size() / 3; i--; p += 3) {
		uint32_t dw = (p[0]<<16) | (p[1]<<8) | p[2];
		v.push_back(CBase64Table::s_toBase64[(dw>>18) & 0x3F]);
		v.push_back(CBase64Table::s_toBase64[(dw>>12) & 0x3F]);
		v.push_back(CBase64Table::s_toBase64[(dw>>6) & 0x3F]);
		v.push_back(CBase64Table::s_toBase64[dw & 0x3F]);
	}
	if (size_t rem = mb.size() % 3) {
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
	uint8_t m_prevValue;

	BitWriteStream(Stream& bas)
		: Base(bas)
		, Offset(0)
		, m_prevValue(0) {
	}

	void Write(int count, uint32_t value) {
		ASSERT(count <= 8);
		m_prevValue |= value << (8 - count) >> Offset;
		if (Offset + count >= 8) {
			Base.WriteBuffer(&m_prevValue, 1);
			m_prevValue = uint8_t(value << (16 - Offset - count));
		}
		Offset = (Offset + count) % 8;
	}

	void Flush() {
		if (Offset)
			Base.WriteBuffer(&m_prevValue, 1);
		Offset = 0;
		m_prevValue = 0;
	}
};

static Blob FromBaseX(int charsInGroup, int bits, RCString s, const uint8_t valTable[]) {
	MemoryStream ms;
	BitWriteStream bs(ms);
	for (size_t i = 0; i < s.size(); i += charsInGroup) {
		for (size_t j = 0; j < charsInGroup && i + j < s.size() ; ++j) {
			wchar_t ch = s[i + j];
			if ('=' == ch) {
				bs.Offset = 0;
				bs.Flush();
				break;
			}
			if (uint16_t(ch) >= 256)
				Throw(errc::invalid_argument);
			bs.Write(bits, valTable[ch]);
		}
	}
	if (charsInGroup > 1 && bs.Offset || charsInGroup == 1 && bs.Offset >= bits || bs.m_prevValue)
		Throw(ExtErr::Padding);
	return Blob(ms.AsSpan());
}

static String ToBaseX(int charsInGroup, int bytesInGroup, RCSpan mb, const char *table) {
	ostringstream os;
	const int bitsInGroup = bytesInGroup * 8 / charsInGroup,
		mask = (1<<bitsInGroup) - 1;
	for (size_t i = 0; i < mb.size();) {
		int nbits = 0;
		uint64_t val = 0;
		for (int j = bytesInGroup; j-- && i < mb.size(); nbits += 8)
			val |= uint64_t(mb[i++]) << (j * 8);
		for (int j = charsInGroup; j--; nbits -= bitsInGroup)
			os.put(nbits > 0 ? table[(val >> (j * bitsInGroup)) & mask] : '=');
	}
	return os.str();
}

class CBase32Table : public vector<uint8_t> {
public:
	static const char s_toBase32[];

	CBase32Table()
		: vector<uint8_t>((size_t)256, 0)
	{
		for (uint8_t i = (uint8_t)strlen(s_toBase32); i--;) // to eliminate trailing zero
			_self[s_toBase32[i]] = _self[tolower(s_toBase32[i])] = i;
	}
};

static InterlockedSingleton<CBase32Table> s_pBase32Table;

const char CBase32Table::s_toBase32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

Blob Convert::FromBase32String(RCString s, const uint8_t* table) {
	return FromBaseX(1, 5, s, table ? table : &(*s_pBase32Table)[0]);
}

String Convert::ToBase32String(RCSpan mb, const char *table) {
	return ToBaseX(8, 5, mb, table ? table : CBase32Table::s_toBase32);
}




int AFXAPI Rand() {
#if UCFG_USE_POSIX
	return (unsigned int)time(0) % ((unsigned int)RAND_MAX+1);
#else
	return int(System.PerformanceCounter % (RAND_MAX + 1));
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
}

void Random::NextBytes(const span<uint8_t>& mb) {
	uniform_int_distribution<int> dist(0, 255);
	for (size_t i = 0; i < mb.size(); i++)
		mb.data()[i] = (uint8_t)dist(*Rngeng(_self));
}

int Random::Next() {
	return uniform_int_distribution<int>(numeric_limits<int>::min(), numeric_limits<int>::max()) (*Rngeng(_self));
}

int Random::Next(int maxValue) {
	return uniform_int_distribution<int>(0, maxValue - 1)(*Rngeng(_self));
}

double Random::NextDouble() {
	return uniform_real_distribution<double>(0, 1) (*Rngeng(_self));
}

static class SystemURandomStream : public Stream {
public:
#if UCFG_USE_POSIX
	int m_fdsDevURandom;

	SystemURandomStream()
		: m_fdsDevURandom(open("/dev/urandom", O_RDONLY))
	{}
#endif

	size_t Read(void *buf, size_t count) const override {
#if UCFG_USE_POSIX
		ssize_t rc = ::read(m_fdsDevURandom, buf, count);
		CCheck(rc < 0 ? -1 : 0);
		return (size_t)rc;
#else
		unsigned u;
		CCheck(rand_s(&u));
		size_t r = (min)(sizeof u, count);
		memcpy(buf, &u, r);
		return r;
#endif
	}
} s_systemURandomStream;

static BinaryReader s_systemURandomReader(s_systemURandomStream);

BinaryReader& AFXAPI GetSystemURandomReader() {
	return s_systemURandomReader;
}

void CAnnoyer::OnAnnoy() {
	if (m_iAnnoy)
		m_iAnnoy->OnAnnoy();
}

void CAnnoyer::Request() {
	DateTime now = Clock::now();
	if (now-m_prev > m_period) {
		OnAnnoy();
		m_prev = now;
		m_period += m_period;
	}
}

HashAlgorithm::HashAlgorithm()
	: BlockSize(0)
	, HashSize(0)
	, IsHaifa(false)
	, IsBigEndian(true)
	, IsLenBigEndian(true)
	, IsBlockCounted(false)
	, Is64Bit(false)
{}

void HashAlgorithm::PrepareEndianness(void *dst, int count) noexcept {
	if (IsBigEndian == UCFG_LITLE_ENDIAN) {
		if (Is64Bit) {
			for (uint64_t *p = (uint64_t*)dst, *e = p + count; p != e; ++p)
				*p = swap64(*p);
		} else {
			for (uint32_t *p = (uint32_t*)dst, *e = p + count; p != e; ++p)
				*p = swap32(*p);
		}
	}
}

void HashAlgorithm::PrepareEndiannessAndHashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept {
	PrepareEndianness(src, WordCount);
	HashBlock(dst, src, counter);
}

template <typename W>
hashval ComputeHashImp(HashAlgorithm& algo, void *hash, Stream& stm, uint64_t processedLen) {
	const size_t dataSize = HashAlgorithm::WordCount * sizeof(W);
	DECLSPEC_ALIGN(32) W buf[64];									// input data & scratchbuffer
	uint64_t counter;
	bool bLast = false;
	while (true) {
		memset(buf, 0, dataSize);
		if (bLast) {
			counter = 0;
			break;
		}
		size_t cb = stm.Read(buf, dataSize);
		if (cb < dataSize) {
			((uint8_t*)buf)[cb] = 0x80;
			bLast = true;
		}
		processedLen += cb;
		counter = processedLen << 3;
		if (bLast && (processedLen & (dataSize - 1)) < dataSize - int(bool(algo.IsHaifa)) - 8) {
			if (!(processedLen & (dataSize - 1)))
				counter = 0;
			break;
		}
		algo.PrepareEndiannessAndHashBlock(hash, (uint8_t*)buf, counter);
	}
	if (algo.IsHaifa)
		((uint8_t*)buf)[dataSize - sizeof(W) * 2 - 1] = 1;
	processedLen = algo.IsBlockCounted ? (processedLen + 8 + dataSize) / dataSize
		: processedLen << 3;
	*(uint64_t*)((uint8_t*)buf + dataSize - 8) = algo.IsLenBigEndian ? htobe(processedLen) : htole(processedLen);
	algo.PrepareEndiannessAndHashBlock(hash, (uint8_t*)buf, counter);
	algo.OutTransform(hash);
	algo.PrepareEndianness(hash, 8);
	return hashval((const uint8_t*)hash, algo.HashSize);
}

hashval HashAlgorithm::ComputeHash(Stream& stm) {
#if UCFG_CPU_X86_X64
	__m128i hash[8];
#else
	W hash[16];
#endif
	InitHash(hash);
	return Is64Bit
		? ComputeHashImp<uint64_t>(_self, hash, stm, 0)
		: ComputeHashImp<uint32_t>(_self, hash, stm, 0);
}

hashval HashAlgorithm::ComputeHash(RCSpan mb) {
	CMemReadStream stm(mb);
	return ComputeHash(stm);
}

hashval HashAlgorithm::Finalize(void *hash, Stream& stm, uint64_t processedLen)
{
	return Is64Bit
		? ComputeHashImp<uint64_t>(_self, hash, stm, processedLen)
		: ComputeHashImp<uint32_t>(_self, hash, stm, processedLen);
}


// RFC 2104
hashval HMAC(HashAlgorithm& halgo, RCSpan key, RCSpan text) {
	Span k = key;
	hashval hk;
	if (key.size() > halgo.BlockSize) {
		hk = halgo.ComputeHash(key);
		k = Span(hk);
	}
	size_t size = halgo.BlockSize + max(halgo.BlockSize, text.size());
	uint8_t* buf = (uint8_t*)alloca(size);
	for (size_t i = 0; i < halgo.BlockSize; ++i)
		buf[i] = i < k.size() ? k[i] ^ 0x36 : 0x36;
	memcpy(buf + halgo.BlockSize, text.data(), text.size());
	hashval hv = halgo.ComputeHash(Span(buf, halgo.BlockSize + text.size()));
	for (size_t i = 0; i < halgo.BlockSize; ++i)
		buf[i] = i < k.size() ? k[i] ^ 0x5C : 0x5C;
	memcpy(buf + halgo.BlockSize, hv.constData(), hv.size());
	return halgo.ComputeHash(Span(buf, halgo.BlockSize + hv.size()));
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
	return hashval((const uint8_t*)&val, sizeof val);
}



bool PersistentCache::Lookup(RCString key, Blob& blob) {
	path fn = AfxGetCApp()->get_AppDataDir() / ".cache" / key.c_str();
	create_directories(fn.parent_path());
	if (!exists(fn))
		return false;
	FileStream fs(fn.c_str(), FileMode::Open, FileAccess::Read);
	blob.resize((size_t)fs.Length);
	fs.ReadExactly(blob.data(), blob.size());
	return true;
}

void PersistentCache::Set(RCString key, RCSpan mb) {
	path fn = AfxGetCApp()->get_AppDataDir() / ".cache" / key.c_str();
	create_directories(fn.parent_path());
	FileStream fs(fn, FileMode::Create, FileAccess::Write);
	fs.WriteBuffer(mb.data(), mb.size());

	/*!!!
#if UCFG_WIN32
	RegistryKey rk(AfxGetCApp()->KeyCU, "cache");
	rk.SetValue(key, mb);
#endif*/
}

#if UCFG_WIN32

ResourceObj::ResourceObj(HMODULE hModule, HRSRC hRsrc)
	: m_hModule(hModule)
	, m_hRsrc(hRsrc)
	, m_p(0)
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

const uint8_t *Resource::data() const {
#if UCFG_WIN32
	if (!m_pimpl->m_p)
		m_pimpl->m_p = ::LockResource(m_pimpl->m_hglbResource);
	return (const uint8_t*)m_pimpl->m_p;
#else
	return m_blob.constData();
#endif
}

size_t Resource::size() const {
#if UCFG_WIN32
	return Win32Check(::SizeofResource(m_pimpl->m_hModule, m_pimpl->m_hRsrc));
#else
	return m_blob.size();
#endif
}

void * AFXAPI AfxGetResource(const CResID& resID, const CResID& type) {
	return (void*)Resource(resID, type).data();//!!!
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



String ToDosPath(RCString lpath) {
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



path Path::GetPhysicalPath(const path& p) {
	String ps = p.native();
#if UCFG_WIN32_FULL
	while (true) {
		Path::CSplitPath sp = SplitPath(ps.c_str());
		vector<String> vec = System.QueryDosDevice(sp.m_drive);				// expand SUBST-ed drives
		if (vec.empty() || !vec[0].StartsWith("\\??\\"))
			break;
		ps = vec[0].substr(4) + sp.m_dir + sp.m_fname + sp.m_ext;
	}
#endif
	return ps.c_str();
}

DateTime DateTime::FromAsctime(RCString s) {
#if UCFG_WDM
	Throw(E_NOTIMPL);
#else
	char month[4];
	int year, day;
	if (sscanf(s, "%3s %d %d", month, &day, &year) == 3) {
		static const char s_months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
		if (const char* p = strstr(s_months, month))
			return DateTime(year, int(p - s_months) / 3 + 1, day);
	}
	Throw(ExtErr::InvalidInteger);
#endif
}

String CEscape::Escape(CEscape& esc, RCString s) {
	UTF8Encoding utf8;
	Blob blob = utf8.GetBytes(s);
	size_t size = blob.size();
	ostringstream os;
	const char* p = (const char*)blob.constData();
	for (size_t i = 0; i < size; ++i) {
		char ch = p[i];
		esc.EscapeChar(os, ch);
	}
	return os.str();
}

String CEscape::Unescape(CEscape& esc, RCString s) {
	vector<uint8_t> v;
	istringstream is(s.c_str());
	for (int ch; (ch = esc.UnescapeChar(is)) != EOF;)
		v.push_back((uint8_t)ch);
	UTF8Encoding utf8;
	if (v.empty())
		return String();
	return utf8.GetChars(Span(&v[0], v.size()));
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

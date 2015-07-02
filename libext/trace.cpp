/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#ifdef __linux__
#	include <sys/syscall.h>
#endif

#if UCFG_WIN32
#	include <windows.h>
#endif

#pragma warning(disable: 4073)
#pragma init_seg(lib)				// to initialize CTrace early

namespace Ext {
using namespace std;

class CDebugStreambuf : public streambuf {
	static const int MAX_BUFSIZE = 1024;

	vector<char> m_buf;

	void FlushBuffer() {
		m_buf.push_back(0);
		const char *s = &m_buf[0];
#ifdef WIN32
#	if UCFG_WCE
		OutputDebugString(String(s));
#	else		
		OutputDebugStringA(s);
#	endif
#elif defined WDM_DRIVER
		KdPrint(("%s", s));
#else
		fprintf(stderr, "%s", s);
#endif
		m_buf.clear();
	}

	int overflow(int c) {
		if (c != EOF) {
			if (m_buf.size() >= MAX_BUFSIZE)
				FlushBuffer();
			m_buf.push_back((char)c);
			if (c == '\n')
				FlushBuffer();
		}
		return c;
	}
};

class CDebugStream : public Stream {
public:
	void WriteBuffer(const void *buf, size_t count) override {
		String s((const char*)buf, count);
#if UCFG_WCE
	OutputDebugString(s);
#elif defined(WIN32)
	OutputDebugStringA(s);
#elif defined WDM_DRIVER
	KdPrint(("%s", s.c_str()));
#else
	fprintf(stderr, "%s", s.c_str());
#endif
	}
};

#if !UCFG_WDM      //!!!?
TraceStream::TraceStream(const path& p, bool bAppend)
	:	base(m_file)
{
	File::OpenInfo oi;
	oi.Path = p;
	oi.Mode = bAppend ? FileMode::Append : FileMode::Create;
	oi.Share = FileShare::ReadWrite;

	try {
		create_directories(p.parent_path());
		m_file.Open(oi);
	} catch (RCExc) {												// not a showstopper
	}
}
#endif // !UCFG_WDM

String TruncPrettyFunction(const char *fn) {
	const char *e = strchr(fn, '('), *b;
	for (b=e-1; b!=fn; --b)
		if (b[-1] == ' ') 
			break;
	return String(b, e-b);
}

bool CTrace::s_bShowCategoryNames;
//static CDebugStreambuf s_debugStreambuf;
//Stream *CTrace::s_pOstream = new ostream(&s_debugStreambuf);	// ostream allocated in Heap because it should be valid after PROCESS_DETACH
Stream *CTrace::s_pOstream = new CDebugStream;	// ostream allocated in Heap because it should be valid after PROCESS_DETACH

Stream *CTrace::s_pSecondStream;
bool CTrace::s_bPrintDate;

#if UCFG_WDM && defined(_DEBUG)
	int CTrace::s_nLevel = 1;
	CTrace::PFN_DbgPrint CTrace::s_pTrcPrint = &DbgPrint;
#else
	int CTrace::s_nLevel = 0;
	CTrace::PFN_DbgPrint CTrace::s_pTrcPrint = 0;
#endif

void CTrace::InitTraceLog(RCString regKey) {
#ifdef _WIN32
	RegistryKey key(0, regKey);
	s_nLevel = (DWORD)key.TryQueryValue("TraceLevel", s_nLevel);

	TRC(0, "Trace Level " << s_nLevel);
#endif

#if UCFG_WDM
	s_pTrcPrint = s_nLevel ? &DbgPrint : nullptr;
#endif
}

Stream* CTrace::GetOStream() {
	return s_pOstream;
}

void CTrace::SetOStream(Stream *os) {
	s_pOstream = os;
}

void CTrace::SetSecondOStream(Stream *os) {
	s_pSecondStream = os;
}


#if !UCFG_WDM
static struct CTraceInit {
	CTraceInit() {
		if (const char *slevel = getenv("UCFG_TRC")) {
			if (!CTrace::GetOStream())
				CTrace::SetOStream(new CIosStream(clog));
			CTrace::s_nLevel = atoi(slevel);
		}
	}
} s_traceInit;			//!!! Windows: too early here
#endif // !UCFG_WDM

static class CNullStream : public ostream {
	class CNullStreambuf : public streambuf {
		int overflow(int ch) override { return ch; }
	};
public:
	CNullStream()
			:	ostream(&m_sbuf)
		{
		}
private:
	CNullStreambuf m_sbuf;
} s_nullStream;

static class CWNullStream : public wostream {
	class CWNullStreambuf : public wstreambuf {
		int_type overflow(int_type ch) override { return ch; }
	};
public:
	CWNullStream()
			:	wostream(&m_sbuf)
		{
		}
private:
	CWNullStreambuf m_sbuf;
} s_wnullStream;

ostream& AFXAPI GetNullStream() {
	return s_nullStream;
}

wostream& AFXAPI GetWNullStream() {
	return s_wnullStream;
}

static int s_nThreadNumber;

union TinyThreadInfo {
	struct {
		byte Counter[3];		//!!!Endianess
		byte TraceLocks;
	};
	void *P;
};

typedef void (*PFNTrace)(const char *, ...);

#if UCFG_WDM

struct TraceThreadContext {
	LIST_ENTRY list_entry;
	PETHREAD pethread;
	int id;
	int indent;
};

BOOLEAN Is_In_List(LIST_ENTRY *list, LIST_ENTRY *entry, KSPIN_LOCK *spin)
{
	KIRQL oldIrql;
	LIST_ENTRY *le;
	KeAcquireSpinLock(spin, &oldIrql);
	for (le=list->Flink; le!=list; le=le->Flink)
		if (le == entry)
			break;
	KeReleaseSpinLock(spin, oldIrql);
	return le != list;
}

LIST_ENTRY g_list_trace;
KSPIN_LOCK st_spin_trace;

TraceThreadContext *GetThreadContext()
{
	KIRQL oldIrql;
	PETHREAD pethread = PsGetCurrentThread();
	LIST_ENTRY *le;
	TraceThreadContext *ctx;
	KeAcquireSpinLock(&st_spin_trace, &oldIrql);
	for (le=g_list_trace.Flink; le!=&g_list_trace; le=le->Flink)
		if ((ctx = (TraceThreadContext*)le)->pethread == pethread)
			break;
	if (le == &g_list_trace)
	{
		ctx = new TraceThreadContext;
		ctx->pethread = pethread;
		ctx->id = ++s_nThreadNumber;
		ctx->indent = 1;
		InsertHeadList(&g_list_trace, &ctx->list_entry);
	}
	KeReleaseSpinLock(&st_spin_trace, oldIrql);
	return ctx;
}



//!!! #pragma comment(lib, "wdmsec.lib")

UNICODE_STRING g_registryPath;


#if DBG
PFNTrace g_pfnTrace = (PFNTrace)DbgPrint;
#else
PFNTrace g_pfnTrace;
#endif

typedef ULONG (* PFNvDbgPrintExWithPrefix)(PCCH  Prefix, IN ULONG  ComponentId, IN ULONG  Level, IN PCCH  Format, IN va_list  arglist);
PFNvDbgPrintExWithPrefix g_pfnPFNvDbgPrintExWithPrefix;

ULONG __cdecl DrvTrace(PCH format, ...) {
	va_list arglist;
	char fmt[20];
	char prefix[50];
	TraceThreadContext *ctx  = GetThreadContext();
	int indent = std::min(ctx->indent, (int)(sizeof(prefix)-10));
	va_start(arglist, format);
	sprintf(fmt, "%d%% %ds", ctx->id, indent);
	sprintf(prefix, fmt, "");
	return g_pfnPFNvDbgPrintExWithPrefix(prefix, DPFLTR_IHVDRIVER_ID, 0xFFFFFFFF, format, arglist);
}

#else

CTls CFunTrace::s_level;
static CTls t_threadNumber;

CFunTrace::CFunTrace(const char *funName, int trclevel)
	:   m_trclevel(trclevel)
	,	m_funName(funName)
{
	intptr_t level = (intptr_t)(void*)s_level.Value;
	TRC(m_trclevel, String(' ', level*2)+">"+m_funName);
	s_level.Value = (void*)(level+1);
}

CFunTrace::~CFunTrace() {
	intptr_t level = (intptr_t)(void*)s_level.Value-1;
	s_level.Value = (void*)level;
	TRC(m_trclevel, String(' ', level*2)+"<"+m_funName);
}

class TraceBlocker {
public:
	bool Trace;

	TraceBlocker() {
		TinyThreadInfo tti;
		tti.P = t_threadNumber.Value;
		Trace = !tti.TraceLocks;
		++tti.TraceLocks;
		t_threadNumber.Value = tti.P;
	}

	~TraceBlocker() {
		TinyThreadInfo tti;
		tti.P = t_threadNumber.Value;
		--tti.TraceLocks;
		t_threadNumber.Value = tti.P;
	}
};


#endif // UCFG_WDM

inline intptr_t AFXAPI GetThreadNumber() {
#ifdef WDM_DRIVER
	return (intptr_t)PsGetCurrentThreadId();
#elif UCFG_WIN32
	return ::GetCurrentThreadId();
#elif defined(__linux__)
	return syscall(SYS_gettid);
#else
	intptr_t r = (intptr_t)(void*)t_threadNumber.Value;
	if (!(r & 0xFFFFFF))
		t_threadNumber.Value = (void*)(uintptr_t)(r |= ++s_nThreadNumber);
	return r & 0xFFFFFF;
#endif
}

CTraceWriter::CTraceWriter(int level, const char* funname) noexcept
	:	m_pos(level & CTrace::s_nLevel ? CTrace::s_pOstream : 0)
{
	if (m_pos) {
		m_bPrintDate = CTrace::s_bPrintDate;
		Init(funname);
	}
}

CTraceWriter::CTraceWriter(Ext::Stream *pos) noexcept
	:	m_pos(pos)
	,	m_bPrintDate(true)
{
	Init(0);
}

void CTraceWriter::Init(const char* funname) {
	if (funname)
		m_os << funname << " ";
}

#if UCFG_USE_POSIX
#	define EXT_TID_FORMATTER "%-4" EXT_LL_PREFIX "d"
#else
#	define EXT_TID_FORMATTER "%-4" EXT_LL_PREFIX "x"
#endif

CTraceWriter::~CTraceWriter() noexcept {
	if (m_pos) {
		m_os.put('\n');
		string str = m_os.str();
		LocalDateTime dt = Clock::now().ToLocalTime();
		tm t = dt;
		int mst = dt.Ticks / 1000 % 10000;
		long long tid = GetThreadNumber();
		char bufTime[20], buf[100];
		sprintf(bufTime, " %02d:%02d:%02d.%04d ", t.tm_hour, t.tm_min, t.tm_sec, mst);
		if (m_bPrintDate)
			sprintf(buf, EXT_TID_FORMATTER " %4d-%02d-%02d%s", tid, 1900+int(t.tm_year), t.tm_mon, t.tm_mday, bufTime);
		else
			sprintf(buf, EXT_TID_FORMATTER "%s", tid, bufTime);
		string date_s = buf + str;
		string time_str;
		if (ostream *pSecondStream = (ostream*)CTrace::s_pSecondStream) {
			sprintf(buf, EXT_TID_FORMATTER "%s", tid, bufTime);
			time_str = buf + str;
		}

#if !UCFG_WDM
		TraceBlocker traceBlocker;
		if (traceBlocker.Trace)
#endif
		{
			try {
				m_pos->WriteBuffer(date_s.data(), date_s.size());
			} catch (RCExc) {}
			if (Ext::Stream *pSecondStream = CTrace::s_pSecondStream) {
				try {
					pSecondStream->WriteBuffer(time_str.data(), time_str.size());
				} catch (RCExc) {}
			}
		}
	}
}

ostream& CTraceWriter::Stream() noexcept {
	return m_pos ? static_cast<ostream&>(m_os) : s_nullStream;
}

void CTraceWriter::VPrintf(const char* fmt, va_list args) {
	if (m_pos) {
		char buf[1000];
#if UCFG_USE_POSIX
		vsnprintf(buf, sizeof(buf), fmt, args );
#else
		_vsnprintf(buf, sizeof(buf), fmt, args );
#endif
		m_os << buf;
	}
}

void CTraceWriter::Printf(const char* fmt, ...) {
	if (m_pos) {
		va_list args;
		va_start(args, fmt);
		VPrintf(fmt, args);
	}
}

CTraceWriter& CTraceWriter::CreatePreObject(char *obj, int level, const char* funname) {
	CTraceWriter& w = *new(obj) CTraceWriter(level);
	w.Stream() << funname << ":\t";
	return w;
}

void CTraceWriter::StaticPrintf(int level, const char* funname, const char* fmt, ...) {
	CTraceWriter w(level);
	w.Stream() << funname << ":\t";
	va_list args;
	va_start(args, fmt);
	w.VPrintf(fmt, args);	
}


} // Ext::



/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#pragma warning(disable: 4073)
#pragma init_seg(lib)				// to initialize CTrace early

namespace Ext {
using namespace std;


String TruncPrettyFunction(const char *fn) {
	const char *e = strchr(fn, '('), *b;
	for (b=e-1; b!=fn; --b)
		if (b[-1] == ' ') 
			break;
	return String(b, e-b);
}

bool CTrace::s_bShowCategoryNames;
static CDebugStreambuf s_debugStreambuf;
Stream *CTrace::s_pOstream = new ostream(&s_debugStreambuf);	// ostream allocated in Heap because it should be valid after PROCESS_DETACH
void *CTrace::s_pSecondStream;
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

void CTrace::SetOStream(void *os) {
	s_pOstream = os;
}

#if !UCFG_WDM
static struct CTraceInit {
	CTraceInit() {
		if (const char *slevel = getenv("UCFG_TRC")) {
			CTrace::SetOStream(&cerr);
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



} // Ext::



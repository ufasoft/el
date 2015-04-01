#pragma once

#include <wincon.h>
#include <shellapi.h>

namespace Ext {


class AFX_CLASS Console : public SafeHandle {
public:
	Console(HANDLE h)
		:	SafeHandle((intptr_t)h)
	{}

	static void AFXAPI Beep(DWORD dwFreq = 800, DWORD dwDuration = 200);
};

class AFX_CLASS COutputConsole : public Console {
public:
	COutputConsole(HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE))
		:	Console(h)
	{}

	CONSOLE_SCREEN_BUFFER_INFO get_ScreenBufferInfo();
	DEFPROP_GET(CONSOLE_SCREEN_BUFFER_INFO, ScreenBufferInfo);

	void SetCursorPosition(COORD coord);
	//!!!  __declspec(property(put=SetCursorPosition)) COORD CursorPosition;
};

class AFX_CLASS CInputConsole : public Console {
public:
	CInputConsole(HANDLE h = GetStdHandle(STD_INPUT_HANDLE))
		:	Console(h)
	{}
};




class CDropFiles {
public:
	HDROP m_h;

	CDropFiles(HDROP h)
		:	m_h(h)
	{
	}

	~CDropFiles() {
		::DragFinish(m_h);
	}

	vector<path> GetPaths();
};

class AFX_CLASS CEnvironmentStrings {
public:
	LPTSTR m_p;

	CEnvironmentStrings()
		:	m_p(GetEnvironmentStrings())
	{
	}

	~CEnvironmentStrings() {
		Win32Check(FreeEnvironmentStrings(m_p));
	}
};

/*!!!
class AFX_CLASS CServiceHandler {
public:
	CServiceHandler(RCString serviceName, LPHANDLER_FUNCTION_EX lpHandlerProc, void *ctx);
	void SetServiceStatus(SERVICE_STATUS& serviceStatus);
private:
	SERVICE_STATUS_HANDLE m_handle;
};*/

ENUM_CLASS(CrashDumpType) {
	None,
	File,
	Email,
	HttpPost
} END_ENUM_CLASS(CrashDumpType);

class CUnhandledExceptionFilter {
public:
	EXT_DATA static CUnhandledExceptionFilter *I;

	CrashDumpType Typ;

	CUnhandledExceptionFilter();
	virtual ~CUnhandledExceptionFilter();
	void RestorePrev();

	virtual LONG Handle(EXCEPTION_POINTERS *ExceptionInfo) { return EXCEPTION_CONTINUE_SEARCH; }
protected:
	LPTOP_LEVEL_EXCEPTION_FILTER m_prev;
	CBool m_bSet;

private:
	static std::mutex s_mtx;	

	static DWORD WINAPI StackOverflowThreadFunction(LPVOID ctx);
	static LONG CALLBACK UnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);
};

struct NamedPipeCreateInfo {
	String Name;
	CInt<DWORD> OpenMode, PipeMode;
	uint32_t MaxInstances;
	observer_ptr<SECURITY_ATTRIBUTES> Sa;

	NamedPipeCreateInfo()
		:	MaxInstances(PIPE_UNLIMITED_INSTANCES)
		,	OpenMode(PIPE_ACCESS_DUPLEX)
	{}
};

class NamedPipe : public File {
public:
	void Create(const NamedPipeCreateInfo& ci);

	void SetHandleState(DWORD mode) {
		Win32Check(::SetNamedPipeHandleState((HANDLE)(intptr_t)Handle(*this), &mode, 0, 0));
	}

	bool Connect(LPOVERLAPPED ovl = NULL);
};

class StdHandle {
public:
	static HANDLE AFXAPI Get(DWORD nStdHandle);
	static HANDLE AFXAPI Set(DWORD nStdHandle, HANDLE h) { Win32Check(::SetStdHandle(nStdHandle, h)); }
};



} // Ext::

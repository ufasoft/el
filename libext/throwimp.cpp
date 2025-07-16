/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <windows.h>
#	include <wininet.h>
#endif

#if UCFG_WIN32 && !UCFG_MINISTL
#	include <shlwapi.h>

#	include <el/libext/win32/ext-win.h>
#endif

#pragma warning(disable: 4073)
#pragma init_seg(lib)				// to initialize early

namespace Ext {
using namespace std;

void ThrowNonEmptyPointer() {
	Throw(ExtErr::NonEmptyPointer);
}

HRESULT AFXAPI ToHResult(const system_error& ex) {
	const error_code& ec = ex.code();
	const error_category& cat = ec.category();
	int ecode = ec.value();
	if (cat == generic_category())
		return HRESULT_FROM_C(ecode);
	else if (cat == system_category()) {														// == win32_category() on Windows
#if UCFG_WIN32
		return (HRESULT)(((ecode) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000);
#else
		return (HRESULT)(((ecode) & 0x0000FFFF) | (FACILITY_OS << 16) | 0x80000000);
#endif
	} else if (cat == hresult_category())
		return ecode;
#ifdef _WIN32
	else if (cat == ntos_category())
		return HRESULT_FROM_NT(ecode);
#endif
	else {
		int fac = FACILITY_UNKNOWN;
		for (const ErrorCategoryBase* p = ErrorCategoryBase::Root; p; p = p->Next) {
			if (p == &cat) {
				fac = p->Facility;
				break;
			}
		}
		return (HRESULT)(((ecode) & 0x0000FFFF) | (fac << 16) | 0x80000000);
	}
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
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_EXT, int(ExtErr::InvalidCast));
	} catch (const out_of_range&) {
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_EXT, int(ExtErr::IndexOutOfRange));
	} catch (const exception&) {
		return E_FAIL;
	}
}

#if !UCFG_WDM
thread_local const wchar_t* Exception::t_LastStringArg;
#endif


static class HResultCategory : public error_category {		// outside function to eliminate thread-safe static machinery
	typedef error_category base;

	const char* name() const noexcept override { return "HResult"; }

	string message(int eval) const override {
#if !UCFG_ERROR_MESSAGE || UCFG_WDM
		return "Error";		//!!!
#else
		return explicit_cast<string>(HResultToMessage(eval, false));
#endif
	}

	bool equivalent(int errval, const error_condition& c) const noexcept override {
		switch (HRESULT_FACILITY(errval)) {
		case FACILITY_C:
			return generic_category().equivalent(errval & 0xFFFF, c);
#if UCFG_WIN32
		case FACILITY_WIN32:
			return win32_category().equivalent(errval & 0xFFFF, c);
#endif
		default:
			return base::equivalent(errval, c);
		}
	}

	bool equivalent(const error_code& ec, int errval) const noexcept override { return base::equivalent(ec, errval); }
} s_hresultCategory;

const error_category& AFXAPI hresult_category() {
	return s_hresultCategory;
}

void Exception::TryStackTrace() {
#if UCFG_STACK_TRACE
	if (CStackTrace::Use)
		StackTrace = CStackTrace::FromCurrentThread();
#endif
}

Exception::Exception(HRESULT hr, RCString message)
	: base(hr, hresult_category(), message.empty() ? string() : string(explicit_cast<string>(message)))
	, m_message(message) {
	TryStackTrace();
}

Exception::Exception(const error_code& ec, RCString message)
	: base(ec, message.empty() ? string() : string(explicit_cast<string>(message)))
	, m_message(message) {
	TryStackTrace();
}

Exception::Exception(ExtErr errval, RCString message)
	: base(make_error_code(errval), message.empty() ? string() : string(explicit_cast<string>(message)))
	, m_message(message) {
	TryStackTrace();
}

String Exception::get_Message() const {
#if UCFG_TRC
	ostringstream os;
	os << "Error ";
#if !UCFG_WDM
	os << setw(8) << hex << ToHResult(_self) << ": ";
#endif
	string msg = code().message();
#if defined(_MSC_VER) && UCFG_STDSTL && UCFG_STD_SYSTEM_ERROR									// VC implementation returns ANSI string
	if (code().category() == system_category()) {
		CodePageEncoding encAnsi(CP_ACP);
		msg = String(encAnsi.GetChars(ConstBuf(msg.data(), msg.length())));
}
#endif
	os << dec << code() << " " << msg;
	return os.str();
#else
	return "exception";
#endif
}


String Exception::ToString() const {
	String r = get_Message().TrimEnd();
#if UCFG_TRC
	for (CDataMap::const_iterator i = Data.begin(), e = Data.end(); i != e; ++i)
		r += "\n  " + i->first + ": " + i->second;
#if UCFG_STACK_TRACE
	if (CStackTrace::Use)
		r += "\n  " + StackTrace.ToString();
#endif
#endif
	return r;
}

const char* Exception::what() const noexcept {
	try {
		if (m_message.empty())
			m_message = get_Message();
		return m_message.c_str();
	} catch (RCExc) {
		return "Error: double exception in the Exception::what()";
	}
}



intptr_t __stdcall AfxApiNotFound() {
	Throw(HRESULT_OF_WIN32(ERROR_PROC_NOT_FOUND));
}

DECLSPEC_NORETURN void AFXAPI ThrowImp(const error_code& ec) {
	TRC(3, ec);
	HRESULT hr = ec.value();
#if UCFG_WDM && !_HAS_EXCEPTIONS
	KeBugCheck(hr);
#else
	if (ec == ExtErr::EndOfStream)
		throw EndOfStreamException();
	if (ec == ExtErr::FileFormat)
		throw FileFormatException();
	if (ec == ExtErr::ThreadInterrupted)
		throw thread_interrupted();
	if (ec == ExtErr::InvalidCast)
		throw bad_cast();
	if (ec == ExtErr::IndexOutOfRange)
		throw out_of_range(ec.message());

	if (ec.category() != hresult_category())
		throw Exception(ec);

	switch (hr) {
	case E_ACCESSDENIED:			throw AccessDeniedException();
	case E_OUTOFMEMORY:				throw bad_alloc();
	case E_INVALIDARG:				throw ArgumentExc();
	case E_NOTIMPL:					throw NotImplementedExc();
	case E_FAIL:					throw UnspecifiedException();
	case HRESULT_OF_WIN32(ERROR_STACK_OVERFLOW):	throw StackOverflowExc();

		/*!!!R
			case HRESULT_OF_WIN32(ERROR_FILE_NOT_FOUND):
				{
					FileNotFoundException e;
		#if !UCFG_WDM
					if (Exception::t_LastStringArg)
						e.FileName = *Exception::t_LastStringArg;
		#endif
					throw e;
				}
			case HRESULT_OF_WIN32(ERROR_FILE_EXISTS):
				{
					FileAlreadyExistsExc e;
		#if !UCFG_WDM
					if (Exception::t_LastStringArg)
						e.FileName = *Exception::t_LastStringArg;
		#endif
					throw e;
				}
			case HRESULT_OF_WIN32(ERROR_PATH_NOT_FOUND):
				{
					DirectoryNotFoundExc e;
					throw e;
				}
				*/
	case HRESULT_OF_WIN32(ERROR_PROC_NOT_FOUND):
	{
		ProcNotFoundExc e;
#if !UCFG_WDM
		if (Exception::t_LastStringArg)
			e.ProcName = Exception::t_LastStringArg;
#endif
		throw e;
	}
	//!!!R	case HRESULT_OF_WIN32(ERROR_INVALID_HANDLE):
	//!!!R		::MessageBox(0, _T(" aa"), _T(" aa"), MB_OK); //!!!D
	default:
		throw Exception(ec);
	}
#endif
}

DECLSPEC_NORETURN void AFXAPI ThrowImp(HRESULT hr) {
	ThrowImp(error_code(hr, hresult_category()));
}

DECLSPEC_NORETURN void AFXAPI ThrowWin32(int code) {
	ThrowImp(HRESULT_FROM_WIN32(code));
}

#if UCFG_CRT=='U' && !UCFG_WDM

typedef void (AFXAPI* PFNThrowImp)(HRESULT hr);
void SetThrowImp(PFNThrowImp pfn);
static int s_initThrowImp = (SetThrowImp(&ThrowImp), 1);

#endif // UCFG_CRT=='U' && !UCFG_WDM

static void TraceError(const error_code& ec, const char* funname, int nLine) {		// In separate function to decrease stack frame size of ThrowImp(). Important during unwinding
#if UCFG_EH_SUPPORT_IGNORE
	if (!CLocalIgnoreBase::ErrorCodeIsIgnored(ec)) {
		TRC(1, funname << "(Ln" << nLine << "): " << (ec.category() == hresult_category() ? EXT_STR("HRESULT:" << hex << ec.value()) : EXT_STR(ec)) << " " << ec.message());
		TRC(1, CStackTrace::FromCurrentThread());
	}
#endif
}

DECLSPEC_NORETURN void AFXAPI ThrowImp(const error_code& ec, const char* funname, int nLine) {
	TraceError(ec, funname, nLine);
	ThrowImp(ec);
}

DECLSPEC_NORETURN void AFXAPI ThrowImp(HRESULT hr, const char* funname, int nLine) {
	if (HRESULT_SEVERITY(hr) == SEVERITY_ERROR) {
		switch (auto fac = HRESULT_FACILITY(hr)) {
		case FACILITY_WIN32:
			ThrowImp(error_code(HRESULT_CODE(hr), win32_category()), funname, nLine);
		}
	}	
	ThrowImp(error_code(hr, hresult_category()), funname, nLine);
}

typedef map<int, CExceptionFabric*> CExceptionFabrics;
static InterlockedSingleton<CExceptionFabrics> g_exceptionFabrics;

CExceptionFabric::CExceptionFabric(int facility) {
	(*g_exceptionFabrics)[facility] = this;
}

DECLSPEC_NORETURN void AFXAPI ThrowS(HRESULT hr, RCString msg) {
	TRC(1, "Error " << hex << hr << " " << (!!msg ? msg : String()));

	int facility = HRESULT_FACILITY(hr);
	CExceptionFabrics::iterator i = (*g_exceptionFabrics).find(facility);
	if (i != (*g_exceptionFabrics).end())
		i->second->ThrowException(hr, msg);
#if UCFG_WDM && !_HAS_EXCEPTIONS
	KeBugCheck(hr);
#else
	throw Exception(hr, msg);
#endif
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
		auto err = errno;
		Throw(error_code(err, generic_category()));
	}
}

#if !UCFG_WDM && !UCFG_MINISTL

String g_ExceptionMessage;

#if UCFG_WIN32
void AFXAPI ProcessExceptionInFilter(EXCEPTION_POINTERS* ep) {
#	if UCFG_TRC
	ostringstream os;
	os << "Code:\t" << hex << ep->ExceptionRecord->ExceptionCode << "\n"
		<< "Address:\t" << ep->ExceptionRecord->ExceptionAddress << "\n";
	g_ExceptionMessage = os.str();
	TRC(0, g_ExceptionMessage);
#	endif
}

void AFXAPI ProcessExceptionInExcept() {
#if UCFG_GUI
	MessageBox::Show(g_ExceptionMessage);
#endif
	::ExitProcess(ERR_UNHANDLED_EXCEPTION);
}
#endif

void AFXAPI ProcessExceptionInCatch() {
	try {
		throw;
#	if UCFG_TRC
	} catch (const Exception& ex) {
		TRC(0, ex);
		wcerr << ex.what() << endl;
#	else
	} catch (const Exception&) {
#	endif
#if UCFG_GUI
		if (!IsConsole())
			MessageBox::Show(ex.what());
#endif
#if !UCFG_CATCH_UNHANDLED_EXC
		throw;
#endif
#	if UCFG_TRC
	} catch (std::exception& e) {
		TRC(0, e.what());
		cerr << e.what() << endl;
#	else
	} catch (std::exception&) {
#	endif
#if UCFG_GUI
		if (!IsConsole())
			MessageBox::Show(e.what());
#endif
#if !UCFG_CATCH_UNHANDLED_EXC
		throw;
#endif
	} catch (...) {
#	if UCFG_TRC
		TRC(0, "Unknown C++ exception");
		cerr << "Unknown C++ Exception" << endl;
#	endif
#if UCFG_GUI
		if (!IsConsole())
			MessageBox::Show("Unknown C++ Exception");
#endif
#if !UCFG_CATCH_UNHANDLED_EXC
		throw;
#endif
	}
#if UCFG_USE_POSIX
	_exit(ERR_UNHANDLED_EXCEPTION);
#else
	//!!! Error in DLLS ::ExitProcess(ERR_UNHANDLED_EXCEPTION);
	::TerminateProcess(::GetCurrentProcess(), ERR_UNHANDLED_EXCEPTION);
#endif
}

#endif // !UCFG_WDM && !UCFG_MINISTL

#if UCFG_WIN32

int AFXAPI Win32Check(LRESULT i) {
	if (i)
		return (int)i;
	error_code ec;
	if (DWORD dw = ::GetLastError()) {
		if (dw & 0xFFFF0000)
			Throw(dw);
		ec = error_code(dw, system_category());
	} else {
		HRESULT hr = GetLastHResult();
		ec = hr ? error_code(hr, hresult_category()) : make_error_code(ExtErr::UnknownWin32Error);
	}
	Throw(ec);
}

bool AFXAPI Win32Check(BOOL b, DWORD allowableError) {
	Win32Check(b || ::GetLastError() == allowableError);
	return b;
}

#	if UCFG_USE_DECLSPEC_THREAD
__declspec(thread) HRESULT t_lastHResult;
HRESULT AFXAPI GetLastHResult() { return t_lastHResult; }
void AFXAPI SetLastHResult(HRESULT hr) { t_lastHResult = hr; }
#	else
CTls t_lastHResult;
HRESULT AFXAPI GetLastHResult() { return (HRESULT)(uintptr_t)t_lastHResult.Value; }
void AFXAPI SetLastHResult(HRESULT hr) { t_lastHResult.Value = (void*)(uintptr_t)hr; }
#	endif // UCFG_USE_DECLSPEC_THREAD

struct Win32CodeErrc {
	uint16_t Code;
	errc Errc;
};

static const Win32CodeErrc s_win32code2errc[] ={
	ERROR_FILE_NOT_FOUND,		errc::no_such_file_or_directory,
	ERROR_PATH_NOT_FOUND,		errc::no_such_file_or_directory,
	ERROR_ACCESS_DENIED,		errc::permission_denied,
	ERROR_INVALID_HANDLE,		errc::bad_file_descriptor,
	ERROR_OUTOFMEMORY,			errc::not_enough_memory,
	ERROR_NOT_ENOUGH_MEMORY,	errc::not_enough_memory,
	ERROR_NOT_SUPPORTED,		errc::not_supported,
	ERROR_INVALID_PARAMETER,	errc::invalid_argument,
	ERROR_BROKEN_PIPE,			errc::broken_pipe,
	ERROR_ALREADY_EXISTS,		errc::file_exists,
	ERROR_FILENAME_EXCED_RANGE, errc::filename_too_long,
	ERROR_FILE_TOO_LARGE,		errc::file_too_large,
	ERROR_CANCELLED,			errc::operation_canceled,
	ERROR_WAIT_NO_CHILDREN,		errc::no_child_process,
	ERROR_ARITHMETIC_OVERFLOW,	errc::result_out_of_range,
	ERROR_BUSY,					errc::device_or_resource_busy,
	ERROR_DEVICE_IN_USE,		errc::device_or_resource_busy,
	ERROR_BAD_FORMAT,			errc::executable_format_error,
	ERROR_DIR_NOT_EMPTY,		errc::directory_not_empty,
	ERROR_DISK_FULL,			errc::no_space_on_device,
	ERROR_INVALID_ADDRESS,		errc::bad_address,
	ERROR_TIMEOUT,				errc::timed_out,
	ERROR_IO_PENDING,			errc::resource_unavailable_try_again,
	ERROR_NOT_SAME_DEVICE,		errc::cross_device_link,
	ERROR_WRITE_PROTECT,		errc::read_only_file_system,
	ERROR_POSSIBLE_DEADLOCK,	errc::resource_deadlock_would_occur,
	ERROR_PRIVILEGE_NOT_HELD,	errc::operation_not_permitted,
	ERROR_INTERNET_CANNOT_CONNECT, errc::connection_refused,
	WSAENOBUFS, 				errc::no_buffer_space,
	WSAEINTR,					errc::interrupted,
	WSAEBADF,					errc::bad_file_descriptor,
	WSAEACCES,					errc::permission_denied,
	WSAEFAULT,					errc::bad_address,
	WSAEINVAL,					errc::invalid_argument,
	WSAEMFILE,					errc::too_many_files_open,
	WSAEWOULDBLOCK,				errc::operation_would_block,
	WSAEINPROGRESS,				errc::operation_in_progress,
	WSAEALREADY,				errc::connection_already_in_progress,
	WSAENOTSOCK,				errc::not_a_socket,
	WSAEDESTADDRREQ,			errc::destination_address_required,
	WSAEMSGSIZE,				errc::message_size,
	WSAEPROTOTYPE,				errc::wrong_protocol_type,
	WSAENOPROTOOPT,				errc::no_protocol_option,
	WSAEPROTONOSUPPORT,			errc::protocol_not_supported,
	WSAEOPNOTSUPP,				errc::protocol_not_supported,
	WSAEAFNOSUPPORT,			errc::address_family_not_supported,
	WSAEADDRINUSE,				errc::address_in_use,
	WSAEADDRNOTAVAIL,			errc::address_not_available,
	WSAENETDOWN,				errc::network_down,
	WSAENETUNREACH,				errc::network_unreachable,
	WSAENETRESET,				errc::network_reset,
	WSAECONNABORTED,			errc::connection_aborted,
	WSAECONNRESET,				errc::connection_reset,
	WSAEISCONN,					errc::already_connected,
	WSAENOTCONN,				errc::not_connected,
	WSAETIMEDOUT,				errc::timed_out,
	WSAECONNREFUSED,			errc::connection_refused,
	WSAELOOP,					errc::too_many_symbolic_link_levels,
	WSAENAMETOOLONG,			errc::filename_too_long,
	WSAEHOSTUNREACH,			errc::host_unreachable,
	WSAENOTEMPTY,				errc::directory_not_empty,
	WSAECANCELLED,				errc::operation_canceled,
	0
};

static class Win32Category : public ErrorCategoryBase {			// outside function to eliminate thread-safe static machinery
	typedef ErrorCategoryBase base;
public:
	Win32Category()
		: base("Win32", FACILITY_WIN32)
	{}

	string message(int eval) const override {
#if !UCFG_WDM
		TCHAR buf[256];

		if (eval >= INTERNET_ERROR_BASE && eval <= INTERNET_ERROR_LAST) {
			if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, LPCVOID(GetModuleHandle(_T("wininet.dll"))),
				eval, 0, buf, sizeof buf, 0))
				return "WinInet: " + String(buf).Trim();
		} else if (eval >= WSABASEERR && eval<WSABASEERR + 1024) {
			HMODULE hModuleThis = 0;
			GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)&s_win32code2errc,	&hModuleThis);
			if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
				hModuleThis,
				eval, 0, buf, sizeof buf, 0))
				return String(buf).Trim();
		}
		if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, eval, 0, buf, sizeof buf, 0))
			return String(buf).Trim();
#endif
		return Convert::ToString(eval, 16);
	}

	error_condition default_error_condition(int errval) const noexcept override {
		int code;
		for (const Win32CodeErrc *p=s_win32code2errc; (code=p->Code); ++p)
			if (code == errval)
				return p->Errc;
		switch (errval) {
		case ERROR_HANDLE_EOF:			return ExtErr::EndOfStream;
		case ERROR_WRONG_PASSWORD:
		case ERROR_INVALID_PASSWORD:
			return ExtErr::InvalidPassword;
		case ERROR_LOGON_FAILURE:		return ExtErr::LogonFailure;
		case ERROR_PWD_TOO_SHORT:		return ExtErr::PasswordTooShort;
		case ERROR_CRC: 				return ExtErr::Checksum;
		}
		return error_condition(errval, *this);
	}

	bool equivalent(int errval, const error_condition& c) const noexcept override {			//!!!TODO
		switch (errval) {
		case ERROR_TOO_MANY_OPEN_FILES:	return c==errc::too_many_files_open || c==errc::too_many_files_open_in_system;
		default:
			return base::equivalent(errval, c);
		}
	}

	bool equivalent(const error_code& ec, int errval) const noexcept override {
		if (ec.category() == system_category())
			return errval == ec.value();
		if (ec.category() == hresult_category() && HRESULT_FACILITY(ec.value()) == FACILITY_WIN32)
			return errval == (ec.value() & 0xFFFF);
		else
			return base::equivalent(ec, errval);
	}

} s_win32Category;

const error_category& AFXAPI win32_category() {
	return s_win32Category;
}


#endif // UCFG_WIN32

} // ExtSTL::

namespace std {

ostream& operator<<(ostream& os, const error_code& ec) {
	os << ec.category().name() << ":";
	if (ec.category() == Ext::hresult_category())
		os << hex;
	os << ec.value();
	return os;
}

} // std::

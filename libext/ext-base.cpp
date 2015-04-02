/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <windows.h>
#endif

#ifdef __linux__
#	include <sys/syscall.h>
#endif

#if UCFG_WIN32 && !UCFG_MINISTL
#	include <shlwapi.h>

#	include <el/libext/win32/ext-win.h>
#endif

namespace Ext {
using namespace std;

bool AFXAPI operator==(const ConstBuf& x, const ConstBuf& y) {
	if (!x.P || !y.P)
		return x.P == y.P;
	return x.Size==y.Size &&
		(x.P==y.P || !memcmp(x.P, y.P, x.Size));
}

const class CRuntimeClass Object::classObject =
	{ "Object", sizeof(Object), 0xffff, NULL, 0, NULL };

/*!!!R CRuntimeClass *Object::GetRuntimeClass() const {
	return RUNTIME_CLASS(Object);
}*/

#ifdef _AFXDLL
	CRuntimeClass* PASCAL Object::_GetBaseClass() {
		return NULL;
	}
	CRuntimeClass* PASCAL Object::GetThisClass() {
		return _RUNTIME_CLASS(Object);
	}
#endif

bool CHandleBaseBase::Close(bool bFromDtor) {
	bool r = false;
	bool prev = false;
	if (m_abClosed.compare_exchange_strong(prev, true)) {
#if !defined(_MSC_VER) || defined(_CPPUNWIND)
		if (bFromDtor && InException) {
			try {
				r = Release();
			} catch (RCExc) {
			}
		} else
#endif
			r = Release();
	}
	return r;
}


#ifndef WDM_DRIVER
EXT_THREAD_PTR(void) SafeHandle::t_pCurrentHandle;
#endif

SafeHandle::HandleAccess::~HandleAccess() {	
#if !defined(_MSC_VER) || defined(_CPPUNWIND)
			try //!!!
#endif
			{
				Release();
			}
#if !defined(_MSC_VER) || defined(_CPPUNWIND)
			catch (RCExc) {
			}
#endif
}


SafeHandle::SafeHandle(intptr_t handle)
	:	m_aHandle(handle)
	,	m_invalidHandleValue(-1)
#ifdef WDM_DRIVER
	,	m_pObject(nullptr)
#endif
{
	m_abClosed = false;
	m_aInUse = 1;

#ifdef WIN32
	Win32Check(Valid());
#endif
}

SafeHandle::~SafeHandle() {
	InternalReleaseHandle();
}

void SafeHandle::InternalReleaseHandle() const {
	intptr_t h = m_aHandle.exchange(m_invalidHandleValue);
	if (h != m_invalidHandleValue) {
		if (m_bOwn) //!!!
			ReleaseHandle(h);
	}
#if UCFG_WDM
	if (m_CreatedReference) {
		if (m_pObject) {
			ObDereferenceObject(m_pObject);
			m_pObject = nullptr;
		}
		m_CreatedReference = false;
	}
#endif
}

#if UCFG_WDM
void SafeHandle::AttachKernelObject(void *pObj, bool bKeepRef) {
	InternalReleaseHandle();
	m_CreatedReference = bKeepRef;
	m_pObject = pObj;
}

NTSTATUS SafeHandle::InitFromHandle(HANDLE h, ACCESS_MASK DesiredAccess, POBJECT_TYPE ObjectType, KPROCESSOR_MODE  AccessMode) {
	KEVENT *p;
	NTSTATUS st = ObReferenceObjectByHandle(h, DesiredAccess, ObjectType, AccessMode, (void**)&p, 0);
	AttachKernelObject(p, true);
	return st;
}

#endif

void SafeHandle::ReleaseHandle(intptr_t h) const {
#if UCFG_USE_POSIX
	CCheck(::close((int)h));
#elif UCFG_WIN32
	Win32Check(::CloseHandle((HANDLE)h));
#else
	NtCheck(::ZwClose((HANDLE)h));
#endif
}

/*!!!
void SafeHandle::CloseHandle() {
if (Valid() && m_bOwn)
Win32Check(::CloseHandle(exchange(m_handle, (HANDLE)0)));
}*/

intptr_t SafeHandle::DangerousGetHandle() const {
	if (!m_aInUse)
		Throw(E_EXT_ObjectDisposed);
	return m_aHandle.load();
}

void SafeHandle::AfterAttach(bool bOwn) {
	if (Valid()) {
		m_aInUse = 1;
		m_abClosed = false;
		m_bOwn = bOwn;
	} else {
		m_aHandle = m_invalidHandleValue;
#if UCFG_USE_POSIX
		CCheck(-1);
#elif UCFG_WIN32
		Win32Check(false);
#endif
	}
}

void SafeHandle::ThreadSafeAttach(intptr_t handle, bool bOwn) {
	intptr_t prev = m_invalidHandleValue;
	if (m_aHandle.compare_exchange_strong(prev, handle))
		AfterAttach(bOwn);
	else
		ReleaseHandle(handle);		
}

void SafeHandle::Attach(intptr_t handle, bool bOwn) {
	if (Valid())
		Throw(E_EXT_AlreadyOpened);
	m_aHandle = handle;
	AfterAttach(bOwn);
}

intptr_t SafeHandle::Detach() { //!!!
	m_abClosed = true;
	m_aInUse = 0;
	m_bOwn = false;
	return m_aHandle.exchange(m_invalidHandleValue);
}

void SafeHandle::Duplicate(intptr_t h, uint32_t dwOptions) {
#if UCFG_WIN32
	if (Valid())
		Throw(E_EXT_AlreadyOpened);
	HANDLE hMy;
	Win32Check(::DuplicateHandle(GetCurrentProcess(), (HANDLE)h, GetCurrentProcess(), &hMy, 0, FALSE, dwOptions));
	m_aHandle = intptr_t(hMy);
	m_aInUse = 1;
	m_abClosed = false;
#else
	Throw(E_NOTIMPL);
#endif
}

bool SafeHandle::Valid() const {
	return m_aHandle != 0 && m_aHandle != m_invalidHandleValue;
}

SafeHandle::BlockingHandleAccess::BlockingHandleAccess(const SafeHandle& h)
	:	HandleAccess(h)
{
#ifndef WDM_DRIVER
	m_pPrev = (HandleAccess*)(void*)t_pCurrentHandle;
	t_pCurrentHandle = this;
#endif

	/*!!!
	if (m_sock.m_socketThreader)
	{
	if (m_sock.m_socketThreader->m_bClosing)
	m_sock.Close();
	//!!!R			m_sock.m_socketThreader->m_lock.Lock();	
	m_prevKeeper = exchange(m_sock.m_socketThreader->m_pCurrentHandleKeeper, this);
	}*/
}

SafeHandle::BlockingHandleAccess::~BlockingHandleAccess() {
#ifndef WDM_DRIVER
	t_pCurrentHandle = m_pPrev;
#endif

	/*!!!
	if (m_sock.m_socketThreader)
	{
	if (m_sock.m_socketThreader->m_bClosing)
	m_sock.Close();
	m_sock.m_socketThreader->m_pCurrentHandleKeeper = m_prevKeeper;
	//!!!R			m_sock.m_socketThreader->m_lock.Unlock();	
	}*/
}

#if !UCFG_WDM
thread_specific_ptr<String> Exception::t_LastStringArg;
#endif

String Exception::get_Message() const {
	ostringstream os;
	os << "Error ";
#if !UCFG_WDM
	os << setw(8) << hex << ToHResult(_self) << ": ";
#endif
	os << dec << code() << " " << code().message();
	return os.str();
}

const char *Exception::what() const noexcept {
	try {
		if (m_message.empty())
			m_message = get_Message();
		return m_message.c_str();
	} catch (RCExc) {
		return "Error: double exception in the Exception::what()";
	}
}

String Exception::ToString() const {
	String r = get_Message().TrimEnd();
	for (CDataMap::const_iterator i=Data.begin(), e=Data.end(); i!=e; ++i)
		r += "\n  "+i->first+": "+i->second;
#if !UCFG_WCE
	if (CStackTrace::Use)
		r += "\n  " + StackTrace.ToString();
#endif
	return r;
}


static class HResultCategory : public error_category {		// outside function to eliminate thread-safe static machinery
	typedef error_category base;

	const char *name() const noexcept override { return "HResult"; }

	string message(int eval) const override {
#if UCFG_WDM
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

const error_category& hresult_category() {
	return s_hresultCategory;
}

Exception::Exception(HRESULT hr, RCString message)
	:	base(hr, hresult_category(), message.empty() ? string() : string(explicit_cast<string>(message)))
	,	m_message(message)
{
#if !UCFG_WCE
	if (CStackTrace::Use)
		StackTrace = CStackTrace::FromCurrentThread();
#endif
}

Exception::Exception(const error_code& ec, RCString message)
	:	base(ec, message.empty() ? string() : string(explicit_cast<string>(message)))
	, m_message(message)
{
#if !UCFG_WCE
	if (CStackTrace::Use)
		StackTrace = CStackTrace::FromCurrentThread();
#endif
}

intptr_t __stdcall AfxApiNotFound() {
	Throw(HRESULT_OF_WIN32(ERROR_PROC_NOT_FOUND));
}

DECLSPEC_NORETURN void AFXAPI ThrowImp(const error_code& ec) {
	HRESULT hr = ec.value();
#if UCFG_WDM && !_HAS_EXCEPTIONS
	KeBugCheck(hr);
#else
	if (ec.category() != hresult_category())
		throw Exception(ec);
	switch (hr) {
	case E_ACCESSDENIED:			throw AccessDeniedException();
	case E_OUTOFMEMORY:				throw bad_alloc();
	case E_INVALIDARG:				throw ArgumentExc();
	case E_EXT_EndOfStream:			throw EndOfStreamException();
	case E_EXT_FileFormat:			throw FileFormatException();
	case E_NOTIMPL:					throw NotImplementedExc();
	case E_FAIL:					throw UnspecifiedException();
	case E_EXT_ThreadInterrupted:	throw thread_interrupted();
	case E_EXT_InvalidCast:			throw bad_cast();
	case E_EXT_IndexOutOfRange:
#if UCFG_WDM
		throw out_of_range("Out of range");
#else
		throw out_of_range(explicit_cast<string>(HResultToMessage(E_EXT_IndexOutOfRange)));
#endif
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
				e.ProcName = *Exception::t_LastStringArg;
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


#if UCFG_CRT=='U' && !UCFG_WDM

typedef void (AFXAPI *PFNThrowImp)(HRESULT hr);
void SetThrowImp(PFNThrowImp pfn);
static int s_initThrowImp = (SetThrowImp(&ThrowImp), 1);

#endif // UCFG_CRT=='U' && !UCFG_WDM




DECLSPEC_NORETURN void AFXAPI ThrowImp(const error_code& ec, const char *funname, int nLine) {
#if UCFG_EH_SUPPORT_IGNORE
	if (!CLocalIgnoreBase::ErrorCodeIsIgnored(ec)) {
		TRC(1, funname <<  "(Ln" << nLine << "): " << (ec.category()==hresult_category() ? EXT_STR("HRESULT:" << hex << ec.value()) : EXT_STR(ec)) << " " << ec.message());
	}
#endif
	ThrowImp(ec);
}

DECLSPEC_NORETURN void AFXAPI ThrowImp(HRESULT hr, const char *funname, int nLine) {
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


#if !UCFG_WDM && !UCFG_MINISTL

String g_ExceptionMessage;

#if UCFG_WIN32
void AFXAPI ProcessExceptionInFilter(EXCEPTION_POINTERS *ep) {
	ostringstream os;
	os << "Code:\t" << hex << ep->ExceptionRecord->ExceptionCode << "\n"
		<< "Address:\t" << ep->ExceptionRecord->ExceptionAddress << "\n";
	g_ExceptionMessage = os.str();
	TRC(0, g_ExceptionMessage);
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
	} catch (const Exception& ex) {
		TRC(0, ex);
		wcerr << ex.what() << endl;
#if UCFG_GUI
		if (!IsConsole())
			MessageBox::Show(ex.what());
#endif
#if !UCFG_CATCH_UNHANDLED_EXC
		throw;
#endif
	} catch (std::exception& e) {
		TRC(0, e.what());
		cerr << e.what() << endl;
#if UCFG_GUI
	if (!IsConsole())
		MessageBox::Show(e.what());
#endif
#if !UCFG_CATCH_UNHANDLED_EXC
		throw;
#endif
	} catch (...) {
		TRC(0, "Unknown C++ exception");
		cerr << "Unknown C++ Exception" << endl;
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



String CEscape::Escape(CEscape& esc, RCString s) {
	UTF8Encoding utf8;
	Blob blob = utf8.GetBytes(s);
	size_t size = blob.Size;
	ostringstream os;
	const char *p = (const char*)blob.constData(); 
	for (size_t i=0; i<size; ++i) {
		char ch = p[i];
		esc.EscapeChar(os, ch);
	}
	return os.str();
}

String CEscape::Unescape(CEscape& esc, RCString s) {
	vector<byte> v;
	istringstream is(s.c_str());
	for (int ch; (ch=esc.UnescapeChar(is))!=EOF;)
		v.push_back((byte)ch);
	UTF8Encoding utf8;
	if (v.empty())
		return String();
	return utf8.GetChars(ConstBuf(&v[0], v.size()));
}

String AssertFailedExc::get_Message() const {
	return EXT_STR(FileName << "(" << LineNumber << "): Assertion Failed: " << Exp);
}

AFX_API bool AFXAPI AfxAssertFailedLine(const char* sexp, const char*fileName, int nLine) {
/*!!!R #	if defined(WIN32) && !defined(_AFX_NO_DEBUG_CRT)
		// we remove WM_QUIT because if it is in the queue then the message box
		// won't display
		MSG msg;
		BOOL bQuit = PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
		BOOL bResult = _CrtDbgReport(_CRT_ASSERT, lpszFileName, nLine, NULL, NULL);
		if (bQuit)
			PostQuitMessage((int)msg.wParam);
		if (bResult)
			Ext::AfxDebugBreak();
#	else */
#if UCFG_WDM
		KeBugCheck(HRESULT_FROM_WIN32(ERROR_ASSERTION_FAILURE));
#else
		throw AssertFailedExc(sexp, fileName, nLine);
#endif
//!!!#	endif
	return false;
}

#if UCFG_WCE

extern "C" {

#if !UCFG_STDSTL
int _daylight;
#endif

void __cdecl API_wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line) {
		throw AssertFailedExc(_Message, _File, _Line);
}

void _cdecl abort() {
	exit(3);
}

int _cdecl remove(const char *path) {
	bool b = ::DeleteFile(String(path));
	if (!b) {
		errno = EFAULT;		//!!!TODO
		return -1;
	}
	return 0;
}



} // "C"

#endif  // UCFG_WCE


#if UCFG_WCE || UCFG_USE_OLD_MSVCRTDLL

extern "C" {

#define IS_2_POW_N(X)   (((X)&(X-1)) == 0)
#define PTR_SZ          sizeof(void *)


__declspec(dllexport) void * __cdecl my_aligned_malloc(size_t size, size_t align)
{
	uintptr_t ptr, retptr, gap, offset;
	offset = 0;

/* validation section */
	ASSERT(IS_2_POW_N(align));
	ASSERT(offset == 0 || offset < size);

align = (align > PTR_SZ ? align : PTR_SZ) -1;

/* gap = number of bytes needed to round up offset to align with PTR_SZ*/
gap = (0 - offset)&(PTR_SZ -1);

if ( (ptr =(uintptr_t)Malloc(PTR_SZ +gap +align +size)) == (uintptr_t)NULL)
    return NULL;

retptr =((ptr +PTR_SZ +gap +align +offset)&~align)- offset;
((uintptr_t *)(retptr - gap))[-1] = ptr;

return (void *)retptr;
}
	
__declspec(dllexport) void  __cdecl my_aligned_free(void *memblock)
{
		uintptr_t ptr;

		if (memblock == NULL)
				return;

		ptr = (uintptr_t)memblock;

		/* ptr points to the pointer to starting of the memory block */
		ptr = (ptr & ~(PTR_SZ -1)) - PTR_SZ;

		/* ptr is the pointer to the start of memory block*/
		ptr = *((uintptr_t *)ptr);
		Free((void *)ptr);
}


} // "C"

#endif // UCFG_WCE || UCFG_USE_OLD_MSVCRTDLL



extern "C" void _cdecl AfxTestEHsStub(void *prevFrame) {
	if (((byte*)prevFrame - (byte*)&prevFrame) < 5*sizeof(void*)) {		
#if UCFG_WDM
		KeBugCheck(E_FAIL);
#else
		std::cerr << "Should be compiled with /EHs /EHc-" << endl;
		abort();														
#endif
	}
	//!!!ThrowImp(1);
}

size_t AFXAPI hash_value(const void * key, size_t len) {
	return MurmurHashAligned2(ConstBuf(key, len), _HASH_SEED);			
}


} // Ext::


extern "C" {

PFN_memcpy g_fastMemcpy = &memcpy;

#if UCFG_USE_MASM && UCFG_WIN32 && defined(_M_IX86)		//!!! UCFG_CPU_X86_X64

void *MemcpySse(void *dest, const void *src, size_t count);

static int InitFastMemcpy() {
	if (Ext::CpuInfo().HasSse)
		g_fastMemcpy = &MemcpySse;
	return 1;
}

static int s_initFastMemcpy = InitFastMemcpy();

#endif

#if UCFG_CPU_X86_X64
byte g_bHasSse2;

static int InitBignumFuns() {
	g_bHasSse2 = Ext::CpuInfo().HasSse2;
	return 1;
}

static int s_initBignumFuns = InitBignumFuns();
#endif // UCFG_CPU_X86_X64


} // "C"

namespace ExtSTL {
#if !UCFG_STDSTL || !UCFG_STD_MUTEX || UCFG_SPECIAL_CRT
	const adopt_lock_t adopt_lock = {};
	const defer_lock_t defer_lock = {};
	const try_to_lock_t try_to_lock = {};
#endif // !UCFG_STDSTL || !UCFG_STD_MUTEX

} // ExtSTL::



/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include EXT_HEADER(map)
#include EXT_HEADER_SYSTEM_ERROR

namespace Ext {
	using std::pair;
	using std::size;
	using std::vector;
	using std::map;
	using std::errc;
	using std::error_code;
	using std::error_condition;
	using std::system_error;
	using std::error_category;
	using std::observer_ptr;
	using std::atomic;
	using std::try_to_lock_t;

class CCriticalSection;
class Exception;

typedef const std::exception& RCExc;


inline DECLSPEC_NORETURN EXTAPI void AFXAPI ThrowImp(errc v) { ThrowImp(make_error_code(v)); }
inline DECLSPEC_NORETURN EXTAPI void AFXAPI ThrowImp(errc v, const char *funname, int nLine) { ThrowImp(make_error_code(v), funname, nLine); }

#if !UCFG_DEFINE_THROW
	DECLSPEC_NORETURN __forceinline void AFXAPI Throw(errc v) { ThrowImp(v); }
	DECLSPEC_NORETURN __forceinline void AFXAPI Throw(errc v, const char *funname, int nLine) { ThrowImp(v, funname, nLine); }
#endif
	
class CPrintable {
public:
	virtual ~CPrintable() {}
	virtual String ToString() const =0;
};


#if !UCFG_WCE
class CStackTrace : public CPrintable {
public:
	EXT_DATA static bool Use;

	int AddrSize;
	vector<uint64_t> Frames;

	CStackTrace()
		:	AddrSize(sizeof(void*))
	{}

	static CStackTrace AFXAPI FromCurrentThread();
	String ToString() const;
};
#endif

const error_category& AFXAPI win32_category();
const error_category& AFXAPI ntos_category();
const error_category& AFXAPI hresult_category();

class Exception : public system_error, public CPrintable {
	typedef std::system_error base;
public:
	typedef Exception class_type;

#if !UCFG_WDM
	static thread_specific_ptr<String> t_LastStringArg;
#endif

	mutable String m_message;

	typedef std::map<String, String> CDataMap;
	CDataMap Data;

	explicit Exception(HRESULT hr = 0, RCString message = "");
	explicit Exception(const error_code& ec, RCString message = "");
	~Exception() noexcept {}		//!!! necessary for GCC 4.6
	String ToString() const override;


	const char *what() const noexcept override;

#if !UCFG_WCE
	CStackTrace StackTrace;
#endif	
protected:
	virtual String get_Message() const;
private:

#ifdef _WIN64
	LONG dummy;
#endif
};

class ExcLastStringArgKeeper {
public:
	String m_prev;

	ExcLastStringArgKeeper(RCString s) {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		m_prev = exchange(*ps, s);
#endif
	}

	~ExcLastStringArgKeeper() {
#if !UCFG_WDM
		*Exception::t_LastStringArg = m_prev;
#endif
	}

	operator const String&() const {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		return *ps;
#endif
	}

	operator const char *() const {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		return ps->c_str();
#endif
	}

	operator const String::value_type *() const {
#if !UCFG_WDM
		String *ps = Exception::t_LastStringArg;
		if (!ps)
			Exception::t_LastStringArg.reset(ps = new String);
		return *ps;
#endif
	}
};

#define EXT_DEFINE_EXC(c, b, code) class c : public b { public: c(HRESULT hr = code) : b(hr) {} };
#define EXT_DEFINE_EXC_EC(c, b, code) class c : public b { public: c(ExtErr ec = code) : b(ec) {} };

EXT_DEFINE_EXC_EC(ArithmeticExc, Exception, ExtErr::Arithmetic)
EXT_DEFINE_EXC_EC(OverflowExc, ArithmeticExc, ExtErr::Overflow)
EXT_DEFINE_EXC(ArgumentExc, Exception, E_INVALIDARG)
EXT_DEFINE_EXC_EC(EndOfStreamException, Exception, ExtErr::EndOfStream)
EXT_DEFINE_EXC_EC(FileFormatException, Exception, ExtErr::FileFormat)
EXT_DEFINE_EXC(NotImplementedExc, Exception, E_NOTIMPL)
EXT_DEFINE_EXC(UnspecifiedException, Exception, E_FAIL)
EXT_DEFINE_EXC(AccessDeniedException, Exception, E_ACCESSDENIED)

/*!!!R
class IOExc : public Exception {
	typedef Exception base;
public:
	path FileName;

	IOExc(HRESULT hr)
		: base(hr) {
	}

	~IOExc() noexcept {}

	String get_Message() const override {
		String r = base::get_Message();
		if (!FileName.empty())
			r += " " + FileName.native();
		return r;
	}
};

class FileNotFoundException : public IOExc {
	typedef IOExc base;
public:
	FileNotFoundException()
		: base(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
	}
};

class FileAlreadyExistsExc : public IOExc {
	typedef IOExc base;
public:
	FileAlreadyExistsExc()
		: base(HRESULT_FROM_WIN32(ERROR_FILE_EXISTS)) {
	}
};

class DirectoryNotFoundExc : public Exception {
	typedef Exception base;
public:
	DirectoryNotFoundExc()
		: base(HRESULT_OF_WIN32(ERROR_PATH_NOT_FOUND)) {
	}
};
*/

class thread_interrupted : public Exception {
public:
	thread_interrupted()
		:	Exception(ExtErr::ThreadInterrupted)
	{}
};

class SystemException : public Exception {
public:
	SystemException(const error_code& ec = error_code(), RCString message = "")
		:	Exception(ec, message)
	{
	}
};


class CryptoException : public Exception {
	typedef Exception base;
public:
	explicit CryptoException(const error_code& ec, RCString message)
		:	base(ec, message)
	{}
};

class StackOverflowExc : public Exception {
	typedef Exception base;
public:
	StackOverflowExc()
		:	base(HRESULT_FROM_WIN32(ERROR_STACK_OVERFLOW))
	{}

	CInt<void *> StackAddress;
};

class ProcNotFoundExc : public Exception {
	typedef Exception base;
public:
	String ProcName;

	ProcNotFoundExc()
		:	base(HRESULT_OF_WIN32(ERROR_PROC_NOT_FOUND))
	{
	}

	~ProcNotFoundExc() noexcept {}

protected:
	String get_Message() const override {
		String r = base::get_Message();
		if (!ProcName.empty())
			r += " " + ProcName;
		return r;
	}
};


intptr_t __stdcall AfxApiNotFound();
#define DEF_DELAYLOAD_THROW static FARPROC WINAPI DliFailedHook(unsigned dliNotify, PDelayLoadInfo  pdli) { return &AfxApiNotFound; } \
							static struct InitDliFailureHook {  InitDliFailureHook() { __pfnDliFailureHook2 = &DliFailedHook; } } s_initDliFailedHook;

extern "C" AFX_API void _cdecl AfxTestEHsStub(void *prevFrame);

//!!! DBG_LOCAL_IGNORE_NAME(1, ignOne);	

#define DEF_TEST_EHS							\
	static DECLSPEC_NOINLINE int AfxTestEHs(void *p) {			\
		try {								\
			AfxTestEHsStub(&p);					\
		} catch (...) {							\
		}										\
		return 0;								\
	}											\
	static int s_testEHs  = AfxTestEHs(&AfxTestEHs);									\


class AssertFailedExc : public Exception {
	typedef Exception base;
public:
	String Exp;
	String FileName;
	int LineNumber;

	AssertFailedExc(RCString exp, RCString fileName, int line)
		:	base(HRESULT_FROM_WIN32(ERROR_ASSERTION_FAILURE))
		,	Exp(exp)
		,	FileName(fileName)
		,	LineNumber(line)
	{
	}

	~AssertFailedExc() noexcept {}
protected:
	String get_Message() const override;
};


template <typename H>
struct HandleTraitsBase {
	typedef H handle_type;

	static bool ResourceValid(const handle_type& h) {
		return h != 0;
	}

	static handle_type ResourseNull() {
		return 0;
	}
};

template <typename H>
struct HandleTraits : HandleTraitsBase<H> {
};

/*!!!
template <typename H>
inline void ResourceRelease(const H& h) {
	Throw(E_NOTIMPL);
}*/

template <typename H>
class ResourceWrapper {
public:
	typedef HandleTraits<H> traits_type;
	typedef typename HandleTraits<H>::handle_type handle_type;

	handle_type m_h;		//!!! should be private
    
	ResourceWrapper()
		:	m_h(traits_type::ResourseNull())
	{}

	ResourceWrapper(const ResourceWrapper& res)
		:	m_h(traits_type::ResourseNull())
	{
		operator=(res);
	}

	~ResourceWrapper() {
		if (Valid()) {
			if (!InException)
				traits_type::ResourceRelease(m_h);
			else {
				try {
					traits_type::ResourceRelease(m_h);
				} catch (RCExc&) {
				//	TRC(0, e);
				}
			}
		}
	}

	bool Valid() const {
		return traits_type::ResourceValid(m_h);
	}

	void Close() {
		if (Valid())
			traits_type::ResourceRelease(exchange(m_h, traits_type::ResourseNull()));
	}

	handle_type& OutRef() {
		if (Valid())
			Throw(ExtErr::AlreadyOpened);
		return m_h;
	}

	handle_type Handle() const {
		if (!Valid())
			Throw(E_HANDLE);
		return m_h;
	}

	handle_type operator()() const { return Handle(); }

	handle_type& operator()() {
		if (!Valid())
			Throw(E_HANDLE);
		return m_h;		
	}

	ResourceWrapper& operator=(const ResourceWrapper& res) {
		handle_type tmp = exchange(m_h, traits_type::ResourseNull());
		if (traits_type::ResourceValid(tmp))
			traits_type::ResourceRelease(tmp);
		if (traits_type::ResourceValid(res.m_h))
			m_h = traits_type::Retain(res.m_h);
		return *this;
	}
private:
	CInException InException;
};


class CEscape {
public:
	virtual ~CEscape() {}

	virtual void EscapeChar(std::ostream& os, char ch) {
		os.put(ch);
	}

	virtual int UnescapeChar(std::istream& is) {
		return is.get();
	}

	static AFX_API String AFXAPI Escape(CEscape& esc, RCString s);
	static AFX_API String AFXAPI Unescape(CEscape& esc, RCString s);
};

struct CExceptionFabric {
	CExceptionFabric(int facility);
	virtual DECLSPEC_NORETURN void ThrowException(HRESULT hr, RCString msg) =0;
};

DECLSPEC_NORETURN void AFXAPI ThrowS(HRESULT hr, RCString msg);



} // Ext::


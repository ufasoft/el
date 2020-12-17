// Code common for Win32 and Kernel mode

#pragma once

#ifndef UCFG_STD_EXCHANGE
#	ifdef __cpp_lib_exchange_function
#		define UCFG_STD_EXCHANGE 1
#	else
#		define UCFG_STD_EXCHANGE (UCFG_CPP14 || UCFG_LIBCPP_VERSION >= 1100)
#	endif
#endif


#if !UCFG_STD_EXCHANGE && !UCFG_MINISTL


/*!!!?R

template <typename T, typename U>
inline T Do_exchange(T& obj, const U EXT_REF new_val, false_type) {
#	if UCFG_CPP11_RVALUE
	T old_val = std::move(obj);
	obj = std::forward<U>(new_val);
#else
	T old_val = obj;
	obj = new_val;
#endif
  	return old_val;
}

template <typename T, typename U>
inline T Do_exchange(T& obj, const U& new_val, false_type) {
#	if UCFG_CPP11_RVALUE
	T old_val = std::move(obj);
#else
	T old_val = obj;
#endif
	obj = new_val;
  	return old_val;
}

template <typename T, typename U>
inline T Do_exchange(T& obj, const U new_val, true_type) {
#	if UCFG_CPP11_RVALUE
	T old_val = std::move(obj);
#else
	T old_val = obj;
#endif
	obj = new_val;
  	return old_val;
}

template <typename T, typename U>
inline T exchange(T& obj, U EXT_REF new_val) {
	return Do_exchange(obj, new_val, typename is_scalar<U>::type());
}

#	if UCFG_CPP11_RVALUE

	template <typename T, typename U>
	inline T exchange(T& obj, const U& new_val) {
		return Do_exchange(obj, new_val, typename is_scalar<U>::type());
	}
#	endif
*/

namespace std {

template <typename T, typename U>
inline T exchange(T& obj, U EXT_REF new_val) {
	T old_val = std::move(obj);
#	if UCFG_CPP11_RVALUE
	obj = forward<U>(new_val);
#	else
	obj = new_val;
#	endif
	return old_val;
}

inline char exchange(char& obj, char new_val) {
	char old_val = obj;
	obj = new_val;
	return old_val;
}

inline int exchange(int& obj, int new_val) {
	int old_val = obj;
	obj = new_val;
	return old_val;
}

inline long exchange(long& obj, long new_val) {
	long old_val = obj;
	obj = new_val;
	return old_val;
}
} // std::
#endif // !UCFG_STD_EXCHANGE


namespace Ext {

class NonInterlockedPolicy {
public:
	template <class T> static int Increment(T& v) { int r = v + 1; v = r; return r; }
	template <class T> static int Decrement(T& v) { int r = v - 1; v = r; return r; }
};

class InterlockedPolicy {
public:
	template <class T> static T Increment(atomic<T>& a) { return ++a; }
	template <class T> static T Decrement(atomic<T>& a) { return --a; }
};

template <typename T>
struct ptr_traits {
	typedef typename T::interlocked_policy interlocked_policy;
};


	/*!!!
#if UCFG_USE_BOOST
	using boost::hash_value;
#endif*/


class Stream;
class Blob;
class String;
typedef const String& RCString;
	//!!!using namespace std;

#ifdef WIN32
	class COleVariant;
#endif

template <typename T> T exchangeZero(T& v) { return std::exchange(v, (T)0); }

//#define SwapRet std::exchange						//!!!O
//#define SwapRetZero exchangeZero					//!!!O

class CBool {
public:
	CBool(bool b = false)
		:	m_b(b)
	{}

	CBool(const CBool& v)
		:	m_b(v.m_b)
	{}

	operator bool() const volatile { return m_b; }

/*!!!R	EXPLICIT_OPERATOR_BOOL() const volatile {
		return m_b ? EXT_CONVERTIBLE_TO_TRUE : 0;
	}*/

	/*!!!R
	struct _Boolean { int i; }; operator int _Boolean::*() const volatile {
		return m_b ? &_Boolean::i : 0;
	}*/

	bool& Ref() { return m_b; }
private:
	bool m_b;
};

template <typename T>
class CInt {
public:
	CInt(T v = 0)
		:	m_v(v)
	{}

	operator T() const { return m_v; }
	operator T&() { return m_v; }

	T Value() const { return m_v; }

	CInt& operator++() { ++m_v; return (*this); }
	T operator++(int) { return m_v++; }	
private:
	T m_v;
};

template <typename T>
class Keeper : noncopyable {
public:
	volatile T& m_t; 
	T m_prev;

	Keeper(T& t, const T& v)
		:	m_t(t)
		,	m_prev(m_t)
	{
		t = v;
	}

	Keeper(volatile T& t, const T& v)
		:	m_t(t)
		,	m_prev(m_t)
	{
		t = v;
	}

	~Keeper() {
		m_t = m_prev;
	}
};

class CBoolKeeper : public Keeper<bool> {
	typedef Keeper<bool> base;
public:
	CBoolKeeper(bool& b, bool n = true)
		:	base(b, n)
	{}

	CBoolKeeper(volatile bool& b, bool n = true)
		:	base(b, n)
	{}
};

} // Ext::

#ifdef WIN32

//#include <windows.h>
#include <tchar.h>

namespace Ext {
/*!!!	inline const TCHAR *GetCommandLineArgs() {
		const TCHAR *pCL = ::GetCommandLine();
		if (*pCL == _T('\"')) {
			pCL++;
			while (*pCL && *pCL++ != _T('\"'))
				;
		} else
			while (*pCL > _T(' '))
				pCL++;
		while (*pCL && *pCL <= _T(' '))
			pCL++;
		return pCL;
	}*/
}



#endif	

namespace Ext {



class LEuint16_t {	//!!!
	unsigned short m_val;
public:
	operator unsigned short() const { return m_val; } 
};

template <class T> inline void ZeroStruct(T& s) {
	memset(&s, 0, sizeof(T));
}

#define STATIC_PROPERTY(OWNERNAME, TYPE, NAME, FNGET, FNPUT) \
	static struct propclass_##NAME { \
	inline operator TYPE() const{ \
	return OWNERNAME::FNGET(); \
	} \
	inline void operator=(TYPE src) { \
	OWNERNAME::FNPUT(src); \
	} \
	} NAME;

#define STATIC_PROPERTY_GET(OWNERNAME, TYPE, NAME, FNGET) \
	static struct propclass_##NAME { \
	inline operator TYPE() const { return OWNERNAME::FNGET(); } \
	inline TYPE operator->() const { return OWNERNAME::FNGET(); } \
	} NAME;


#define STATIC_PROPERTY_DEF(OWNERNAME, TYPE, NAME) \
	STATIC_PROPERTY(OWNERNAME, TYPE, NAME, get_##NAME, put_##NAME)

#define STATIC_PROPERTY_DEF_GET(OWNERNAME, TYPE, NAME) \
	STATIC_PROPERTY_GET(OWNERNAME, TYPE, NAME, get_##NAME)

#ifdef _MSC_VER

#	define DEFPROP(TYPE, NAME) \
		__declspec(property(get=get_##NAME, put=put_##NAME)) TYPE NAME;
#	define DEFPROP_CONST(TYPE, NAME)		DEFPROP(TYPE, NAME)
#	define DEFPROP_CONST_CONST(TYPE, NAME)	DEFPROP(TYPE, NAME)

#	define DEFPROP_GET(TYPE, NAME) \
		__declspec(property(get=get_##NAME)) TYPE NAME;
#	define DEFPROP_GET_CONST(TYPE, NAME) DEFPROP_GET(TYPE, NAME)

// because VC calls base class's property method if called as Property from derived pointer
// BUG-Report: https://connect.microsoft.com/VisualStudio/feedback/details/621165
#	define DEFPROP_VIRTUAL_GEN(TYPE, NAME, gc, pc) \
		inline TYPE vget_##NAME() gc { return get_##NAME(); }	\
		inline void vput_##NAME(TYPE const& v) pc { put_##NAME(v); }	\
		__declspec(property(get=vget_##NAME, put=vput_##NAME)) TYPE NAME;

#	define DEFPROP_VIRTUAL(TYPE, NAME)				DEFPROP_VIRTUAL_GEN(TYPE, NAME,,)
#	define DEFPROP_VIRTUAL_CONST(TYPE, NAME)		DEFPROP_VIRTUAL_GEN(TYPE, NAME, const, )
#	define DEFPROP_VIRTUAL_CONST_CONST(TYPE, NAME)	DEFPROP_VIRTUAL_GEN(TYPE, NAME, const, const)

#	define DEFPROP_VIRTUAL_GET(TYPE, NAME) \
		inline TYPE vget_##NAME() { return get_##NAME(); }	\
		__declspec(property(get=vget_##NAME)) TYPE NAME;

#	define DEFPROP_VIRTUAL_GET_CONST(TYPE, NAME) \
		inline TYPE vget_##NAME() const { return get_##NAME(); }	\
		__declspec(property(get=vget_##NAME)) TYPE NAME;

#else // _MSC_VER

	//!!!#include "extwin32.h"

#	ifndef CONTAINING_RECORD
#		ifdef __GNUC__
#			define CONTAINING_RECORD(address, type, field) ((type *)( \
				(char*)(address) - __builtin_offsetof(type, field)))
#		else
#			define CONTAINING_RECORD(address, type, field) ((type *)( \
				(char*)(address) - (int)(&((type *)0)->field)))
#		endif
#	endif																										


#	define PROPERTY(OWNERNAME, TYPE, NAME, FNGET, FNPUT) 					\
	struct propclass_##NAME { 											\
		inline operator TYPE() const{ 									\
			return CONTAINING_RECORD(this, OWNERNAME, NAME)->FNGET(); 	\
		} 																\
		inline void operator=(TYPE src) { 								\
			CONTAINING_RECORD(this, OWNERNAME, NAME)->FNPUT(src); 		\
		} 																\
		inline void operator=(const propclass_##NAME& src) { 			\
			operator=((TYPE)src); 										\
		} 																\
		template <typename CT> 											\
		inline bool operator<(const CT& src) const { 					\
			return (operator TYPE()) < src; 							\
		}								\
										\
		template <typename CT>  														\
		inline bool operator==(const CT& src) const { 				\
			return (operator TYPE()) == src; 							\
		}					\
	} NAME;

#	define PROPERTY_GET(OWNERNAME, TYPE, NAME, FNGET) 						\
	struct propclass_##NAME { 											\
		inline operator TYPE() const { 									\
			return CONTAINING_RECORD(this, OWNERNAME, NAME)->FNGET(); 	\
		}								\
										\
		template <typename CT> 								\
		inline bool operator<(const CT& src) const { 					\
			return (operator TYPE()) < src; 							\
		}								\
										\
		template <typename CT>  														\
		inline bool operator==(const CT& src) const { 				\
			return (operator TYPE()) == src; 							\
		} 																\
	} NAME;


#	define DEFPROP(TYPE, NAME) \
	PROPERTY(class_type, TYPE, NAME, get_##NAME, put_##NAME)
#	define DEFPROP_CONST(TYPE, NAME) \
	PROPERTY(class_type, TYPE, NAME, get_##NAME, put_##NAME)
#	define DEFPROP_CONST_CONST(TYPE, NAME) \
	PROPERTY(class_type, TYPE, NAME, get_##NAME, put_##NAME)
#	define DEFPROP_VIRTUAL(TYPE, NAME) \
	PROPERTY(class_type, TYPE, NAME, get_##NAME, put_##NAME)
#	define DEFPROP_VIRTUAL_CONST(TYPE, NAME) \
	PROPERTY(class_type, TYPE, NAME, get_##NAME, put_##NAME)
#	define DEFPROP_VIRTUAL_CONST_CONST(TYPE, NAME) \
	PROPERTY(class_type, TYPE, NAME, get_##NAME, put_##NAME)


#	define DEFPROP_GET(TYPE, NAME) \
		PROPERTY_GET(class_type, TYPE, NAME, get_##NAME)
#	define DEFPROP_GET_CONST(TYPE, NAME) \
		PROPERTY_GET(class_type, TYPE, NAME, get_##NAME)
#	define DEFPROP_VIRTUAL_GET(TYPE, NAME) \
		PROPERTY_GET(class_type, TYPE, NAME, get_##NAME)
#	define DEFPROP_VIRTUAL_GET_CONST(TYPE, NAME) \
		PROPERTY_GET(class_type, TYPE, NAME, get_##NAME)

#endif // _MSC_VER

template <class T> class CPointerKeeper {
public:
	CPointerKeeper(std::observer_ptr<T>& p, T *q)
		:	m_p(p)
	{
		m_old = p.get();
		p.reset(q);
	}

	~CPointerKeeper() {
		m_p.reset(m_old);
	}
private:
	std::observer_ptr<T>& m_p;
	T *m_old;
};


struct Buf {
	unsigned char *P;
	size_t Size;

	Buf(void *p = 0, size_t siz = 0)
		:	P((unsigned char*)p)
		,	Size(siz)
	{}
};


size_t AFXAPI hash_value(const void *key, size_t len);

#undef AFX_DATA //!!!
#define AFX_DATA AFX_CORE_DATA

class CRuntimeClass;
//class ostream;
//typedef ostream CDumpContext;


class Object {
public:
	typedef NonInterlockedPolicy interlocked_policy;

	static const AFX_DATA CRuntimeClass classObject;
	mutable atomic<int> m_aRef;

	Object()
		:	m_aRef(0)
	{
	}

	Object(const Object& ob)
		: m_aRef(0)
	{
	}

	Object& operator=(const Object& ob) {
		m_aRef = 0;
		return *this;
	}

	virtual ~Object() {
	}

	bool IsHeaped() const { return m_aRef < (10000); }			//!!! should be replaced to some num limits
	void InitInStack() { m_aRef = 20000; }

//!!!R	virtual CRuntimeClass *GetRuntimeClass() const;

#ifdef _AFXDLL
	static CRuntimeClass* PASCAL _GetBaseClass();
	static CRuntimeClass* PASCAL GetThisClass();
#endif

};

	// generate static object constructor for class registration
	void AFXAPI AfxClassInit(CRuntimeClass* pNewClass);
	struct AFX_CLASSINIT
	{ AFX_CLASSINIT(CRuntimeClass* pNewClass) { AfxClassInit(pNewClass); } };

	class /*!!!AFX_CLASS*/ CRuntimeClass {
	public:
		const char *m_lpszClassName;
		int m_nObjectSize;
		uint32_t m_wSchema; // schema number of the loaded class
		Object* (PASCAL* m_pfnCreateObject)(); // NULL => abstract class
#ifdef _AFXDLL
		CRuntimeClass* (PASCAL* m_pfnGetBaseClass)();
#else
		CRuntimeClass* m_pBaseClass;
#endif

		CRuntimeClass* m_pNextClass;       // linked list of registered classes
		const AFX_CLASSINIT* m_pClassInit;

		Object *CreateObject()
		{
			return m_pfnCreateObject();
		}

		bool IsDerivedFrom(const CRuntimeClass *pBaseClass) const;
	};

	//////////////////////////////////////////////////////////////////////////////
	// Helper macros for declaring CRuntimeClass compatible classes

#define _RUNTIME_CLASS(class_name) ((CRuntimeClass*)(&class_name::class##class_name))
#ifdef _AFXDLL
#	define RUNTIME_CLASS(class_name) (class_name::GetThisClass())
#else
#	define RUNTIME_CLASS(class_name) _RUNTIME_CLASS(class_name)
#endif

#define AFX_DATADEF
#define AFX_COMDAT
#define BASED_CODE

#ifndef WDM_DRIVER
#	define EXPORT
#endif


#ifdef _AFXDLL
#define DECLARE_DYNAMIC(class_name) \
protected: \
	static CRuntimeClass* PASCAL _GetBaseClass(); \
public: \
	static const CRuntimeClass class##class_name; \
	static CRuntimeClass* PASCAL GetThisClass(); \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#define _DECLARE_DYNAMIC(class_name) \
protected: \
	static CRuntimeClass* PASCAL _GetBaseClass(); \
public: \
	static CRuntimeClass class##class_name; \
	static CRuntimeClass* PASCAL GetThisClass(); \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#else
#define DECLARE_DYNAMIC(class_name) \
public: \
	static const CRuntimeClass class##class_name; \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#define _DECLARE_DYNAMIC(class_name) \
public: \
	static CRuntimeClass class##class_name; \
	virtual CRuntimeClass* GetRuntimeClass() const; \

#endif

	// not serializable, but dynamically constructable
#define DECLARE_DYNCREATE(class_name) \
	DECLARE_DYNAMIC(class_name) \
	static Object* PASCAL CreateObject();

#define _DECLARE_DYNCREATE(class_name) \
	_DECLARE_DYNAMIC(class_name) \
	static Object* PASCAL CreateObject();

#define DECLARE_SERIAL(class_name) \
	_DECLARE_DYNCREATE(class_name) \
	AFX_API friend CArchive& AFXAPI operator>>(CArchive& ar, class_name* &pOb);

#ifdef _AFXDLL
#define IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	CRuntimeClass* PASCAL class_name::_GetBaseClass() \
	{ return RUNTIME_CLASS(base_class_name); } \
	AFX_COMDAT const CRuntimeClass class_name::class##class_name = { \
#class_name, sizeof(class class_name), wSchema, pfnNew, \
	&class_name::_GetBaseClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
	{ return _RUNTIME_CLASS(class_name); } \
	CRuntimeClass* class_name::GetRuntimeClass() const \
	{ return _RUNTIME_CLASS(class_name); } \

#define _IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	CRuntimeClass* PASCAL class_name::_GetBaseClass() \
	{ return RUNTIME_CLASS(base_class_name); } \
	AFX_COMDAT CRuntimeClass class_name::class##class_name = { \
#class_name, sizeof(class class_name), wSchema, pfnNew, \
	&class_name::_GetBaseClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
	{ return _RUNTIME_CLASS(class_name); } \
	CRuntimeClass* class_name::GetRuntimeClass() const \
	{ return _RUNTIME_CLASS(class_name); } \

#define IMPLEMENT_RUNTIMECLASS_T(class_name, T1, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT const CRuntimeClass class_name<T1>::class##class_name = { \
#class_name, sizeof(class class_name<T1>), wSchema, pfnNew, \
	&base_class_name<T1>::GetThisClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
	{ return _RUNTIME_CLASS_T(class_name, T1); } \
	CRuntimeClass* class_name<T1>::GetRuntimeClass() const \
	{ return _RUNTIME_CLASS_T(class_name, T1); } \

#define _IMPLEMENT_RUNTIMECLASS_T(class_name, T1, base_class_name, wSchema, pfnNew, class_init) \
	CRuntimeClass* PASCAL class_name<T1>::GetThisClass() \
	{ return RUNTIME_CLASS(base_class_name); } \
	AFX_COMDAT CRuntimeClass class_name<T1>::class##class_name = { \
#class_name, sizeof(class class_name<T1>), wSchema, pfnNew, \
	&base_class_name<T1>::GetThisClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
	{ return _RUNTIME_CLASS_T(class_name, T1); } \
	CRuntimeClass* class_name<T1>::GetRuntimeClass() const \
	{ return _RUNTIME_CLASS_T(class_name, T1); } \

#define IMPLEMENT_RUNTIMECLASS_T2(class_name, T1, T2, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT const CRuntimeClass class_name<T1, T2>::class##class_name = { \
#class_name, sizeof(class class_name<T1, T2>), wSchema, pfnNew, \
	&base_class_name<T1, T2>::GetThisClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
	{ return _RUNTIME_CLASS_T2(class_name, T1, T2); } \
	CRuntimeClass* class_name<T1, T2>::GetRuntimeClass() const \
	{ return _RUNTIME_CLASS_T2(class_name, T1, T2); } \

#define _IMPLEMENT_RUNTIMECLASS_T2(class_name, T1, T2, base_class_name, wSchema, pfnNew, class_init) \
	CRuntimeClass* PASCAL class_name<T1, T2>::GetThisClass() \
	{ return RUNTIME_CLASS(base_class_name); } \
	AFX_COMDAT CRuntimeClass class_name<T1, T2>::class##class_name = { \
#class_name, sizeof(class class_name<T1, T2>), wSchema, pfnNew, \
	&base_class_name<T1, T2>::GetThisClass, NULL, class_init }; \
	CRuntimeClass* PASCAL class_name::GetThisClass() \
	{ return _RUNTIME_CLASS_T2(class_name, T1, T2); } \
	CRuntimeClass* class_name<T1, T2>::GetRuntimeClass() const \
	{ return _RUNTIME_CLASS_T2(class_name, T1, T2); } \

#else
#define IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT const CRuntimeClass class_name::class##class_name = { \
#class_name, sizeof(class class_name), wSchema, pfnNew, \
	RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name::GetRuntimeClass() const \
	{ return RUNTIME_CLASS(class_name); } \

#define _IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT CRuntimeClass class_name::class##class_name = { \
#class_name, sizeof(class class_name), wSchema, pfnNew, \
	RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name::GetRuntimeClass() const \
	{ return RUNTIME_CLASS(class_name); } \

#define IMPLEMENT_RUNTIMECLASS_T(class_name, T1, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT const CRuntimeClass class_name<T1>::class##class_name = { \
#class_name, sizeof(class class_name<T1>), wSchema, pfnNew, \
	RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name<T1>::GetRuntimeClass() const \
	{ return RUNTIME_CLASS_T(class_name, T1); } \

#define _IMPLEMENT_RUNTIMECLASS_T(class_name, T1, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT CRuntimeClass class_name<T1>::class##class_name = { \
#class_name, sizeof(class class_name<T1>), wSchema, pfnNew, \
	RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name<T1>::GetRuntimeClass() const \
	{ return RUNTIME_CLASS_T(class_name, T1); } \

#define IMPLEMENT_RUNTIMECLASS_T2(class_name, T1, T2, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT const CRuntimeClass class_name<T1, T2>::class##class_name = { \
#class_name, sizeof(class class_name<T1, T2>), wSchema, pfnNew, \
	RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name<T1, T2>::GetRuntimeClass() const \
	{ return RUNTIME_CLASS_T2(class_name, T1, T2); } \

#define _IMPLEMENT_RUNTIMECLASS_T2(class_name, T1, T2, base_class_name, wSchema, pfnNew, class_init) \
	AFX_COMDAT CRuntimeClass class_name<T1, T2>::class##class_name = { \
#class_name, sizeof(class class_name<T1, T2>), wSchema, pfnNew, \
	RUNTIME_CLASS(base_class_name), NULL, class_init }; \
	CRuntimeClass* class_name<T1, T2>::GetRuntimeClass() const \
	{ return RUNTIME_CLASS_T2(class_name, T1, T2); } \

#endif

#define IMPLEMENT_DYNAMIC(class_name, base_class_name) \
	IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, NULL, NULL)

#define IMPLEMENT_DYNCREATE(class_name, base_class_name) \
	Object* PASCAL class_name::CreateObject() \
	{ return new class_name; } \
	IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, \
	class_name::CreateObject, NULL)


} // Ext::


namespace Ext {


template <typename T>
class optional {
public:
	optional()
		:	m_bInitialized(false)
	{}

	optional(const T& v)
		:	m_v(v)
		,	m_bInitialized(true)
	{}

	bool operator!() const { return !m_bInitialized; }

	const T& get() const {
		if (!m_bInitialized)
			Throw(E_FAIL);
		return m_v;
	}
	
	T& get() {
		if (!m_bInitialized)
			Throw(E_FAIL);
		return m_v;
	}

	void reset() {
		m_bInitialized = false;
		m_v = T();
	}

	optional& operator=(const optional& op) {
		m_bInitialized = op.m_bInitialized;
		m_v = op.m_v;
		return *this;
	}

	optional& operator=(const T& v) {
		m_v = v;
		m_bInitialized = true;
		return *this;
	}
private:
	T m_v;
	volatile bool m_bInitialized;
};



//!!!#if UCFG_FRAMEWORK && !defined(_CRTBLD)

class AFX_CLASS CTrace {
public:
	typedef unsigned long (_cdecl* PFN_DbgPrint)(const char *format, ...);
	EXT_DATA static PFN_DbgPrint s_pTrcPrint;

	EXT_DATA static bool s_bShowCategoryNames;
	EXT_DATA static int s_nLevel;
	EXT_DATA static bool s_bPrintDate;

	static void AFXAPI InitTraceLog(RCString regKey);
	static Stream* AFXAPI GetOStream();
	static void AFXAPI SetOStream(Stream *os);
	static void AFXAPI SetSecondOStream(Stream *os);
private:
	static Stream *s_pOstream, *s_pSecondStream;

	friend class CTraceWriter;
};


//!!!#endif // UCFG_FRAMEWORK

/*!!!  different implementtions in different modules
template <typename T>
class Singleton {				
public:
	T& operator()() const {
		static T s_t;
		return s_t;
	}
};
*/

template <typename R>
int FindSignature(const unsigned char *sig, size_t len, R reader) {
	for (int i=0, m=0, r=0, ch=-1;;) {
		if (m) {
			if (m==i || !memcmp(sig, sig+m, i-m)) {
				i -= std::exchange(m, 0);
				continue;
			}
		} else {
			if (ch==-1 && (ch=reader()) == -1)
				return -1;
			if (sig[i] == ch) {
				if (++i == len)
					return r;
				ch = -1;
				continue;
			} else if (!i) {
				ch = -1;
				i = 1;
			}
		}
		++r;
		++m;
	}
}

#ifndef WDM_DRIVER
	class CTls;
#endif

class EXT_API CFunTrace {
public:

#if UCFG_WDM
	CFunTrace(const char *funName, int trclevel = 0)
		:	m_trclevel(trclevel)
		,	m_funName(funName)
	{
		KdPrint((">%s\n", m_funName));
	}

	~CFunTrace() {
		KdPrint(("<%s\n", m_funName));
	}
#else
	static CTls s_level;

	CFunTrace(const char *funName, int trclevel = 0);
	~CFunTrace();
#endif

private:
	int m_trclevel;
	const char *m_funName;
};

#ifdef _MSC_VER
	inline HRESULT HResult(HRESULT err) { return err; }
#endif
inline HRESULT HResult(unsigned int err) { return (HRESULT)err; }
inline HRESULT HResult(unsigned long err) { return (HRESULT)err; }

#define EXT_CONCAT1(a, b) a##b
#define EXT_CONCAT(a, b) EXT_CONCAT1(a, b)

#if UCFG_EH_SUPPORT_IGNORE
#	define DBG_LOCAL_IGNORE(hr)					CLocalIgnore<std::error_code> EXT_CONCAT(_localIgnore, __COUNTER__)(error_code(Ext::HResult(hr), Ext::hresult_category()));
#	define DBG_LOCAL_IGNORE_CONDITION(econd)	CLocalIgnore<std::error_condition> EXT_CONCAT(_localIgnore, __COUNTER__)(make_error_condition(econd));
#	define DBG_LOCAL_IGNORE_CONDITION_OBJ(econd)	CLocalIgnore<std::error_condition> EXT_CONCAT(_localIgnore, __COUNTER__)(econd);
#	define DBG_LOCAL_IGNORE_WIN32(name)			CLocalIgnore<std::error_condition> EXT_CONCAT(_localIgnore, __COUNTER__)(error_condition(name, Ext::win32_category()));
//!!!R #	define DBG_LOCAL_IGNORE_NAME(hr, name)	CLocalIgnore<std::error_code> _localIgnore##name(error_code(Ext::HResult(hr), Ext::hresult_category()));
#else
#	define DBG_LOCAL_IGNORE(hr)
#	define DBG_LOCAL_IGNORE_CONDITION(econd)
#	define DBG_LOCAL_IGNORE_CONDITION_OBJ(econd)
#	define DBG_LOCAL_IGNORE_WIN32(name)
//!!!R #	define DBG_LOCAL_IGNORE_NAME(hr, name)
#endif

String TruncPrettyFunction(const char *fn);

} // Ext::


#ifdef _MSC_VER
#	define EXT_TRC_FUNCNAME __FUNCTION__
#else
#	define EXT_TRC_FUNCNAME Ext::TruncPrettyFunction(__PRETTY_FUNCTION__)
#endif


#if UCFG_TRC
#	define DBG_PARAM(param) param
#	define TRC(level, s) { if ((1<<level) & Ext::CTrace::s_nLevel) Ext::CTraceWriter(1<<level, EXT_TRC_FUNCNAME).Stream() << s; }

#	define TRCP(level, s) { if (level & Ext::CTrace::s_nLevel) {				\
		char obj[sizeof(Ext::CTraceWriter)];									\
		Ext::CTraceWriter& w = Ext::CTraceWriter::CreatePreObject(obj, level, EXT_TRC_FUNCNAME);				\
		w.Printf s;																\
		w.~CTraceWriter(); }}												


#	define TRC_SHORT(level, s) //!!!? ( (1<<level) & Ext::CTrace::s_nLevel ? OUTPUT_DEBUG(' ' << s) : 0)
#	define D_TRACE(cat, level, args) //!!!?( ((1<<level) & Ext::CTrace::s_nLevel) && cat.Enabled ? OUTPUT_DEBUG(cat.m_name << ": " << args << endl):0)
#	define FUN_TRACE  Ext::CFunTrace _funTrace(EXT_TRC_FUNCNAME, 0);
#	define FUN_TRACE_1  Ext::CFunTrace _funTrace(EXT_TRC_FUNCNAME, 1);
#	define FUN_TRACE_2  Ext::CFunTrace _funTrace(EXT_TRC_FUNCNAME, 2);

#	define CLASS_TRACE(name) class CClassTrace : public CFunTrace { \
	public: \
		CClassTrace() \
			:	CFunTrace(name) \
		{} \
	}  _classTrace; \

#	define TRC_PRINT(p) while (Ext::CTrace::s_pTrcPrint) { Ext::CTrace::s_pTrcPrint p; break; }

#else
#	define DBG_PARAM(param)
#	define TRC(level, s)
#	define TRCP(level, s)
#	define TRC_SHORT(level, s)
#	define D_TRACE(cat, level, args)
#	define FUN_TRACE
#	define FUN_TRACE_1
#	define FUN_TRACE_2

#	define CLASS_TRACE(name)

#	define TRC_PRINT(p)
#endif

#define FOREACH(t, n, typ, vals) \
	for (typ n##_vals=vals; n##_vals; n##_vals=typ()) \
		for (t n; n=n##_vals.NextNode();)

#define DEF_SINGLETON_ACCESSOR static class_type *II() { return static_cast<class_type*>(I); }

#define EXT_DISABLE_COPY_CONSTRUCTOR  private: class_type(const class_type&);

#define EXT_DISABLE_COPY_AND_ASSIGN  EXT_DISABLE_COPY_CONSTRUCTOR class_type& operator=(const class_type&);

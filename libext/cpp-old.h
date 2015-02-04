#pragma once

// compatibility hacks for old compilers

#if UCFG_CPP11_RVALUE
#	define EXT_REF &&
#	else
#	define EXT_REF &
#endif

#if UCFG_CPP11_ENUM

#	define ENUM_CLASS(name) enum class name
#	define ENUM_CLASS_BASED(name, base) enum class name : base
#	define END_ENUM_CLASS(name) ; 	inline name operator|(name a, name b) { return (name)((int)a|(int)b); } \
									inline name operator&(name a, name b) { return (name)((int)a&(int)b); }

#else // UCFG_CPP11_ENUM

#	define ENUM_CLASS(name) struct enum_##name { enum E
#	define ENUM_CLASS_BASED(name, base) struct enum_##name { enum E : base
#	define END_ENUM_CLASS(name) ; }; typedef enum_##name::E name; 											\
									inline name operator|(name a, name b) { return (name)((int)a|(int)b); } \
									inline name operator&(name a, name b) { return (name)((int)a&(int)b); }

#endif // UCFG_CPP11_ENUM

#if defined(_MSC_VER) && _MSC_VER < 1700
#	define EXT_FOR(decl, cont) for each (decl in cont)
#else
#	define EXT_FOR(decl, cont) for (decl : cont)
#endif

#if !UCFG_CPP11_NULLPTR

namespace std {

class nullptr_t {
public:
	template<class T> operator T*() const { return 0; }
	template<class C, class T> operator T C::*() const { return 0; }

	operator const char *() const { return NULL; }
private:
	void operator&() const;
};

} // ::std

const std::nullptr_t nullptr = {};

#endif // UCFG_CPP11_NULLPTR

#if UCFG_CPP11_NULLPTR && defined(__clang__)
namespace std {
	typedef decltype(nullptr) nullptr_t;
}
#endif

#if !UCFG_CPP11_OVERRIDE
#	define override
#endif

#if !UCFG_CPP11_CONSTEXPR
#	define constexpr
#endif

#if	UCFG_CPP11_THREAD_LOCAL
#	define THREAD_LOCAL thread_local
#else
#	define THREAD_LOCAL __declspec(thread)
#endif

#if UCFG_CPP11_EXPLICIT_CAST
#	define EXPLICIT_OPERATOR_BOOL explicit operator bool
#	define EXT_OPERATOR_BOOL explicit operator bool
#	define EXT_CONVERTIBLE_TO_TRUE true
#	define _TYPEDEF_BOOL_TYPE
#else
#	define EXPLICIT_OPERATOR_BOOL struct _Boolean { int i; }; operator int _Boolean::* 
#	define EXT_OPERATOR_BOOL operator _Bool_type
#	define EXT_CONVERTIBLE_TO_TRUE (&_Boolean::i)
#endif

namespace std {


#ifndef _CONVERTIBLE_TO_TRUE
#	define _CONVERTIBLE_TO_TRUE	(&std:: _Bool_struct::_Member)

	struct _Bool_struct {	// define member just for its address
		int _Member;
	};

	typedef int _Bool_struct::* _Bool_type;
#endif /* _CONVERTIBLE_TO_TRUE */

#if !UCFG_CPP11_BEGIN

template <class C>
typename C::iterator begin(C& c) {
	return c.begin();
}

template <class C>
typename C::const_iterator begin(const C& c) {
	return c.begin();
}

template <class C>
typename C::iterator end(C& c) {
	return c.end();
}

template <class C>
typename C::const_iterator end(const C& c) {
	return c.end();
}

template <class T, size_t size>
inline T *begin(T (&ar)[size]) {
	return ar;
}

template <class T, size_t size>
inline T *end(T (&ar)[size]) {
	return ar+size;
}

#endif // !UCFG_CPP11_BEGIN

#if !UCFG_STD_SIZE

template <class C>
inline typename C::size_type size(const C& c) {
	return c.size();
}

template <class T, size_t sz>
char(*EXT__countof_helper(UNALIGNED T(&ar)[sz]))[sz];		// constexpr emulation


template <class T, size_t sz>
constexpr inline size_t size(T (&ar)[sz]) {
	return sz;
}

#endif // !UCFG_STD_SIZE

#if !UCFG_HAVE_STATIC_ASSERT
#	ifndef NDEBUG
#		define static_assert(test, errormsg)                         \
    do {                                                        \
        struct ERROR_##errormsg {};                             \
        typedef ww::compile_time_check< (test) != 0 > tmplimpl; \
        tmplimpl aTemp = tmplimpl(ERROR_##errormsg());          \
        sizeof(aTemp);                                          \
    } while (0)
#	else
#		define static_assert(test, errormsg)                         \
    do {} while (0)
#	endif
#endif // !UCFG_HAVE_STATIC_ASSERT


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


#endif // !UCFG_STD_EXCHANGE

#if !UCFG_STD_UNCAUGHT_EXCEPTIONS
inline int __cdecl uncaught_exceptions() noexcept {
	return API_uncaught_exceptions();
}
#endif // !UCFG_STD_UNCAUGHT_EXCEPTIONS

#if !UCFG_STD_OBSERVER_PTR

template <class T>
class observer_ptr {
public:
	typedef T element_type;
	typedef T *pointer;
	typedef T &reference;

	observer_ptr() noexcept
		: m_p(0) {
	}

	observer_ptr(nullptr_t) noexcept
		: m_p(0) {
	}

	observer_ptr(const observer_ptr& p) noexcept
		: m_p(p.m_p) {
	}

	explicit observer_ptr(T *p) noexcept
		: m_p(p) {
	}

	T *get() const noexcept { return m_p; }

	EXT_OPERATOR_BOOL() const noexcept { return m_p ? _CONVERTIBLE_TO_TRUE : 0; }

	T *operator->() const noexcept { return get(); }
	T& operator*() const noexcept { return *m_p; }
	operator pointer() const noexcept { return m_p; }
	bool operator==(T *p) const noexcept { return m_p == p; }
	bool operator<(T *p) const noexcept { return m_p < p; }
	pointer release() noexcept { return exchange(m_p, nullptr); }
	void reset(pointer p) { m_p = p; }										//!!!TODO  = nullptr
private:
	T *m_p;
};

#endif // !UCFG_STD_OBSERVER_PTR



} // std::


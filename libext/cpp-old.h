/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

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

#if defined(_MSC_VER) && !UCFG_USE_OLD_MSVCRTDLL
#   include <eh.h>
#endif

#if !UCFG_STD_UNCAUGHT_EXCEPTIONS
	extern "C" int __cdecl __uncaught_exceptions();
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
constexpr inline size_t size(const T (&ar)[sz]) {
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



#if !UCFG_STD_UNCAUGHT_EXCEPTIONS || !UCFG_STDSTL
inline int __cdecl uncaught_exceptions() noexcept { return __uncaught_exceptions(); }
#endif // !UCFG_STD_UNCAUGHT_EXCEPTIONS

#if !UCFG_STD_CLAMP

template <typename T, class L>
T clamp(const T& v, const T& lo, const T& hi, L pred) {
	return pred(v, lo) ? lo : pred(hi, v) ? hi : v;
}

template <typename T>
T clamp(const T& v, const T& lo, const T& hi) {
	return clamp<T, std::less<T>>(v, lo, hi, std::less<T>());
}

#endif // UCFG_STD_CLAMP



} // std::

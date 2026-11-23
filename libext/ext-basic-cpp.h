/*######   Copyright (c) 2014-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


#if UCFG_STDSTL
#	include <system_error>
#else
#	include <el/stl/system_error>
#endif


#define EXT_DEF_HASH(T)																													\
	BEGIN_STD_TR1																														\
		template<> class hash<T>{ public: size_t operator()(const T& v) const { return EXT_HASH_VALUE_NS::hash_value(v); } };			\
		template<> class hash<const T&>{ public: size_t operator()(const T& v) const { return EXT_HASH_VALUE_NS::hash_value(v); } };	\
	END_STD_TR1

#define EXT_DEF_HASH_NS(NS, T)																						\
	} BEGIN_STD_TR1																									\
	template<> class hash<NS::T>{ public: size_t operator()(const NS::T& v) const { return v.GetHashCode(); } };	\
	END_STD_TR1 namespace NS {

namespace std {
class error_category;
class error_code;
class error_condition;
}

namespace Ext {
enum class ExtErr;

//!!!? Collides with <concepts>
template <class T> struct totally_ordered {
	friend bool operator!=(const T& x, const T& y) { return !(x == y); }
	friend bool operator>(const T& x, const T& y) { return y < x; }
	friend bool operator>=(const T& x, const T& y) { return !(x < y); }
	friend bool operator<=(const T& x, const T& y) { return !(y < x); }
};

}

namespace std {

const std::error_category& AFXAPI ext_category();

} namespace Ext {



std::error_condition make_error_condition(ExtErr v);
std::error_code make_error_code(ExtErr v);



template <typename T, class L>
bool between(const T& v, const T& lo, const T& hi, L pred) {
	return !pred(v, lo) && !pred(hi, v);
}

template <typename T>
bool between(const T& v, const T& lo, const T& hi) {
	return between<T, std::less<T>>(v, lo, hi, std::less<T>());
}

template <typename T, class L>
inline bool Between(T v, T lo, T hi, L pred) {
	return !pred(v, lo) && !pred(hi, v);
}

template <typename T>
inline bool Between(T v, T lo, T hi) {
	return Between<T, std::less<T>>(v, lo, hi, std::less<T>());
}

class noncopyable {
	noncopyable(const noncopyable&);
	noncopyable& operator=(const noncopyable&);
protected:
	noncopyable() {}
};

#if UCFG_USE_IN_EXCEPTION
class CInException : noncopyable {
	int m_nUncaught;
public:
	CInException() noexcept;
	void swap(CInException& r);
	explicit operator bool() const noexcept;
};
#endif // UCFG_USE_IN_EXCEPTION


} // namespace Ext

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

#if UCFG_CPP11_RVALUE
#	define EXT_MOVABLE_BUT_NOT_COPYABLE(TYPE)   \
		private:                                    \
			TYPE(const TYPE &);                     \
			TYPE &operator=(const TYPE &);

#	define EXT_RV_REF(typ) typ &&
#endif // UCFG_CPP11_RVALUE

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

namespace std {
#ifndef _CONVERTIBLE_TO_TRUE
#	define _CONVERTIBLE_TO_TRUE	(&std:: _Bool_struct::_Member)

	struct _Bool_struct {	// define member just for its address
	int _Member;
};

typedef int _Bool_struct::* _Bool_type;
#endif /* _CONVERTIBLE_TO_TRUE */
}

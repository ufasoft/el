/*######   Copyright (c) 2014-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include EXT_HEADER_SYSTEM_ERROR

#define EXT_DEF_HASH(T)																													\
	BEGIN_STD_TR1																														\
		template<> class hash<T>{ public: size_t operator()(const T& v) const { return EXT_HASH_VALUE_NS::hash_value(v); } };			\
		template<> class hash<const T&>{ public: size_t operator()(const T& v) const { return EXT_HASH_VALUE_NS::hash_value(v); } };	\
	END_STD_TR1

#define EXT_DEF_HASH_NS(NS, T)																						\
	} BEGIN_STD_TR1																									\
	template<> class hash<NS::T>{ public: size_t operator()(const NS::T& v) const { return v.GetHashCode(); } };	\
	END_STD_TR1 namespace NS {

namespace Ext {

//!!!? Collides with <concepts>
template <class T> struct totally_ordered {
	friend bool operator!=(const T& x, const T& y) { return !(x == y); }
	friend bool operator>(const T& x, const T& y) { return y < x; }
	friend bool operator>=(const T& x, const T& y) { return !(x < y); }
	friend bool operator<=(const T& x, const T& y) { return !(y < x); }
};

const std::error_category& AFXAPI ext_category();

inline std::error_code make_error_code(ExtErr v) {
	return std::error_code(int(v), ext_category());
}
inline std::error_condition make_error_condition(ExtErr v) {
	return std::error_condition(int(v), ext_category());
}

} // namespace Ext


namespace std {
template <> struct is_error_condition_enum<Ext::ExtErr> : true_type {};
} // namespace std

namespace Ext {

DECLSPEC_NORETURN __forceinline void ThrowImp(ExtErr errval) {
	ThrowImp(make_error_code(errval));
}
DECLSPEC_NORETURN __forceinline void ThrowImp(ExtErr errval, const char* funname, int nLine) {
	ThrowImp(make_error_code(errval), funname, nLine);
}

#if !UCFG_DEFINE_THROW
DECLSPEC_NORETURN __forceinline void AFXAPI Throw(ExtErr v) {
	ThrowImp(v);
}
#endif

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


} // namespace Ext

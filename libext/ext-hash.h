/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


#if !UCFG_STDSTL //|| UCFG_WCE //|| !defined(_XFUNCTIONAL_)

namespace std {

#if !UCFG_STD_HASH
template <typename T> struct hash {
public:
	size_t operator()(const T& v) const { return hash_value(v); }
};
#endif // UCFG_STD_HASH

template <class T> struct hash<T*> {
public:
	size_t operator()(T *v) const { return reinterpret_cast<size_t>(v); }
};

#define EXT_DEF_INTTYPE_HASH(typ) template <> struct hash<typ> { public: size_t operator()(typ v)  const { return static_cast<typ>(v); } };

EXT_DEF_INTTYPE_HASH(bool);
EXT_DEF_INTTYPE_HASH(char);
EXT_DEF_INTTYPE_HASH(signed char);
EXT_DEF_INTTYPE_HASH(unsigned char);
EXT_DEF_INTTYPE_HASH(short);
EXT_DEF_INTTYPE_HASH(unsigned short);
EXT_DEF_INTTYPE_HASH(int);
EXT_DEF_INTTYPE_HASH(unsigned int);
EXT_DEF_INTTYPE_HASH(long);
EXT_DEF_INTTYPE_HASH(unsigned long);

#	ifdef _NATIVE_WCHAR_T_DEFINED
	EXT_DEF_INTTYPE_HASH(wchar_t);
#	endif

#	if SIZE_MAX == _UI64_MAX
	EXT_DEF_INTTYPE_HASH(uint64_t);
	EXT_DEF_INTTYPE_HASH(int64_t);
#	else

template <> struct hash<uint64_t> {
public:
	size_t operator()(uint64_t v) const { return size_t(v ^ (v>>32)); }
};

template <> struct hash<int64_t> {
public:
	size_t operator()(int64_t v) const { return hash<uint64_t>()((const uint64_t&)v); }
};

#	endif


#if !UCFG_STD_HASH
template <> struct hash<double> {
public:
	size_t operator()(const double& v) const { return hash<uint64_t>()((const uint64_t&)v); }
};
#endif // UCFG_STD_HASH



//!!!template<class T> inline size_t hash_value(const T& v) { return (size_t)v ^ _HASH_SEED; }


/*!!!
template <class T> struct hash {};

template<> struct hash<uint32_t>
{
size_t operator()(uint32_t dw) const { return dw ^ _HASH_SEED; }
};

template<> struct hash<long>
{
size_t operator()(long dw) const { return dw ^ _HASH_SEED; }
};

template<> struct hash<unsigned long>
{
size_t operator()(unsigned long dw) const { return dw ^ _HASH_SEED; }
};

template<> struct hash<double>
{
size_t operator()(double d) const { return *(uint32_t*)&d ^ *((uint32_t*)&d+1); }
};

template<> struct hash<uint16_t>
{
size_t operator()(uint16_t w) const { return w ^ _HASH_SEED; }
};

*/

} // std::


#endif // UCFG_STDSTL || !defined(_XFUNCTIONAL_)




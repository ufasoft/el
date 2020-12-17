/*######   Copyright (c) 2013-2018 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/inc/ext_config.h>

#define UCFG_LIBEXT 1

#if UCFG_WDM && defined(_WIN64)
#	define memmove t_memmove
#endif

#ifdef _MSC_VER
#	include "vc/vc-warnings.h"
#elif defined(__clang__)
#	pragma clang diagnostic ignored "-Wswitch"
#	pragma clang diagnostic ignored "-Winvalid-offsetof"
#	pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#	pragma clang diagnostic ignored "-Wparentheses"
#	pragma clang diagnostic ignored "-Wswitch-enum"
#	pragma clang diagnostic ignored "-Wunused-value"
#endif

#if defined(_MSC_VER) && !UCFG_EXT_C && _FILE_OFFSET_BITS == 64 // Non-ANSI name for compatibility
typedef __int64 off_t;											// incompatible with VC
typedef long _off_t;
#	define _OFF_T_DEFINED
#endif

#if UCFG_EXT_C
#	ifndef _WIN64
#		define _INC_STAT
#	endif

#	define _INC_STDIO
#	define _INC_IO
#	define _INC_STDDEF
//#	define _INC_ERRNO

#	if !UCFG_WCE
#		define _INC_STDLIB
#	endif

#	define _INC_WTIME_INL

#	define _CSTDIO_
#	define _CSTDLIB_
#	define _FSTREAM_
//S:\src\el\inc\libext.h#		define _INC_WCHAR
//#		define _CWCHAR_
#endif

#if UCFG_WCE
#	define _INC_ERRNO
#	define _INC_PROCESS
#	define _INC_IO
#	define _INC_STAT
#	define _INC_CONIO
#endif

#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers

#define afx_msg

//#include <yvals.h>
//!!!#include <stdexcpt.h> //!!!

#if UCFG_GNUC
#	ifdef __CHAR_UNSIGNED__
#		define _CHAR_UNSIGNED
#	endif

#	define _NATIVE_WCHAR_T_DEFINED //!!!H
#endif

#ifdef _PREFAST_
#	error PREFAST not supported
#endif

#if defined(__cplusplus) && defined(WIN32)
#	ifndef _NATIVE_WCHAR_T_DEFINED
#		error libext requires /Zc:wchar_t compiler option
#	endif
#endif

#ifdef _USE_32BIT_TIME_T
#	error libext requires 64-bit time values
#endif

#ifdef _MSC_VER
#	include "libext/vc.h"
#else
#	include <sys/cdefs.h>
#endif // _MSC_VER

#if UCFG_WCE
#	include <cmnintrin.h>

#	ifdef __cplusplus
#		define tolower C_tolower
#		define toupper C_toupper
#		include <stdlib.h>
#		undef islower
#		undef isupper
#		undef isalpha
#		undef tolower
#		undef toupper
#	endif
#endif

#ifdef _MSC_VER

#	if !defined(__cplusplus) && _MSC_VER < 1900
#		define inline __inline
#	endif

#	define __packed
#	define __attribute__(x)
#	define _U_
#	define DECLSPEC(a) __declspec(a)

#	define DECLSPEC_NORETURN __declspec(noreturn) // to avoid warning

#	ifndef DECLSPEC_ALIGN
#		define DECLSPEC_ALIGN(a) DECLSPEC(align(a))
#	endif

#else // _MSC_VER

#	if UCFG_GNUC
#		define __FUNCTION__ __func__
#	endif
#	define DECLSPEC(a) __attribute__((a))
#	define __forceinline inline DECLSPEC(always_inline)

#	define DECLSPEC_NORETURN DECLSPEC(noreturn)
#	define DECLSPEC_ALIGN(a) DECLSPEC(aligned(a))

#endif // _MSC_VER

#if UCFG_GNUC && !defined(__clang__)
#	define DECLSPEC_ARTIFICIAL __attribute__((artificial)) //!!!TODO test for clang version
#else
#	define DECLSPEC_ARTIFICIAL
#endif

#ifndef DECLSPEC_NOINLINE
#	define DECLSPEC_NOINLINE DECLSPEC(noinline)
#endif

#define DECLSPEC_DLLEXPORT DECLSPEC(dllexport)
#define DECLSPEC_DLLIMPORT DECLSPEC(dllimport)

#define AFX_CLASS_EXPORT DECLSPEC_DLLEXPORT
#define AFX_CLASS_IMPORT DECLSPEC_DLLIMPORT

#if defined(__cplusplus) && UCFG_WDM
#	define iswspace C_iswspace
#	include <ctype.h>
#	undef iswspace

inline bool iswspace(wchar_t ch) {
	return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\n' || ch == '\r';
}
#endif

#define AFX_NOVTABLE
#define AFX_DATA_EXPORT
#define AFX_DATA_IMPORT DECLSPEC_DLLIMPORT
#define AFX_API_EXPORT
#define AFX_API_IMPORT DECLSPEC_DLLIMPORT

#ifdef _AFXDLL
#	ifdef _EXT
#		define AFX_RT DECLSPEC_DLLEXPORT
#		define AFX_CORE_DATA
#		define AFX_GLOBAL_DATA
#		define EXT_DATA AFX_DATA_EXPORT

#		define AFX_CLASS //!!!AFX_API_EXPORT
#		define AFX_TEMPL_CLASS
#		if UCFG_WCE
#			define AFX_API //!!! DECLSPEC_DLLEXPORT
#		else
#			define AFX_API
#		endif

//!!!  #define EXPIMP_CLASS
#		define EXPIMP_TEMPLATE AFX_API_EXPORT
#		define EXT_CLASS DECLSPEC_DLLEXPORT

#		ifdef _WIN64
#			define EXTAPI //!!!  DECLSPEC_DLLEXPORT
#			define EXT_API
#		else
#			define EXTAPI
#			define EXT_API DECLSPEC_DLLEXPORT
#		endif

#		if !UCFG_STDSTL
#			define _CRTIMP_PURE //!!!
#		endif

#	else
#		define AFX_RT DECLSPEC_DLLIMPORT
#		define AFX_CORE_DATA AFX_DATA_IMPORT //!!!
#		define AFX_GLOBAL_DATA AFX_DATA_IMPORT
#		define EXT_DATA AFX_DATA_IMPORT

#		define AFX_CLASS		//!!!AFX_API_IMPORT
#		define AFX_TEMPL_CLASS //!!!AFX_API_IMPORT
#		define AFX_API AFX_API_IMPORT

//!!!  #define EXPIMP_CLASS DECLSPEC_DLLIMPORT
#		define EXPIMP_TEMPLATE AFX_API_IMPORT
#		define EXT_CLASS DECLSPEC_DLLIMPORT

#		ifdef _WIN64
#			define EXTAPI //!!!DECLSPEC_DLLIMPORT
#			define EXT_API
#		else
#			define EXTAPI
#			define EXT_API DECLSPEC_DLLIMPORT
#		endif
#	endif

#else
#	ifdef _CRTBLD
#		define AFX_RT DECLSPEC_DLLEXPORT
#	else
#		define AFX_RT
#	endif

#	define AFX_CLASS
#	define AFX_API
#	define AFX_GLOBAL_DATA
#	define AFX_CORE_DATA
#	define AFX_DATA
#	define EXT_DATA
#	define EXT_CLASS
#	define EXT_API
#	define EXTAPI
#endif

/*!!!
	#ifndef EXT_DATA
	#  ifdef _EXT
	#    define EXT_DATA DECLSPEC_DLLEXPORT
	#  else
	#    define EXT_DATA DECLSPEC_DLLIMPORT
	#  endif
	#endif
	*/

#if UCFG_USE_OLD_MSVCRTDLL && !defined(CRTDLL)
#	define _INC_CRTDBG
#	define _INC_SWPRINTF_INL_
#endif

#if !UCFG_STDSTL
#	define std ExtSTL
#endif

//!!!T #if UCFG_USE_OLD_MSVCRTDLL
//!!!T #	include "c.h"
//!!!T #endif

/// <sys/stat.h> 64/32 macro-magic
#if UCFG_WIN32 && UCFG_STAT64 && _VC_CRT_MAJOR_VERSION < 14
#	define _INC_STAT_INL

#	if UCFG_USE_OLD_MSVCRTDLL && !defined(_WIN64)
#		define _stat64 API_stat64
#		define _wcstol_l C_wcstol_l
#	endif

#	include <wchar.h> // instead of <sys/stat.h> to define only structs

#	undef _stat
#	undef _fstat
#	undef _wstat
#	define stat _stat64
#	define _stat _stat64
#	define fstat _fstat64
#	define _fstat _fstat64

#	if UCFG_USE_OLD_MSVCRTDLL && UCFG_PLATFORM_IX86
//!!!R#		define fstat C_fstat
#		define _fstat64i32 C__fstat64i32
#		define _wstat64i32 C__wstat64i32
#		define _strtoi64 C_strtoi64
#		define _strtoui64 C_strtoui64

#		ifndef _WIN64
#			pragma push_macro("_stat64")
#			undef _stat64
#			define _stat64 C_stat64

#			undef _wcstol_l
__BEGIN_DECLS
EXT_API long _cdecl API__wcstol_l(const wchar_t *nptr, wchar_t **endptr, int base, _locale_t loc);
__END_DECLS
#			define _wcstol_l API__wcstol_l

#		endif
#	endif

#	if !UCFG_WCE
#		include <sys/stat.h>
#	endif

#	if UCFG_USE_OLD_MSVCRTDLL
#		undef _fstat64i32
#		undef _wstat64i32

#		ifdef _WIN64
#			define _wstat _wstat64

#			undef _fstat
#			define _fstat _fstat64

#		else
#			pragma pop_macro("_stat64")

__BEGIN_DECLS
EXT_API int __cdecl API_stat64(const char *path, struct _stat64 *buf);
EXT_API int __cdecl API_wstat64(const wchar_t *path, struct _stat64 *buf);
EXT_API int __cdecl API_fstat64(int fildes, struct _stat64 *buf);
__END_DECLS
#			define _wstat API_wstat64
#			define _fstat64 API_fstat64

#		endif
#	endif

#	define stat _stat64
#	define _stat _stat64
#	define fstat _fstat64
#	define _fstat _fstat64

#endif // UCFG_WIN32 && UCFG_STAT64

#if !defined(_MSC_VER) || (_MSC_VER >= 1600)
#	if !UCFG_STDSTL && defined(_MSC_VER) && (_MSC_VER >= 1700)
#		pragma push_macro("_YVALS")
#		define _YVALS
//!!!?#		define _YVALS_CORE_H
#	endif

#	if !UCFG_STDSTL && !UCFG_EXT_C
#		define std ExtSTL
#		include <stddef.h>
#	endif

#	ifdef _WIN64
#		define _INTPTR 2
#	elif defined(_WIN32)
#		define _INTPTR 1
#	endif

#	include <stdint.h> // should be after ext-cpp.h because <yvals.h> ???
#	define HAVE_STDINT_H 1

#	if !UCFG_STDSTL && defined(_MSC_VER) && (_MSC_VER >= 1700)
#		pragma pop_macro("_YVALS")
#	endif
#else
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

#if !defined(_HAS_CHAR16_T_LANGUAGE_SUPPORT) || !_HAS_CHAR16_T_LANGUAGE_SUPPORT
typedef unsigned short char16_t;
#endif

#if !defined(WIN32) && !defined(_MINWINDEF_)
typedef uint32_t DWORD; // required for MC-generated .h files
#	define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#endif

#if !UCFG_CPP17
	typedef uint8_t byte;
#endif

typedef unsigned int u_int; //!!!
typedef unsigned long u_long;

#ifdef __cplusplus
#	if !UCFG_STDSTL
#		define nan C_nan
#	endif

namespace Ext {
#endif

#define INFINITE 0xFFFFFFFF // Infinite timeout

#if UCFG_GNUC

#	define __success(a)

#	if defined(__SIZEOF_SHORT__) && (__SIZEOF_SHORT__ == 2) || (__SHRT_MAX__ == 32767)
//!!!R		typedef short int16_t;
//!!!R		typedef unsigned short uint16_t;
#	endif

#	if defined(__SIZEOF_LONG_LONG__) && (__SIZEOF_LONG_LONG__ == 8) || (__LONG_LONG_MAX__ == 9223372036854775807LL)
#	endif

#	if defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 8) || (__LONG_MAX__ == 9223372036854775807LL)
#		define UCFG_SEPARATE_INT_TYPE 0
#		define UCFG_SEPARATE_LONG_TYPE 0
#	elif defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 4) || (__LONG_MAX__ == 2147483647)
#		define UCFG_SEPARATE_INT_TYPE 0
#		define UCFG_SEPARATE_LONG_TYPE 1
//!!!R		typedef unsigned long uint32_t;
#	elif defined(__SIZEOF_INT__) && (__SIZEOF_INT__ == 4) || (__INT_MAX__ == 2147483647)
#		define UCFG_SEPARATE_INT_TYPE 0
#		define UCFG_SEPARATE_LONG_TYPE 1
//!!!R		typedef int int32_t;
//!!!R		typedef unsigned int uint32_t;
#	endif

typedef uint8_t UCHAR;
typedef int SOCKET;
//#	define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#	define EXT_INVALID_HANDLE_VALUE (-1)

#	define LANG_USER_DEFAULT 0

typedef unsigned char uchar;

#else
#	define UCFG_SEPARATE_INT_TYPE 0
#	define UCFG_SEPARATE_LONG_TYPE 1
//!!!R		typedef __int16 int16_t;
//!!!R		typedef unsigned __int16 uint16_t;
//!!!R		typedef long int32_t;
//!!!R		typedef unsigned long uint32_t;
//!!!R		typedef __int64 int64_t;
//!!!R		typedef unsigned __int64 uint64_t;

#	ifndef PASCAL
#		define PASCAL __stdcall
#	endif

#endif // __GNUC__

typedef uint8_t guint8;
typedef uint16_t guint16;
typedef uint32_t guint32;

#ifdef __cplusplus

#	if __SIZEOF_WCHAR_T__ > 2 || !defined(_NATIVE_WCHAR_T_DEFINED)
typedef uint16_t Char16;
#	else
typedef wchar_t Char16;
#	endif
}
#	define OPTIONAL_EXT Ext::
#else
#	define OPTIONAL_EXT
#endif

#if defined(_MSC_VER) && !UCFG_WCE
#	include <vadefs.h>
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1400) && !UCFG_MINISTL
#	include <crtdefs.h>
#endif

#if UCFG_EXT_C && !UCFG_WCE
#	include <el/c-runtime/c-runtime.h>
#endif

#if !UCFG_WIN32 && UCFG_USE_POSIX
#	include <sys/stat.h>
#endif

#ifdef WIN32
#	ifndef _MFC_VER			//!!!
#		define _MFC_VER 0x0900 //!!! 0x0710
#	endif

#	ifdef _MSC_VER
#		if !UCFG_STDSTL
#			undef __GOT_SECURE_LIB__
#		endif

#		if UCFG_USE_OLD_MSVCRTDLL && UCFG_PLATFORM_IX86
#			define __iob_func redef__iob_func
#		endif
#	endif

#	if !UCFG_MINISTL && !defined(_CRTBLD) || (UCFG_CRT == 'U')
#		include <stdio.h>
#		include <wchar.h>
#	endif

#	ifdef __cplusplus
extern "C" {
#	endif

#	if !UCFG_STDSTL
//	extern FILE _iob[];
//	FILE * __cdecl __iob_func(void);
#	endif

#	ifdef __cplusplus
} /* end of extern "C" */
#	endif

#	if (defined(_WIN64) || UCFG_STDSTL) && !UCFG_WCE
#		include <wchar.h>
#		include <sys/stat.h>
#	endif

#	if !defined(__STDC__) && !UCFG_MINISTL
//!!!		#define __STDC__ 1

//		#define fstat _fstat
#		if !UCFG_WCE
#			include <sys/stat.h>
#		endif

#		include <sys/types.h>

#		undef __STDC__
#	endif

#	if UCFG_LIB_DECLS
#		if defined(USE_ONLY_LIBEXT) && !defined(_EXT)
#			pragma comment(lib, "libext")
#		elif defined(_EXT)
#			if UCFG_EXTENDED && UCFG_CRT != 'U'
#				pragma comment(lib, "elrt")
#			endif
#		else
#			ifdef _AFXDLL
#				pragma comment(lib, "libext")
#				pragma comment(lib, "ext.lib") // we need these #pragmas before comment(lib) of C++ libs
#				if UCFG_WCE
#					pragma comment(lib, "libext") // can't merge it with ext.lib because /QMFPE warning
#				endif
#			else
#				ifndef _CRTBLD // to prevent elst.lib
//!!!R					#if UCFG_EXTENDED
#					if !UCFG_COMPLEX_WINAPP
#						error "elst requires UCFG_COMPLEX_WINAPP"
#					endif

//#						pragma comment(lib, "elst")

//!!!?#						pragma comment(lib, "elrt")
//!!!					#endif
#				endif
#			endif
#		endif
#	endif

//!!!	#ifndef _AFXDLL
//!!!		#define UCFG_GUI 0
//!!!	#endif

#	define C_LRU_USE_HASH // enable by default

#	if defined(_EXT) && UCFG_USE_OLD_MSVCRTDLL
//!!!		#pragma comment(lib, "/src/foreign/lib/vs8_eh.lib")
#	endif

#	if !UCFG_STDSTL
#		pragma comment(linker, "/NODEFAULTLIB:oldnames.lib")
#	endif

//#	if UCFG_WIN32_FULL //!!!R?
#	if !UCFG_WCE
#		include <sys/types.h>
#	endif
//#	endif

typedef unsigned short sa_family_t;

#	define HAVE_WIN32API 1

#	define fileno _fileno

#	if !UCFG_WCE
#		define fdopen(fd, type) _fdopen(fd,type) // Keep this definition to be identical whe redefined
#	endif

#	ifndef __cplusplus
#		define read _read
#		define write _write

#		ifndef _CRTDBG_MAP_ALLOC
#			define strdup _strdup
#		endif

#		define stricmp _stricmp
#		define strnicmp _strnicmp
#		define mkdir _mkdir
#		define unlink _unlink
#		define snprintf _snprintf
#		define umask _umask

#		define setmode _setmode
#		define dup _dup
#		define dup2 _dup2
#		define lseek _lseek
#		define write _write
#		define spawnvp _spawnvp
//!!!		#define stat _stat
//!!!		#define fstat _fstat
#	endif

#	include <memory.h> //!!! repeat

#	include <memory.h> //!!! repeat
#	include <wchar.h>
#	include <string.h>

//!!!#include <winsock2.h>

#	if UCFG_INTRINSIC_MEMFUN

#		pragma intrinsic(memset)
#		pragma intrinsic(memcpy)
//	#pragma intrinsic(memcmp)
#	endif

#	define SIGHUP 456 //!!!
#	define SIGALRM 14 /* alarm clock */
#	define SIGUSR1 30 /* user defined signal 1 */
#	define SIGUSR2 31 /* user defined signal 2 */

#endif // WIN32

#if defined(_MSC_VER) && UCFG_DEFINE_OLD_NAMES
#	define getpid() _getpid()
#	define open _open
//!!!R #	define close _close
#endif

__BEGIN_DECLS
typedef struct APIT_lldiv_t {
	long long quot;
	long long rem;
} APIT_lldiv_t;

APIT_lldiv_t __cdecl API_lldiv(long long numer, long long denom);
unsigned long long __cdecl API_strtoull(const char *str, char **endptr, int base);
__END_DECLS

#if UCFG_WIN32_FULL
#	if _VC_CRT_MAJOR_VERSION >= 14

#	elif UCFG_CRT == 'U' || UCFG_CRT == 'O' || defined(_CPPLIB_VER) && _CPPLIB_VER < 610
#		if 1 //!!!R UCFG_PLATFORM_IX86
#			define lldiv_t C_lldiv_t
#			define lldiv C_lldiv
#			define div C_div
#		endif
#		define strtoull C_strtoull
#		include <stdlib.h>
#		if 1 //!!!R UCFG_PLATFORM_IX86
#			undef lldiv
#			define lldiv API_lldiv
#			undef lldiv_t
#			define lldiv_t APIT_lldiv_t
#			undef div
#		endif

#		undef strtoull
#		define strtoull API_strtoull

#		if defined(__cplusplus) //!!!R && UCFG_PLATFORM_IX86
extern "C++" {
inline ldiv_t div(long _A1, long _A2) {
	return ldiv(_A1, _A2);
}
inline lldiv_t div(long long _A1, long long _A2) {
	return lldiv(_A1, _A2);
}
}
#		endif					 // __cplusplus
#	else
#		include <stdlib.h>
#	endif
#endif

#if UCFG_USE_WINDEFS
#	ifdef _M_X64
#		define DISPATCHER_CONTEXT X_DISPATCHER_CONTEXT
#	endif
#	pragma warning(push)
#	pragma warning(disable : 4668) // SYMBOL is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
//#		include <windows.h>
#	undef small //!!! defined in <rpcndr.h>
#	pragma warning(pop)
#	if !UCFG_WIN32
#		undef WIN32
#	endif
#	ifdef _M_X64
#		undef DISPATCHER_CONTEXT
#	endif
#endif

#define EXTCLASS EXTAPI

#ifdef _WIN32
#	define AFXAPI __stdcall
#else
#	define AFXAPI
#endif

#ifdef WDM_DRIVER
#	pragma warning(disable : 4668)
#	pragma warning(push, 3)
#	define NTVERSION 'WDM'

__BEGIN_DECLS
#	include <ntifs.h>
#	include <ntddk.h>
__END_DECLS

typedef ULONG POOLTAG;
extern POOLTAG DefaultPoolTag;

#	if defined(__cplusplus) && UCFG_USE_DRIVER_WORKS

//!!!#define _SLIST_HEADER_
//!!!typedef void *PSLIST_ENTRY;
//!!!typedef void *PSLIST_HEADER;
//!!!typedef void *SLIST_HEADER;

#		define _DW_INCLUDE_NTDDK_

#		define __CPPRT__
#		include <el/eldrv/stcinit.h>

inline void *__cdecl operator new(size_t nSize, POOL_TYPE iType) {
#		if DBG
	return nSize ? ExAllocatePoolWithTag(iType, nSize, DefaultPoolTag) : NULL;
#		else
	return nSize ? ExAllocatePool(iType, nSize) : NULL;
#		endif
}

inline void __cdecl operator delete(void *p, POOL_TYPE iType) {
	ExFreePool(p);
}

#		define STATIC_ASSERT
#		include <vdw.h>
#	else

#		if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_IX86)
#			define _X86_
#		endif

#		if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_AMD64)
#			define _AMD64_
#		endif

#		if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_M68K)
#			define _68K_
#		endif

#		if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_MPPC)
#			define _MPPC_
#		endif

#		if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_M_IX86) && !defined(_AMD64_) && defined(_M_IA64)
#			if !defined(_IA64_)
#				define _IA64_
#			endif // !_IA64_
#		endif

#		include <excpt.h>
#		include <ntdef.h>
#		include <wdm.h>
#	endif

#	pragma warning(pop)
#endif // WDM_DRIVER

#if UCFG_WDM
#	define _ASSERTE(a)
#endif

#ifdef WDM_DRIVER
typedef WCHAR *BSTR;

#endif

#ifndef _STL_NAMESPACE
#	define _STL_NAMESPACE ExtSTL
#endif

#if defined(_MSC_VER) && (UCFG_WDM || UCFG_WCE || !UCFG_STDSTL)
#	include "libext/vc/default-libs.h"
#endif

#define EXT_FASTCALL //!!!__fastcall

#ifdef _WIN32
#	if defined(WIN32) && !UCFG_WCE
//!!!#		include <windows.h>
#		ifndef NTDDI_XP
#			define NTDDI_XP NTDDI_WINXP
#		endif
//!!!? #		include <corerror.h>
#	endif

#	include <winerror.h>

#else

#	include <sys/socket.h>
#	include <sys/ioctl.h>
#	include "libext/ext-posix-win.h"

#endif // _WIN32

#ifndef ERROR_ASSERTION_FAILURE
#	define ERROR_ASSERTION_FAILURE 668L
#endif

#include "libext/ext-macro.h"

#include <stddef.h>

#if !UCFG_WCE
#	include <fcntl.h>
#endif

#ifdef __cplusplus

#	if UCFG_SIMPLE_MACROS_SELF
#		define _self (*this)
#	endif

#	if defined(_MSC_VER) && !defined(__PLACEMENT_NEW_INLINE) //!!!
#		define __PLACEMENT_NEW_INLINE
inline void *operator new(size_t, void *_P) {
	return (_P);
}
inline void operator delete(void *, void *) {
}
#	endif

#	ifdef _LIBCPP_VERSION
#		include <system_error>
#	else
namespace std {
class error_code;
}
#	endif

namespace Ext {

/*!!!AFX_API*/ DECLSPEC_NORETURN EXTAPI void AFXAPI ThrowImp(HRESULT hr);
DECLSPEC_NORETURN EXTAPI void AFXAPI ThrowImp(const std::error_code &ec);
DECLSPEC_NORETURN EXTAPI void AFXAPI ThrowImp(HRESULT hr, const char *funname, int nLine);
DECLSPEC_NORETURN EXTAPI void AFXAPI ThrowImp(const std::error_code &ec, const char *funname, int nLine);

#	ifndef UCFG_DEFINE_THROW
#		define UCFG_DEFINE_THROW UCFG_DEBUG
#	endif

#	if UCFG_DEFINE_THROW
#		define Throw(arg)                                                                                                                                                                                                                                    \
			do {                                                                                                                                                                                                                                              \
				Ext::ThrowImp(arg, __FUNCTION__, __LINE__);                                                                                                                                                                                                   \
			} while (false)
#	else
DECLSPEC_NORETURN __forceinline void Throw(HRESULT hr) {
	ThrowImp(hr);
}
DECLSPEC_NORETURN __forceinline void Throw(const std::error_code &ec) {
	ThrowImp(ec);
}
#	endif

#	define THROW_UNK() Throw(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_LINE_NUMBER, __LINE__))

} // namespace Ext
#endif

#if !defined(_SIZE_T_DEFINED) && !defined(_M_X64)
typedef unsigned int size_t;
#endif

#include <string.h>

#define NO_ANSI_KEYWORDS

#ifdef _WIN32
#	define NBBY 8
#	define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))
#	define howmany(x, y) (((x) + ((y)-1)) / (y))
#	ifndef _KERNEL
#		define _KERNEL 1
#	endif
#	define __P(x) x
#endif

#define MSIZE 256
#ifndef MCLSHIFT
#	define MCLSHIFT 11 /* convert bytes to m_buf clusters */
/* 2K cluster can hold Ether frame */
#endif							 /* MCLSHIFT */
#define MCLBYTES (1 << MCLSHIFT) /* size of a m_buf cluster */
#define MCLOFSET (MCLBYTES - 1)
#define NMBCLUSTERS 4096 /* map size, max cluster allocation */

#include "libext/c-old.h"

#if UCFG_WCE
#	define _wassert API_wassert
#endif

#if UCFG_WCE
#	include <assert.h>

/*!!!R
	#ifndef NDEBUG //!!! fixing bug in SDK (<=5.0)
		#undef ASSERT_PRINT
		#define ASSERT_PRINT(exp, file, line) OutputDebugString(TEXT("\r\n*** ASSERTION FAILED in ") TEXT(file) TEXT("(") TEXT(#line) TEXT("):\r\n") TEXT(#exp) TEXT("\r\n"))
	#endif
	*/
#endif

#define ASSERT_CDECL                                                                                                                                                                                                                                          \
	static void __cdecl _CdeclFun(int a){};                                                                                                                                                                                                                   \
	static void (*_pfnCDecl)(int a) = &_CdeclFun;
#define ASSERT_STDCALL                                                                                                                                                                                                                                        \
	static void __stdcall _StdcallFun(int a);                                                                                                                                                                                                                 \
	static void (*_pfnStdcall)(int a) = &_StdcallFun;

#include <float.h>
#include <limits.h>

__BEGIN_DECLS
void *__cdecl API_memmove(void *dst, const void *src, size_t size);

long __cdecl API_lround(double d);
int __cdecl API_fdclass(float d);
int __cdecl API_dclass(double d);
int __cdecl API_ldclass(long double d);
__END_DECLS

#if !UCFG_STDSTL
#	define lround C_lround
#endif

#ifndef __cplusplus
#	include <math.h>
#endif

#if !UCFG_STDSTL && UCFG_CRT != 'U'
#	ifndef _CLASSIFY
#		define _CLASS_ARG(_Val) (sizeof((_Val) + (float)0) == sizeof(float) ? 'f' : sizeof((_Val) + (double)0) == sizeof(double) ? 'd' : 'l')
#		define _CLASSIFY(_Val, _FFunc, _DFunc, _LDFunc) (_CLASS_ARG(_Val) == 'f' ? _FFunc((float)(_Val)) : _CLASS_ARG(_Val) == 'd' ? _DFunc((double)(_Val)) : _LDFunc((long double)(_Val)))
#	endif
#	undef fpclassify
#	define fpclassify(_Val) (_CLASSIFY(_Val, API_fdclass, API_dclass, API_ldclass))

#	undef lround
#	define lround API_lround
#endif

#if UCFG_WCE && UCFG_STDSTL
#	include <new.h> // because <new> includes "new.h"
#endif

#include <setjmp.h>
#include <stdarg.h>

#include "libext/facilities.h"

#if UCFG_WCE
extern const fpos_t _Fpz;
#endif

#ifdef WIN32
#	if UCFG_WIN_HEADERS
#		include <winsock2.h>
#		include <ws2tcpip.h>
#       undef gai_strerror
//#		if UCFG_WIN32_FULL
//!!!R#		include <ws2ipdef.h>
//#		endif
#	endif

__BEGIN_DECLS
typedef int socklen_t;

const char *__cdecl API_inet_ntop(int af, const void *src, char *dst, socklen_t size);
#	define inet_ntop API_inet_ntop

int __cdecl API_inet_pton(int af, const char *src, void *dst);
#	define inet_pton API_inet_pton
__END_DECLS
#	define HAVE_INET_NTOP
#	define HAVE_INET_PTON
#endif

#if UCFG_USE_POSIX
#	include <stdlib.h>
#endif

#if UCFG_USE_OLD_MSVCRTDLL && !defined(_CRTBLD) && UCFG_PLATFORM_IX86

#	define _wassert C_wassert

#	include <stdarg.h>
#	include <malloc.h>

#	ifndef _EXT
#		include <assert.h>
#	else
#		include <assert.h> //!!!
#	endif

#	define time C_time
#	define mktime C_mktime
#	define localtime C_localtime
#	define gmtime C_gmtime
#	define ctime C_ctime
#	define utime C_utime
#	define _utime C__utime
#	define ftime C_ftime
#	define difftime C_difftime
#	define _gmtime64_s C_gmtime64_s
//!!!#	define _timeb C_timeb
//!!!#	define _ftime C__ftime

#	define _vscpriinf C__vscpriinf

#	include <time.h>
#	include <sys/utime.h>
#	include <sys/timeb.h>
#	include <locale.h>
#	undef time
#	undef mktime
#	undef localtime
#	undef gmtime
#	undef ctime
#	undef utime
#	undef _utime
#	undef ftime
#	undef _ftime
#	undef _timeb
#	undef difftime
#	undef _gmtime64_s

#	undef _vscpriinf

//#include <sys/stat.h>

#	include <el/inc/crt/crt.h>

#else
#	include <assert.h>

#	ifdef _WIN64
#		define getcwd _getcwd
#	endif
#endif

#ifdef _WIN32
#	define strtoll _strtoi64
#	define HAVE_STRTOLL
#endif

#if UCFG_CRT == 'U'
#	define HAVE_STRUCT_TIMESPEC 1
#endif

#define strlwr _strlwr

#if UCFG_USE_OLD_MSVCRTDLL && !defined(_CRTBLD) || UCFG_WCE
__BEGIN_DECLS

AFX_RT void *__cdecl my_aligned_malloc(size_t size, size_t align);
#	define _aligned_malloc my_aligned_malloc

AFX_RT void __cdecl my_aligned_free(void *);
#	define _aligned_free my_aligned_free

__END_DECLS
#endif

#if UCFG_CRT == 'U'
#	define _TIMESPEC_DEFINED
#endif

#ifdef WIN32

#	include <el/libext/win32/win-defines.h>

#	if UCFG_WCE
#		include <connmgr.h>
#	else
//#		include <conio.h>
//!!!R#		include <stdlib.h>

#		if defined(_MSC_VER) && !UCFG_MINISTL
#			include <crtdbg.h>
#		endif

#		ifndef _WIN64

#			define rmdir _rmdir

#			ifndef _CRTDBG_MAP_ALLOC
#				define getcwd C_getcwd
//#define __STDC__
#				include <direct.h>
#				undef getcwd

__BEGIN_DECLS

#				if defined(_MSC_VER) && UCFG_STDSTL
inline char *_cdecl API_getcwd(char *buf, int n) {
	return _getcwd(buf, n);
}
#				else
char *_cdecl API_getcwd(char *buf, int n);
#				endif

__END_DECLS
#				define getcwd API_getcwd
#			endif
#		endif

#		include <fcntl.h>
#		include <share.h>
//#		include <mswsock.h>
//#		include <regstr.h>
//!!!R#		include <winsvc.h>

#		if UCFG_WIN_HEADERS
//#			include <vfw.h>
//#			include <multimon.h> // before WinUser.h
#		endif

//#		include <wincon.h> // PortSDK

#		define isatty C_isatty
#		undef getpid
#		define getpid C_getpid
#		define close C_close
#		include <io.h>
#		include <process.h>
#		undef isatty
#		undef getpid
#		define getpid() _getpid()
#		undef close
#		define close API_close

#		ifndef NOGDI
/*!!!R
#			pragma warning(push)
#				pragma warning(disable: 4668) // SYMBOL is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#				include <setupapi.h>
#			pragma warning(pop)
*/
//#			include <RichEdit.h>
#		endif

#		ifdef _MSC_VER
//#			include <devguid.h>
#		endif
#	endif

//!!!#	include <windows.h>
//!!!R #	include <ocidl.h>
//!!!#	include <wininet.h>

#	if UCFG_WIN_HEADERS
//!!!#		include <commdlg.h>
//#		include <commctrl.h>
//#		include <shlwapi.h>
#	endif
#	include <tchar.h>

//#	include <psapi.h>

#	include <basetyps.h>
//!!!#	include <comcat.h>
//!!!R#	include <htmlhelp.h>
#	if UCFG_WIN_HEADERS
//#		include <msxml2.h>
#	endif

#	if defined(__cplusplus) && UCFG_USE_ATL
#		include <atldef.h>
#	endif

#else

#	ifdef _MSC_VER //!!!

__BEGIN_DECLS

#		define RAND_MAX 0x7fff

#		if !UCFG_WDM
typedef struct _div_t {
	int quot;
	int rem;
} div_t;

typedef struct _ldiv_t { // from stdlib.h
	long quot;
	long rem;
} ldiv_t;

typedef struct _lldiv_t {
	long long quot;
	long long rem;
} lldiv_t;

div_t __cdecl div(_In_ int _Numerator, _In_ int _Denominator);
ldiv_t __cdecl ldiv(long _Numerator, long _Denominator);
lldiv_t __cdecl lldiv(_In_ long long _Numerator, _In_ long long _Denominator);
#		endif
int __cdecl rand(void);

__END_DECLS

#	endif

#endif // WIN32

#ifdef _WIN64
#	undef _findfirst
#	undef _findnext
#	undef _finddata_t

#	define _findfirst _findfirst64
#	define _findnext _findnext64
#	define _finddata_t __finddata64_t
#endif

#if !UCFG_WCE
#	include <errno.h>
#	include <signal.h>
#endif

__BEGIN_DECLS

#if defined(_MSC_VER) && !UCFG_WDM
inline int __cdecl API_close(int fh) {
	return _close(fh);
}
#endif

void(__cdecl *__cdecl API_signal(int _SigNum, void(__cdecl *_Func)(int)))(int);
__END_DECLS

#if UCFG_USE_OLD_MSVCRTDLL && UCFG_PLATFORM_IX86
#	define signal API_signal
#endif

#if defined(_WIN32) && !defined(_CORECRT_BUILD)
#	include <sdkddkver.h>
//#	include "libext/ext-os-api.h"
#	include "libext/ext-posix.h"
#	if !UCFG_WIN32
#		define _tcscpy wcscpy
#		define _tcslen wcslen
#	endif
#endif

#if UCFG_WDM
#	define lseek C_lseek
#	define fseek C_fseek
//#	define _vsnprintf C_vsnprintf
#endif

#if !defined(_CRTBLD)
#	if !UCFG_WDM
#		include <stdio.h>
#	endif
#	include <wchar.h>
#	include <time.h>

#	if defined(_MSC_VER) && defined(WIN32)

//!!!R#include <yvals.h>

//!!!#include <ws2tcpip.h>
//!!! #include <wspiapi.h> // to keep compatiblity with pre-XP windows

#		if UCFG_WIN_HEADERS
#			pragma warning(push)
#			pragma warning(disable : 4668) // SYMBOL is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
//#				include <shlobj.h>
#			pragma warning(pop)
#		endif
#	endif
#endif

#if UCFG_WDM
#	undef lseek
#	undef fseek
//#	undef _vsnprintf
#endif

#ifdef _EXT
#	define IMPEXP_API DECLSPEC_DLLEXPORT
#else
#	define IMPEXP_API DECLSPEC_DLLIMPORT
#endif

__BEGIN_DECLS

int AFXAPI RegisterAtExit(void(_cdecl *pfn)());
void AFXAPI UnregisterAtExit(void(_cdecl *pfn)());
void _cdecl MainOnExit();

typedef void *(_cdecl *PFN_memcpy)(void *dest, const void *src, size_t count);
extern EXT_DATA PFN_memcpy g_fastMemcpy;

EXT_API int *__cdecl API_sys_nerr();

#if UCFG_USE_OLD_MSVCRTDLL
#	undef _sys_nerr
#	define _sys_nerr (*API_sys_nerr())

#	define _getpid() ((int)GetCurrentProcessId())
#	define getpid() _getpid()
#	define sleep(n) Sleep((n)*1000)
#	define _sleep(n) Sleep(n)
#	define HAVE_DECL_SLEEP 1
#endif

#ifdef _MSC_VER
#	define UCFG_HAS_LONGJMP_UNWIND 1
#	if UCFG_WCE
#		define longjmp_unwind API_longjmp_unwind
#	else
#		define setjmp_unwind setjmp
#		if UCFG_EXTENDED
#			define longjmp_unwind API_longjmp
#		else
#			define longjmp_unwind longjmp
#		endif
#	endif
#else
#	define UCFG_HAS_LONGJMP_UNWIND 0
#endif

#if !defined(_WIN64) && UCFG_EXTENDED && UCFG_STDSTL
#	define longjmp My_longjmp
IMPEXP_API DECLSPEC_NORETURN void _cdecl My_longjmp(jmp_buf buf, int val);
#endif

const unsigned short *AFXAPI Utf8ToUtf16String(const char *utf8); //!!! changed from char16_t due .C compilation problems

#if UCFG_USE_MASM
void *__cdecl MemcpyAligned32(void *d, const void *s, size_t size);
#else
#	define MemcpyAligned32 memcpy
#endif // UCFG_USE_MASM

#if UCFG_WDM
long __cdecl strtol(const char *str, char **endptr, int base);
long long __cdecl _strtoi64(const char *str, char **endptr, int base);
#endif

#ifdef _MSC_VER
#	if UCFG_STDSTL && !UCFG_MINISTL
inline double _cdecl exp10(double v) {
	return exp(v * 2.30258509299404568402);
} // * M_LN10
#	else
double _cdecl exp10(double v);
#	endif
#endif

__END_DECLS

#ifdef __cplusplus

#	if UCFG_WCE
struct CSetjmpHelper {
	~CSetjmpHelper();
	void Call();
};

#		define EXT_USES_SETJMP_UNWIND CSetjmpHelper _declare_EXT_USES_SETJMP_UNWIND;

int __cdecl API_setjmp_unwind(jmp_buf env);
#		define setjmp_unwind(env) (&_declare_EXT_USES_SETJMP_UNWIND, API_setjmp_unwind(env)) // Ensure calling function generates Frame Pointer
#	else
#		define EXT_USES_SETJMP_UNWIND

#	endif

DECLSPEC_NORETURN void __cdecl API_longjmp_unwind(jmp_buf env, int value, void *historyTable = 0); // it should not be "C" because incompatible with /EHsc
#endif																							   // __cplusplus

#if UCFG_WDM && defined(_WIN64)
#	undef memmove
__BEGIN_DECLS
void *__cdecl memmove(void *dst, const void *src, size_t size);
__END_DECLS
#endif

#ifdef _MSC_VER
#	define EXT_LL_PREFIX "I64" // VC6 CRT supports only this format
#else
#	define EXT_LL_PREFIX "ll"
#endif

#if UCFG_WCE
#	include "libext/ce/ce-crt.h"
#endif

#if UCFG_GNUC
#	include "libext/gcc-intrinsic.h"
#endif

#if UCFG_WIN_HEADERS
//#	include <ntdef.h>
#	include <wchar.h>
#	include <winnt.h>
#endif

#if defined(__cplusplus) && defined(_CRTBLD)
#	include <intrin.h>
#endif

#ifdef __cplusplus
#	include "libext/ext-cpp.h"
#endif

#if UCFG_CPU_X86_X64

#	ifdef _MSC_VER
#		pragma warning(push)
#		if _MSC_VER == 1700
#			pragma warning(disable : 4668) // __cplusplus' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#		endif

#		if UCFG_WDM
#			define _INC_MALLOC // to prevent malloc()
#		endif

#		include <intrin.h>
#		pragma warning(pop)
#	endif

__BEGIN_DECLS

static __inline void Cpuid(int a[4], int level) {
#	ifdef _MSC_VER
	__cpuid(a, level);
#	else
	__cpuid(level, a[0], a[1], a[2], a[3]);
#	endif
}

#	ifdef _MSC_VER
u_int64_t __rdtsc();
#		pragma intrinsic(__rdtsc)
#	endif

EXT_DATA extern uint8_t g_bHasSse2;
__END_DECLS
#endif // UCFG_CPU_X86_X64

#ifdef __cplusplus
#	include "libext/ext-cpu.h"
#endif

#if defined(__cplusplus) && defined(WIN32) && defined(_MSC_VER) && !UCFG_MINISTL

/*!!!
	#if !defined(_EXT) && defined(_AFXDLL)
		#pragma comment(linker, "/NODEFAULTLIB:comsuppwd.lib")
		#pragma comment(linker, "/NODEFAULTLIB:comsuppw.lib")
		#pragma comment(linker, "/NODEFAULTLIB:comsuppd.lib")
		#pragma comment(linker, "/NODEFAULTLIB:comsupp.lib")
	#else
		#ifdef _DEBUG
			#pragma comment(lib, "comsuppwd.lib")
		#else
			#pragma comment(lib, "comsuppw.lib")
		#endif
	#endif       */
#endif

#if !UCFG_MINISTL //&& defined(_MSC_VER) && UCFG_EXTENDED
#	include "el/inc/extver.h"
#endif

#ifdef WIN32
#	include <malloc.h>
#	undef _mm_free //!!! to disable SSE2_INTRINSICS_AVAILABLE in Crypto++
#endif

#ifndef TLS_OUT_OF_INDEXES
#	define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#endif

#ifdef __cplusplus

#	ifdef _MSC_VER
extern "C" IMPEXP_API void __stdcall My_CxxThrowException(void *pExceptionObject, _ThrowInfo *_pThrowInfo);
#	endif

#endif // __cpluplus

#if UCFG_WCE
#	define free my_free

//!!!	extern "C" __declspec(selectany) int _charmax = CHAR_MAX; //!!!
#endif

#if UCFG_REDEFINE_MAIN // for MainOnExit implementation
#	define main _my_main
#	define wmain _my_wmain
#endif

#if defined(_MAIN) || defined(_CONSOLE)
#	ifndef _WINDLL
#		ifdef UNICODE
#			ifdef _CONSOLE
#				pragma comment(linker, "-entry:mainCRTStartup") // main in UNICODE too
#			else
#				pragma comment(linker, "-entry:wWinMainCRTStartup")
#			endif
#		else
#			ifdef _CONSOLE
#				pragma comment(linker, "-entry:mainCRTStartup")
#			else
#				pragma comment(linker, "-entry:WinMainCRTStartup")
#			endif
#		endif
#	endif
#endif

#if UCFG_WCE
#	define _TE(quote) L##quote
#else
#	define _TE(quote) quote
#endif

#define HRESULT_FROM_C(x) ((HRESULT)(((x)&0x0000FFFF) | (FACILITY_C << 16) | 0x80000000))

#if !UCFG_WCE && UCFG_FORCE_LINK && UCFG_EXTENDED //!!!
#	ifdef _USRDLL
#		pragma comment(linker, "/include:__afxForceUSRDLL")
#	endif

#	if defined(_AFXDLL) && !defined(_EXT) && !defined(USE_ONLY_LIBEXT)
#		pragma comment(linker, "/include:__afxForceEXT")
#	endif
#endif

#ifdef __cplusplus
#	include <cmath>
#	if !UCFG_STDSTL
#		undef nan
#		define nan API_nan
#	endif
#endif

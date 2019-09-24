#pragma once

#ifdef _DLL
#	define _AFXDLL
#endif

#if !defined(DEBUG) && defined(_DEBUG)
#	define DEBUG 1
#endif

#ifdef _DEBUG
#	define UCFG_DEBUG 1
#else
#	define UCFG_DEBUG 0
#endif

#ifdef _PROFILE
#	define UCFG_PROFILE 1
#else
#	define UCFG_PROFILE 0
#endif

#ifndef _WIN32_IE
#	define _WIN32_IE 0x500
#endif

#ifdef _WIN32
#	define NTDDI_WINLH NTDDI_VISTA //!!! not defined in current SDK
#endif

#ifndef _MSC_VER
#	ifdef __x86_64__
#		define _M_X64
#	elif defined(__i786__)
#		define _M_IX86 700
#	elif defined(__i686__)
#		define _M_IX86 600
#	elif defined(__i586__)
#		define _M_IX86 500
#	elif defined(__i486__)
#		define _M_IX86 400
#	elif defined(__i386__)
#		define _M_IX86 300
#	elif defined(__arm__)
#		define _M_ARM
#	elif defined(__mips__)
#		define _M_MIPS
#	endif

#	ifdef __SSE2__
#		define _M_IX86_FP 2
#	endif
#endif // ndef _MSC_VER

#if defined(__GNUC__) && __GNUC__ > 0 && !defined(__clang__)
#	define UCFG_GNUC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#	define UCFG_GNUC_VERSION 0
#endif

#ifdef __clang__
#	define UCFG_CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#else
#	define UCFG_CLANG_VERSION 0
#endif

#ifdef _MSC_VER
#	define UCFG_MSC_VERSION _MSC_VER
#	define UCFG_MSC_FULL_VERSION _MSC_FULL_VER
#else
#	define UCFG_MSC_VERSION 0
#	define UCFG_MSC_FULL_VERSION 0
#endif

#ifdef _MSVC_LANG
#	define UCFG_MSVC_LANG _MSVC_LANG
#else
#	define UCFG_MSVC_LANG 0
#endif

#if !defined(_MAC) && (defined(_M_M68K) || defined(_M_MPPC))
#	define _MAC
#endif

#ifdef _M_IX86
#	define UCFG_PLATFORM_IX86 1
#else
#	define UCFG_PLATFORM_IX86 0
#endif

#ifdef _M_X64
#	define UCFG_PLATFORM_X64 1
#else
#	define UCFG_PLATFORM_X64 0
#endif

#ifdef _M_X64
#	define UCFG_PLATFORM_SHORT_NAME "x64"
#elif defined(_M_IX86)
#	define UCFG_PLATFORM_SHORT_NAME "x86"
#elif defined(_M_MIPS)
#	define UCFG_PLATFORM_SHORT_NAME "mips"
#elif defined(_M_ARM)
#	define UCFG_PLATFORM_SHORT_NAME "arm"
#else
#	define UCFG_PLATFORM_SHORT_NAME "platform"
#endif

#if UCFG_PLATFORM_IX86 || UCFG_PLATFORM_X64 || defined(_M_MIPS) || defined(_M_ARM)
#	define UCFG_LITLE_ENDIAN 1
#endif

#if defined(_M_X64) || defined(__ppc64__) || defined(__mips64__) || defined(__arm64__) || defined(_WIN64) || (defined(__LP64__) && __LP64__)
#	define UCFG_64 1
#else
#	define UCFG_64 0
#endif

#define UCFG_CPU_X86_X64 (UCFG_PLATFORM_IX86 || UCFG_PLATFORM_X64)

#define UCFG_PLATFORM_64 UCFG_PLATFORM_X64

#ifndef UCFG_USE_MASM
#	define UCFG_USE_MASM UCFG_CPU_X86_X64
#endif

#ifdef _M_ARM
#	ifndef ARM		//!!!
#		define ARM //!!!
#	endif

#	ifndef _ARM_	 //!!!
#		define _ARM_ //!!!
#	endif

#	ifdef _MSC_VER
#		if _MSC_VER < 1600
#			ifndef _WIN32_WCE
#				define _WIN32_WCE 0x420
#			endif
#		else
#			define _CRT_BUILD_DESKTOP_APP 0
#		endif
#	endif
#endif

#define _CRT_RAND_S

#ifndef __STDC_VERSION__
#	if UCFG_MSC_VERSION >= 1700
#		define __STDC_VERSION__ 199901L
#	else
#		define __STDC_VERSION__ 1
#	endif
#endif

#ifdef _WIN32_WCE
#	ifndef WINCEOSVER
#		define WINCEOSVER _WIN32_WCE
#	endif

//	#ifndef _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA
//		#define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA
//	#endif
#	define UCFG_WCE 1
#else
#	define UCFG_WCE 0
#endif

#if !defined(_WIN32_WINNT) && !UCFG_WCE
#	define _WIN32_WINNT 0x602
#endif

#define _CORECRT_WIN32_WINNT _WIN32_WINNT_WINXP // To be compatible with XP
#define _VCRT_WIN32_WINNT _WIN32_WINNT_WINXP

#define _MSVCRT_WINDOWS

//!!!? not defined in SDK
//#define _WIN32_WINNT_WINTHRESHOLD 0x0700		//!!!? defined in sdkddkver.h
#define _WIN32_WINNT_WIN10_TH2 0x0A01
#define _WIN32_WINNT_WIN10_RS1 0x0A02
#define _WIN32_WINNT_WIN10_RS2 0x0A03
#define _WIN32_WINNT_WIN10_RS3 0x0A04
#define _WIN32_WINNT_WIN10_RS4 0x0A05
#define _NT_TARGET_VERSION_WIN10_RS4 _WIN32_WINNT_WIN10_RS4
#define _WIN32_WINNT_WIN10_RS5 0x0A06

#define NTDDI_WIN7SP1 0x06010001 //!!!?

#ifndef NTDDI_VERSION
#	define NTDDI_VERSION 0x06020000 //  NTDDI_WIN8
#endif

#define _APISET_MINWIN_VERSION 0x200

#define PSAPI_VERSION 1 // to use psapi.dll instead of new kernel32 replacement

#ifndef UCFG_WIN_CRYPT
#	define UCFG_WIN_CRYPT 0			// To avoid warnings in windows.h
#endif

#if !UCFG_WIN_CRYPT
#	define NOCRYPT
#endif

#ifndef __SPECSTRINGS_STRICT_LEVEL //!!!
#	define __SPECSTRINGS_STRICT_LEVEL 0
#endif

#ifdef _M_IX86
#	define _X86_
#endif

#ifdef _M_AMD64
#	define _AMD64_
#endif

#ifdef _UNICODE
#	define UCFG_UNICODE 1
#else
#	define UCFG_UNICODE 0
#endif

#if UCFG_WCE
#	ifndef _UNICODE
#		error libext/CE requires _UNICODE defined
#	endif

#	ifndef UNDER_CE
#		define UNDER_CE _WIN32_WCE
#	endif

#	ifndef WIN32
#		define WIN32
#	endif

#	define _CRTAPI1
#	define _CRTAPI2

#endif

#ifndef WDM_DRIVER
#	ifdef NDIS_WDM
#		define WDM_DRIVER
#	endif
#endif

#ifdef WDM_DRIVER
#	define UCFG_WDM 1
#else
#	define UCFG_WDM 0
#endif

#if UCFG_WDM
#	define _KERNEL_MODE 1 // we don't use -kernel compiler option to enable EH
#endif

#ifndef UCFG_EXT_C
#	define UCFG_EXT_C UCFG_WDM
#endif

#if !defined(WIN32) && defined(_WIN32) && !UCFG_WDM && !UCFG_EXT_C
#	define WIN32
#endif

#ifndef UCFG_WIN32
#	ifdef WIN32
#		define UCFG_WIN32 1
#	else
#		define UCFG_WIN32 0
#	endif
#endif

#define UCFG_WIN32_FULL (UCFG_WIN32 && !UCFG_WCE)

#ifndef UCFG_NTAPI
#	define UCFG_NTAPI (UCFG_WDM || UCFG_WIN32_FULL)
#endif

#ifndef WINVER
#	define WINVER 0x0600 //!!! 0x400
#endif

#ifndef UCFG_USE_WINDEFS
#	define UCFG_USE_WINDEFS UCFG_WIN32
#endif

#ifdef __cplusplus
#	define UCFG_CPLUSPLUS __cplusplus
#else
#	define UCFG_CPLUSPLUS 0
#endif

#ifndef UCFG_CPP20
#	if UCFG_CPLUSPLUS >= 202000 || (UCFG_MSVC_LANG >= 201704)
#		define UCFG_CPP20 1
#	else
#		define UCFG_CPP20 0
#	endif
#endif

#ifndef UCFG_CPP17
#	if UCFG_CPLUSPLUS >= 201700 || (UCFG_MSVC_LANG > 201402) || (UCFG_GNUC_VERSION && UCFG_CPLUSPLUS > 201402) || (UCFG_CLANG_VERSION >= 400) //!!!?
#		define UCFG_CPP17 1
#	else
#		define UCFG_CPP17 UCFG_CPP20
#	endif
#endif

#ifndef UCFG_CPP14
#	if UCFG_CPLUSPLUS >= 201400 || (UCFG_MSC_VERSION >= 1900) || (UCFG_GNUC_VERSION >= 410) || (UCFG_CLANG_VERSION >= 310) //!!!?
#		define UCFG_CPP14 1
#	else
#		define UCFG_CPP14 UCFG_CPP17
#	endif
#endif

#ifndef UCFG_CPP11
#	if UCFG_CPLUSPLUS > 199711 || defined(__GXX_EXPERIMENTAL_CXX0X__) || (UCFG_MSC_VERSION >= 1600) || UCFG_WCE
#		define UCFG_CPP11 1
#	else
#		define UCFG_CPP11 UCFG_CPP14
#	endif
#endif

#ifndef UCFG_CPP11_ENUM
#	if defined(_MSC_VER) && _MSC_VER < 1700
#		define UCFG_CPP11_ENUM 0
#	else
#		define UCFG_CPP11_ENUM UCFG_CPP11
#	endif
#endif

#ifndef UCFG_CPP11_EXPLICIT_CAST
#	if defined(_MSC_VER) && _MSC_VER <= 1700 || defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ == 4 && __GNUC_MINOR__ < 5
#		define UCFG_CPP11_EXPLICIT_CAST 0
#	else
#		define UCFG_CPP11_EXPLICIT_CAST UCFG_CPP11
#	endif
#endif

#ifndef UCFG_CPP11_NULLPTR
#	if defined(_NATIVE_NULLPTR_SUPPORTED)
#		define UCFG_CPP11_NULLPTR 1
#	elif defined(_MSC_VER) && _MSC_VER < 1600
#		define UCFG_CPP11_NULLPTR 0
#	elif defined(__GXX_EXPERIMENTAL_CXX0X__) && !defined(__clang__)
#		if __GNUC__ == 4 && __GNUC_MINOR__ <= 5
#			define UCFG_CPP11_NULLPTR 0
#		else
#			define UCFG_CPP11_NULLPTR 1
#		endif
#	else
#		define UCFG_CPP11_NULLPTR UCFG_CPP11
#	endif
#endif

#ifndef UCFG_CPP11_FOR_EACH
#	define UCFG_CPP11_FOR_EACH UCFG_CPP11
#endif

#ifndef UCFG_CPP11_RVALUE
#	if defined(_MSC_VER) && _MSC_VER < 1600
#		define UCFG_CPP11_RVALUE 0
#	else
#		define UCFG_CPP11_RVALUE UCFG_CPP11
#	endif
#endif

#ifndef UCFG_CPP11_OVERRIDE
#	define UCFG_CPP11_OVERRIDE (UCFG_GNUC_VERSION >= 407 || UCFG_MSC_VERSION >= 1600 || (!UCFG_GNUC_VERSION && !UCFG_MSC_VERSION && UCFG_CPP11))
#endif

#ifndef UCFG_CPP11_CONSTEXPR
#	define UCFG_CPP11_CONSTEXPR (UCFG_CPP11 && (!UCFG_MSC_VERSION || UCFG_MSC_VERSION >= 1900))
#endif

#ifndef UCFG_CPP14_NOEXCEPT
#	define UCFG_CPP14_NOEXCEPT (UCFG_GNUC_VERSION >= 406 || UCFG_MSC_VERSION >= 1900 || (!UCFG_GNUC_VERSION && !UCFG_MSC_VERSION && UCFG_CPP11))
#endif

#ifndef EXT_NOEXCEPT
#	ifdef __cplusplus
#		define EXT_NOEXCEPT noexcept
#	else
#		define EXT_NOEXCEPT
#	endif
#endif

#ifndef UCFG_C99_FUNC
#	define UCFG_C99_FUNC (!UCFG_MSC_VERSION || UCFG_MSC_VERSION >= 1900)
#endif

#ifndef UCFG_CPP11_THREAD_LOCAL
#	define UCFG_CPP11_THREAD_LOCAL (UCFG_CLANG_VERSION >= 305 || UCFG_GNUC_VERSION >= 407 || UCFG_MSC_VERSION >= 1900 || (!UCFG_GNUC_VERSION && !UCFG_MSC_VERSION && UCFG_CPP11))
#endif

#ifdef __cplusplus
#	if !UCFG_MSC_VERSION
#		include <ciso646>
#	endif

#	ifdef _LIBCPP_VERSION
#		define UCFG_LIBCPP_VERSION _LIBCPP_VERSION
#	else
#		define UCFG_LIBCPP_VERSION 0
#	endif

#	ifndef UCFG_STD_SHARED_MUTEX
#		define UCFG_STD_SHARED_MUTEX (UCFG_CPP17 || UCFG_CPP14 && (UCFG_LIBCPP_VERSION >= 1100 || UCFG_MSC_VERSION >= 2000 || UCFG_CLANG_VERSION >= 306))
#	endif

#	ifndef UCFG_STD_OPTIONAL
#		define UCFG_STD_OPTIONAL UCFG_CPP17
#	endif

#	ifndef UCFG_STD_SPAN
#		define UCFG_STD_SPAN (UCFG_CPP20)
#	endif

#	ifndef UCFG_STD_CONCEPTS
#		define UCFG_STD_CONCEPTS (UCFG_CPP20)
#	endif

#	ifndef UCFG_CPP11_HAVE_REGEX
#		define UCFG_CPP11_HAVE_REGEX (UCFG_LIBCPP_VERSION >= 1100 || UCFG_GNUC_VERSION >= 410 || UCFG_MSC_VERSION >= 1600)
#	endif

#	ifndef UCFG_STD_THREAD
#		define UCFG_STD_THREAD (UCFG_LIBCPP_VERSION >= 1100 || UCFG_GNUC_VERSION >= 400 || UCFG_MSC_VERSION >= 1700)
#	endif

#	ifndef UCFG_STD_FILESYSTEM
#		define UCFG_STD_FILESYSTEM (UCFG_CPP14 && !(UCFG_LIBCPP_VERSION && UCFG_LIBCPP_VERSION < 1200))
#	endif

#	ifndef UCFG_STD_DYNAMIC_BITSET
#		define UCFG_STD_DYNAMIC_BITSET 0 //!!! (UCFG_CPP14 && !(UCFG_LIBCPP_VERSION && UCFG_LIBCP_VERSION < 1200) && !(UCFG_MSC_VERSION && UCFG_MSC_VERSION <= 1900))
#	endif

#	ifndef UCFG_STD_MUTEX
#		define UCFG_STD_MUTEX (UCFG_CPP11 || UCFG_MSC_VERSION >= 1700 || UCFG_LIBCPP_VERSION >= 1100)
#	endif

#	ifndef UCFG_STD_HASH
#		define UCFG_STD_HASH (UCFG_CPP14 || UCFG_MSC_VERSION >= 1900)
#	endif

#	ifndef UCFG_STD_BACK_INSERT_ITERATOR
#		define UCFG_STD_BACK_INSERT_ITERATOR (UCFG_CPP14 || UCFG_MSC_VERSION >= 1900)
#	endif
#endif // __cplusplus

#ifndef UCFG_HAVE_STATIC_ASSERT
#	define UCFG_HAVE_STATIC_ASSERT UCFG_CPP11
#endif

#ifndef UCFG_STD_DECIMAL
#	define UCFG_STD_DECIMAL (UCFG_GNUC_VERSION >= 410 || UCFG_MSC_VERSION >= 2000)
#endif

#ifndef UCFG_STD_SYSTEM_ERROR
#	define UCFG_STD_SYSTEM_ERROR UCFG_CPP11
#endif

#ifndef UCFG_CPP11_BEGIN
#	define UCFG_CPP11_BEGIN (UCFG_CPP14 || UCFG_GNUC_VERSION >= 406 || UCFG_MSC_VERSION >= 1600)
#endif

#if !defined(_HAS_CHAR16_T_LANGUAGE_SUPPORT) && UCFG_CPP11 && (!UCFG_MSC_VERSION || UCFG_MSC_VERSION >= 1900)
#	define _HAS_CHAR16_T_LANGUAGE_SUPPORT 1
#endif

#ifndef UCFG_STD_IDENTITY
#	define UCFG_STD_IDENTITY (UCFG_MSC_VERSION >= 1500)
#endif

#ifndef UCFG_STD_SIZE
#	define UCFG_STD_SIZE (UCFG_CPP17 || UCFG_MSC_VERSION >= 1900)
#endif

#ifndef UCFG_STD_UNCAUGHT_EXCEPTIONS
#	define UCFG_STD_UNCAUGHT_EXCEPTIONS (UCFG_CPP17 || UCFG_MSC_VERSION >= 1900)
#endif

#if UCFG_WDM || (UCFG_MSC_VERSION && !defined _CPPUNWIND)
#	define UCFG_USE_IN_EXCEPTION 0
#else
#	define UCFG_USE_IN_EXCEPTION UCFG_STD_UNCAUGHT_EXCEPTIONS
#endif



#ifndef UCFG_STD_CLAMP
#	define UCFG_STD_CLAMP UCFG_CPP17
#endif

#ifndef UCFG_STD_OBSERVER_PTR
#	define UCFG_STD_OBSERVER_PTR 0
#endif

#define WIN9X_COMPAT_SPINLOCK

#ifdef _CPPUNWIND
#	define _HAS_EXCEPTIONS 1
#else
#	define _HAS_EXCEPTIONS 0
#endif

#define _HAS_NAMESPACE 1

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_ALLOW_CHAR_UNSIGNED // to prevent ATL error in VS11

/*!!!R
#ifndef _MSC_VER

#	ifdef __CHAR_UNSIGNED__
#		define _CHAR_UNSIGNED
#	endif

#endif
*/

#define _HAS_TRADITIONAL_STL 1

#ifndef _FILE_OFFSET_BITS
#	define _FILE_OFFSET_BITS 64
#endif

#ifndef _STRALIGN_USE_SECURE_CRT
#	define _STRALIGN_USE_SECURE_CRT 0
#endif

#ifndef UCFG_POOL_THREADS
#	define UCFG_POOL_THREADS 25
#endif

#ifndef UCFG_EH_OS_UNWIND
#	define UCFG_EH_OS_UNWIND (defined _M_X64)
#endif

#define UCFG_EH_PDATA (!defined _M_IX86)

#ifndef UCFG_EH_DIRECT_CATCH
#	define UCFG_EH_DIRECT_CATCH (!UCFG_WCE)
#endif

#ifndef UCFG_EHTRACE
#	define UCFG_EHTRACE 0
#endif

#ifndef UCFG_ISBAD_API
#	if defined(_CRTBLD)
#		define UCFG_ISBAD_API 0
#	else
#		define UCFG_ISBAD_API 1
#	endif
#endif

#if !defined(_MSC_VER) && defined(__GNUC__)
#	define UCFG_GNUC 1
#else
#	define UCFG_GNUC 0
#endif

#if UCFG_WDM && (UCFG_CPU_X86_X64 || defined(_M_ARM)) //!!!?
#	define UCFG_FPU 0
#else
#	define UCFG_FPU 1
#endif

#ifdef _DEBUG
#	define DBG 1
#	define DEBUG 1
#else
#	define DBG 0
#endif

#define _COMPLEX_DEFINED

#ifdef _MSC_VER
#	define HAVE_SNPRINTF 1
#	define HAVE_STRSTR 1
#	define HAVE_STRLCPY
#	define HAVE_STRDUP 1
#	define HAVE_VSNPRINTF 1
#	define HAVE_MEMMOVE 1
#	define HAVE_FCNTL_H 1
#	define HAVE_STRSEP
#	define HAVE_SOCKADDR_STORAGE
#	define HAVE_SQLITE3 1
#	define HAVE_LIBGMP 1
#	define HAVE_LIBNTL 1
#	define HAVE_BERKELEY_DB 1
#	define HAVE_JANSSON 1
#	define HAVE_PCRE2 1
#endif // _MSC_VER

#ifndef UCFG_DEFINE_NOMINMAX
#	define UCFG_DEFINE_NOMINMAX 1
#endif

#if UCFG_DEFINE_NOMINMAX
#	define NOMINMAX // used in windows.h
#endif

#ifndef UCFG_DEFINE_OLD_NAMES
#	define UCFG_DEFINE_OLD_NAMES 0
#endif

#if UCFG_GNUC_VERSION || UCFG_CLANG_VERSION
#	if defined(_M_X64) || defined(_M_ARM) || defined(_M_MIPS)
#		define __cdecl
#		define _cdecl
#		define __stdcall
#		define _stdcall
#		define __fastcall
#		define _fastcall
#		define __thiscall
#		define _thiscall
#	else
#		ifndef __cdecl
#			define __cdecl __attribute((__cdecl__))
#		endif

#		ifndef _cdecl
#			define _cdecl __attribute((__cdecl__))
#		endif

#		ifndef __stdcall
#			define __stdcall __attribute((__stdcall__))
#		endif

#		ifndef _stdcall
#			define _stdcall __attribute((__stdcall__))
#		endif

#		ifndef __fastcall
#			define __fastcall __attribute((__fastcall__))
#		endif

#		ifndef _fastcall
#			define _fastcall __attribute((__fastcall__))
#		endif

#		ifndef __thiscall
#			define __thiscall __attribute((__thiscall__))
#		endif

#		ifndef _thiscall
#			define _thiscall __attribute((__thiscall__))
#		endif
#	endif // defined(_M_X64) || defined(_M_ARM) || defined(_M_MIPS)

#	ifndef PASCAL
#		define PASCAL
#	endif
#endif // UCFG_GNUC_VERSION || UCFG_CLANG_VERSION

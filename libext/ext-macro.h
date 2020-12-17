#pragma once

#ifndef _INC_EXT_MACRO_H
#define _INC_EXT_MACRO_H


#define MULTI_CHAR_2(a, b) ((a) | ((b) << 8))
#define MULTI_CHAR_3(a, b, c) ((a) | ((b) << 8) | ((c) << 16))
#define MULTI_CHAR_4(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#ifndef _WIN32

typedef uint8_t BYTE;
typedef char TCHAR;
typedef long LONG;
typedef unsigned short WCHAR;
typedef long long LONGLONG;
typedef DWORD ACCESS_MASK;
typedef BYTE  BOOLEAN;
typedef void *LPVOID;
typedef unsigned long ULONG32;

typedef struct _LARGE_INTEGER {
	union  {
		struct {
			DWORD LowPart;
			LONG HighPart;
		} DUMMYSTRUCTNAME;
		struct {
			DWORD LowPart;
			LONG HighPart;
		} u;
		LONGLONG QuadPart;
	};
} LARGE_INTEGER;

#define FACILITY_HTTP                    25

#	define MAKEWORD(a, b)      ((uint16_t)(((BYTE)(((uintptr_t)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#	define MAKELONG(a, b)      ((LONG)(((WORD)(((uintptr_t)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#	define LOWORD(l)           ((uint16_t)(((uintptr_t)(l)) & 0xffff))
#	define HIWORD(l)           ((uint16_t)((((uintptr_t)(l)) >> 16) & 0xffff))
#	define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#	define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

typedef void *HMODULE;
typedef void *HINSTANCE;
//!!!typedef void (*FARPROC)();

/*!!!R

#ifdef __cplusplus
	namespace Ext {
#endif

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#ifdef __cplusplus
	}
#endif

*/

#define MAX_COMPUTERNAME_LENGTH 31		//!!!
#define CP_UTF7                   65000       // UTF-7 translation
#define CP_UTF8                   65001       // UTF-8 translation


#define ASSERT assert

#define interface struct

#define VAR_TIMEVALUEONLY       ((DWORD)0x00000001)    /* return time value */
#define VAR_DATEVALUEONLY       ((DWORD)0x00000002)    /* return date value */


#ifndef EMAKEHR
#	define SMAKEHR(val)            MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_URT, val)
#	define EMAKEHR(val)            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_URT, val)
#endif

#define COR_E_FORMAT  EMAKEHR(0x1537L)

#define THREAD_BASE_PRIORITY_MIN    (-2)  // minimum thread base priority boost
#define THREAD_PRIORITY_LOWEST THREAD_BASE_PRIORITY_MIN


#endif // _WIN32

#if !defined(WIN32) && !defined(_WINDEF_)

#ifdef __cplusplus
	namespace Ext {
#endif

	typedef struct _FILETIME {
		uint32_t dwLowDateTime;
		uint32_t dwHighDateTime;
	} FILETIME, *PFILETIME, *LPFILETIME;

#ifdef __cplusplus
	}
#endif

#endif

#ifdef _WIN32
__BEGIN_DECLS
#define	BUS_SPACE_IO	0	/* space is i/o space */
#define BUS_SPACE_MEM	1	/* space is mem space */

unsigned int __cdecl bus_space_read_4(int bus, void *addr, int off);
void __cdecl bus_space_write_4(int bus, void *addr, int off, unsigned int val);
__END_DECLS
#endif

#ifndef CP_UTF8
#	define CP_UTF8 65001
#endif

#endif // _INC_EXT_MACRO_H

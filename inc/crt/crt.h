#pragma once


__BEGIN_DECLS


#undef _wassert
void __cdecl API_wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line);
#define _wassert API_wassert

#if UCFG_PLATFORM_IX86
#	undef _strtoi64
	__int64 __cdecl API_strtoi64(const char *nptr, char **endptr, int ibase);
#	define _strtoi64 API_strtoi64

#	undef _strtoui64
unsigned __int64 __cdecl API_strtoui64(const char *nptr, char **endptr, int ibase);
#	define _strtoui64 API_strtoui64
#endif

int _cdecl API_utime(const char *filename, struct _utimbuf *timbuf);
#define utime API_utime

time_t _cdecl API_time32(time_t *x);
#define time API_time32

struct _timeb {
	time_t time;
	unsigned short millitm;
	short timezone;
	short dstflag;
};

void _cdecl API_ftime(struct _timeb *timeptr);
#define ftime API_ftime
#define _ftime API_ftime


time_t _cdecl API_mktime(struct tm * _Tm);
#define mktime API_mktime

struct tm * _cdecl API_localtime32(const time_t * _Time);
#define localtime API_localtime32

struct tm * _cdecl API_gmtime(const time_t * _Time);
#define gmtime API_gmtime

char * _cdecl API_ctime(const time_t * _Time);
#define ctime API_ctime

errno_t _cdecl API_strncpy_s(char *dst, size_t dstsize, const char *src, size_t maxSize);
#define strncpy_s API_strncpy_s

errno_t _cdecl API_strcpy_s(char *dst, size_t dstsize, const char *src);
#define strcpy_s API_strcpy_s

size_t __cdecl API_strnlen(const char *str, size_t maxsize);
#define strnlen API_strnlen

errno_t _cdecl API_wcsncpy_s(wchar_t *dst, size_t _SizeInWords, const wchar_t * _Src, size_t _MaxCount);
#define wcsncpy_s API_wcsncpy_s

errno_t _cdecl API_strcat_s(char *dst, size_t dstsize, const char *src);
#define strcat_s API_strcat_s

errno_t _cdecl API_memcpy_s(void *dst, size_t dstsize, const void *src, size_t size);
#define memcpy_s API_memcpy_s

errno_t _cdecl API_memmove_s(void * dst, size_t sizeInBytes, const void * src, size_t count);
#define memmove_s API_memmove_s

int _cdecl API_sprintf_s(char *dst, size_t dstsize, const char *format, ...);
#define sprintf_s API_sprintf_s

int _cdecl API_vscprintf(const char *format, va_list list);
#define _vscprintf API_vscprintf


#undef _timezone
DECLSPEC_DLLIMPORT long _timezone;
#define timezone _timezone

#undef _tzname
DECLSPEC_DLLIMPORT char * _tzname[];

#undef _daylight
DECLSPEC_DLLIMPORT int _daylight;


/*!!!R
struct my_stat {
	_dev_t     st_dev;
	_ino_t     st_ino;
	unsigned short st_mode;
	short      st_nlink;
	short      st_uid;
	short      st_gid;
	_dev_t     st_rdev;
	_off_t     st_size;
	__time32_t st_atime;
	__time32_t st_mtime;
	__time32_t st_ctime;
};*/

/*!!!
#define stat my_stat32
__inline int __stdcall my_stat32(const char *name, struct stat *s)
{
return _stat(name, (struct _stat32 *)s);
}*/

	//		int __stdcall my_stat(const char *name, struct stat * _Stat);

//!!!#undef _fstat
//!!!#define _fstat my_fstat
//!!!	int __stdcall my_fstat(int _Desc, struct _stat* _Stat);

	/*!!!R
//#	define stat C_stat
//#	define _stat64i32 C__stat64i32
#	define fstat C_fstat
#	define _fstat C__fstat
#		include <sys/stat.h>
#	undef stat
#	undef _stat
#	undef fstat
#	undef _fstat
#	undef _wstat
//typedef struct _stat64i32 My_stat64i32;

#	ifdef _USE_32BIT_TIME_T

		int __cdecl API__fstat32(_In_ int _FileDes, _Out_ struct _stat32 * _Stat);
#		define _fstat32 API__fstat32
#		define fstat API__fstat32

#	else

//!!!#		define stat API__stat64i32

#			define _wstat API__wstat64i32

#			define fstat API__fstat64i32
#			define _fstat API__fstat64i32
//!!!#		define _stat64i32 API__stat64i32

		struct API_stat64i32 {
			_dev_t     st_dev;
			_ino_t     st_ino;
			unsigned short st_mode;
			short      st_nlink;
			short      st_uid;
			short      st_gid;
			_dev_t     st_rdev;
			_off_t     st_size;
			__time64_t st_atime;
			__time64_t st_mtime;
			__time64_t st_ctime;
		};
		int __cdecl API__fstat64i32(_In_ int _FileDes, _Out_ struct API_stat64i32 * _Stat);
		EXT_API int __cdecl API_stat64i32(const char *filename, struct API_stat64i32 * _Stat);
		int __cdecl API__wstat64i32(const wchar_t *filename, struct API_stat64i32 * _Stat);
#			define stat API_stat64i32
#			define _stat API_stat64i32


#	endif
*/

#define _findfirst64i32 Ñ_findfirst64i32
#define _findnext64i32 C_findnext64i32
#define isatty C_isatty

#ifdef __cplusplus
#	define _open C_open
#endif

#undef close
#define close C_close

#if !UCFG_WDM
#	include <io.h>
#endif
#undef _findfirst
#undef _findnext
#undef isatty
#undef close

int __cdecl API_close(int fh);
#	define close API_close

#ifdef __cplusplus
#	undef _open

#	define _open API_open

#endif

#ifndef _USE_32BIT_TIME_T
	intptr_t _cdecl API__findfirst64i32(const char * _Filename, struct _finddata64i32_t * _FindData);
#	define _findfirst API__findfirst64i32

	int _cdecl API__findnext64i32(intptr_t _FindHandle, struct _finddata64i32_t * _FindData);
#	define _findnext API__findnext64i32

#endif

//!!!#	define stat _stat32



EXT_API int __cdecl my_resetstkoflw (void);
#define _resetstkoflw my_resetstkoflw

void * __cdecl my_recalloc(void * memblock, size_t count, size_t size);
#define _recalloc my_recalloc


unsigned int __cdecl API__set_abort_behavior(unsigned int _Flags, unsigned int _Mask);
#define _set_abort_behavior API__set_abort_behavior

_locale_t __cdecl API__create_locale(int _Category, const char * _Locale);
#define _create_locale API__create_locale

void __cdecl API__free_locale(_locale_t _Locale);
#define _free_locale API__free_locale

int __cdecl API__wopen(const wchar_t *filename, int flags, ...);
#define _wopen API__wopen

int __cdecl API_swprintf(wchar_t * _DstBuf, const wchar_t * _Format, ...);

#ifdef __cplusplus
extern "C++" {
int __cdecl API_swprintf(wchar_t * _DstBuf, size_t count, const wchar_t * _Format, ...);
}
#endif


#define _swprintf API_swprintf
#define swprintf _swprintf

int __cdecl API_swprintf_s(wchar_t * _DstBuf, size_t sizeBuf, const wchar_t * _Format, ...);
#define swprintf_s API_swprintf_s

int __cdecl API_vswprintf(wchar_t * _DstBuf, size_t _MaxCount, const wchar_t * _Format, va_list _ArgList);
#define _vswprintf API_vswprintf
#define vswprintf _vswprintf

#define vswprintf_s _vswprintf	//!!!?

int __cdecl API__vswprintf_p(wchar_t * _DstBuf, size_t _MaxCount, const wchar_t * _Format, va_list _ArgList);
#define _vswprintf_p API__vswprintf_p

int __cdecl API__vswprintf_c_l(wchar_t * _DstBuf, size_t _MaxCount, const wchar_t * _Format, _locale_t _Locale, va_list _ArgList);
#define _vswprintf_c_l API__vswprintf_c_l

double __cdecl API__wcstod_l(const wchar_t *_Str, wchar_t ** _EndPtr, _locale_t _Locale);
#define _wcstod_l API__wcstod_l

__int64  __cdecl API__wcstoi64(const wchar_t * _Str, wchar_t ** _EndPtr, int _Radix);
#define _wcstoi64 API__wcstoi64

unsigned __int64  __cdecl API__wcstoui64(const wchar_t * _Str, wchar_t ** _EndPtr, int _Radix);
#define _wcstoui64 API__wcstoui64

unsigned long __cdecl API__wcstoul_l(const wchar_t *_Str, wchar_t **_EndPtr, int _Radix, _locale_t _Locale);
#define _wcstoul_l API__wcstoul_l


double __cdecl API_difftime(time_t time1, time_t time2);
#define difftime API_difftime

double API_nan(const char *tagp);
#define nan API_nan

#ifndef __cplusplus
#	undef signbit
#	define signbit(v) ((int)(v < 0))
#endif

#ifdef __cplusplus
	int __cdecl API_open(const char *fn, int flags, int mask = 0);
#else
	int __cdecl API_open(const char *fn, int flags, ...);
#endif

errno_t __cdecl API_gmtime64_s(struct tm *ptm, const __time64_t *timp);
#define _gmtime64_s API_gmtime64_s

__END_DECLS

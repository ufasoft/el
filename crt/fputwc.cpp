/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com      ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/


//   fputwc() implementation with fixed UTF-8 output for MSVC

#include <io.h>
#include <stdio.h>
#include <crtversion.h>

#include <windows.h>

int _ext_crt_module_fputwc;

extern "C" {

#if _VC_CRT_MAJOR_VERSION>=14
	void __cdecl __acrt_errno_map_os_error(unsigned long const oserrno);
#else
	void __cdecl _dosmaperr(unsigned long);
	inline void __cdecl __acrt_errno_map_os_error(unsigned long const oserrno) { return _dosmaperr(oserrno); }
#endif // _VC_CRT_MAJOR_VERSION

} // "C"

extern "C" wint_t __cdecl _fputwc_nolock(wchar_t c, FILE *stream) {
	int fh = _fileno(stream);
	if (_isatty(fh)) {
		HANDLE h = (HANDLE)_get_osfhandle(fh);
		if (INVALID_HANDLE_VALUE != h) {
			DWORD dwOut;
			if (::WriteConsoleW(h, &c, 1, &dwOut, 0))
				return c;
			__acrt_errno_map_os_error(::GetLastError());
		}
		return WEOF;
	}	

	char buf[10], *p = buf;									// convert to to UTF-8
	int n = 0;												// we assume fputwc used only for TEXT streams
	if (c < 0x80)
		*p++ = char(c);
	else if (c < 0x800) {
		*p++ = char(0xC0 | (c>>6));
		n = 1;
	} else {
		*p++ = char(0xE0 | (c>>12));
		n = 2;
	}
	while (n--)
		*p++ = char((c >> 6*n) & 0x3F | 0x80);
	return 0<_write(fh, buf, unsigned(p-buf)) ? c : WEOF;
}

extern "C" wint_t __cdecl fputwc(wchar_t c, FILE *stream) {
	return _fputwc_nolock(c, stream);							// no need for locking
}


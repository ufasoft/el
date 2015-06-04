/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/


//   write() implementation with fixed UTF-8 output for MSVC

#include <io.h>
#include <crtversion.h>

#include <windows.h>


extern "C" {

#if _VC_CRT_MAJOR_VERSION>=14
	void __cdecl __acrt_errno_map_os_error(unsigned long const oserrno);
	void __cdecl __acrt_lowio_lock_fh(int fh);
	void __cdecl __acrt_lowio_unlock_fh(int fh);
#else
	void __cdecl _dosmaperr(unsigned long);
	
	inline void __cdecl __acrt_errno_map_os_error(unsigned long const oserrno) { return _dosmaperr(oserrno); }
	inline void __cdecl __acrt_lowio_lock_fh(int fh) { __lock_fhandle(fh);  }
	inline void __cdecl __acrt_lowio_unlock_fh(int fh) { _unlock_fhandle(fh); }
#endif // _VC_CRT_MAJOR_VERSION

} // "C"

struct Utf8State {
	wchar_t Char;
	byte N;
};

class CFileHandleLock {
public:
	CFileHandleLock(int fh) {
		__acrt_lowio_lock_fh(m_fh = fh);
	}

	~CFileHandleLock() {
		__acrt_lowio_unlock_fh(m_fh);
	}
private:
	int m_fh;
};


const int N_STATES = 3;

static Utf8State s_states[N_STATES];


extern "C" int __cdecl _write_nolock(int fh, const void *buffer, unsigned size) {
	HANDLE h = (HANDLE)_get_osfhandle(fh);
	if (INVALID_HANDLE_VALUE == h)
		return -1;
	if (!_isatty(fh) || fh>=N_STATES) {
		DWORD dw;
	    if (!WriteFile(h, buffer, size, &dw, nullptr)) {
			switch (int err = ::GetLastError()) {
			case ERROR_ACCESS_DENIED:
				errno = EBADF;
			    _doserrno = err;
				break;
			default:
              __acrt_errno_map_os_error(err);
			}
			return -1;
		}
		return dw;
	}

	const unsigned char*p = (const unsigned char*)buffer;

	Utf8State& s = s_states[fh];
	unsigned i = 0;
	while (i < size) {
		DWORD dwOut;
		const size_t WBUF_SIZE = 16;
		wchar_t wbuf[WBUF_SIZE];
		wchar_t *pw = wbuf;
		while (pw<wbuf+WBUF_SIZE) {
			while (s.N) {
				if (i>=size)
					break;
				unsigned char ch = p[i];
				if (ch<0x80 || ch>=0xC0) {
					s.N = 0;					// error, resync, char is valid
					break;
				} else {
					++i;
					s.Char = s.Char<<6 | ch & 0x3F;
					if (--s.N == 0)
						*pw++ = s.Char;
				}
			}
			if (i>=size || pw>=wbuf+WBUF_SIZE)
				break;
			unsigned char ch = p[i++];
			if (ch < 0x80)
				*pw++ = ch;
			else if (ch < 0xC0)
				*pw++ = '?';							// error, resync
			else if (ch < 0xE0) {
				s.N = 1;
				s.Char = ch & 0x1F;
			} else if (ch < 0xF0) {
				s.N = 2;
				s.Char = ch & 0xF;
			} else if (ch < 0xF8) {
				s.N = 3;
				s.Char = ch & 7;
			} else {
				s.Char = ch & 3;
				s.N = ch<0xFC ? 4 : 5;
			}
		}
		
		if (pw != wbuf && !::WriteConsoleW(h, wbuf, pw-wbuf, &dwOut, 0)) {
			__acrt_errno_map_os_error(::GetLastError());
			return i;
		}
	}
	return (int)i;
}

extern "C" int __cdecl _write(int fh, const void *buffer, unsigned size) {
	CFileHandleLock lockFileHandle(fh);

	return _write_nolock(fh, buffer, size);
}






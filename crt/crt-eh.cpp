/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#ifdef _MSC_VER && _VC_CRT_MAJOR_VERSION<14 && UCFG_STDSTL
#	include <../crt/src/mtdll.h>
#endif

extern "C" {

#ifndef _MSC_VER
struct __cxa_eh_globals {
	void *m_dummy;
	int ProcessingThrow;
};
__cxa_eh_globals *__cxa_get_globals() noexcept;
#endif // !_MSC_VER

#if !UCFG_STD_UNCAUGHT_EXCEPTIONS
int __cdecl __uncaught_exceptions() {
#	ifdef _MSC_VER
	return _getptd()->_ProcessingThrow;
#	else
	return __cxa_get_globals()->ProcessingThrow;
#	endif // !_MSC_VER
}
#endif // UCFG_STD_UNCAUGHT_EXCEPTIONS


} // "C"



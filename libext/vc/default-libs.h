#pragma once


#	if UCFG_WDM || UCFG_CRT=='U'
//	#define _STL70_
#		pragma comment(linker, "/NODEFAULTLIB:libcmt.lib")
#		pragma comment(linker, "/NODEFAULTLIB:libcmtd.lib")
#		pragma comment(linker, "/NODEFAULTLIB:libcpmt.lib")
#		pragma comment(linker, "/NODEFAULTLIB:libcpmtd.lib")
#		pragma comment(linker, "/NODEFAULTLIB:libcp.lib")
#	endif

#	if UCFG_WCE
#		pragma comment(linker, "/NODEFAULTLIB:secchk.lib")
#	endif

#	if (defined(WIN32) && (UCFG_CRT=='O' || UCFG_CRT=='U')) || UCFG_WCE
#		pragma comment(linker, "/NODEFAULTLIB:msvcrt.lib")
#		pragma comment(linker, "/NODEFAULTLIB:msvcrtd.lib")
#		pragma comment(linker, "/NODEFAULTLIB:msvcprt.lib")
#		pragma comment(linker, "/NODEFAULTLIB:msvcprtd.lib")
#		pragma comment(linker, "/NODEFAULTLIB:threadsafestatics.lib")
#	endif

#	if UCFG_CRT=='U' && defined(WIN32) && !defined(_CRTBLD)
#		pragma comment(lib, "c++")
#	elif UCFG_FRAMEWORK && UCFG_USE_OLD_MSVCRTDLL && defined(WIN32)
#		if !UCFG_WCE

#			ifdef _WIN64

/*!!!R
#			ifdef _DEBUG
#				pragma comment(lib, "C:\\DK\\WDK\\Lib\\Crt\\amd64\\msvcrtd.lib ") //!!! no such .DLL
#			else
#				pragma comment(lib, "C:\\DK\\WDK\\Lib\\Crt\\amd64\\msvcrt.lib ") //!!! C:
#			endif
*/

//!!!#				pragma comment(lib, "C:\\DK\\WDK\\Lib\\Crt\\amd64\\msvcrt.lib ") //!!! C:
#				pragma comment(lib, "C:\\P\\Windows Kits\\8.0\\lib\\crt\\x64\\msvcrt.lib ") //!!! C:
#			else

#				ifdef _DEBUG
#					ifdef CRTDBG_MAP_ALLOC
#						pragma comment(lib, "\\src\\foreign\\lib\\o_msvcrtd.lib") //!!!
#					else	  
#						pragma comment(lib, "\\src\\foreign\\lib\\o_msvcrt.lib")
//!!!#					pragma comment(lib, "C:\\DK\\WDK\\lib\\crt\\i386\\msvcrt")

#					endif	
#					define _AFX_NO_DEBUG_CRT
#				else
#					pragma comment(lib, "\\src\\foreign\\lib\\o_msvcrt.lib")
#				endif
#			endif

//!!!#		pragma comment(linker, "/NODEFAULTLIB:oldnames.lib")
#		endif // !UCFG_WCE
#	endif // UCFG_FRAMEWORK && !UCFG_STDSTL && defined(WIN32)

#if UCFG_FRAMEWORK && UCFG_WCE
#	pragma comment(lib, "coredll")
#	pragma comment(lib, "corelibc")
#	pragma comment(linker, "/NODEFAULTLIB:uuid.lib")
#endif

#ifndef _AFXDLL
#	define _ALLOW_RUNTIME_LIBRARY_MISMATCH
#endif



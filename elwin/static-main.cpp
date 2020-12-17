/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

using namespace Ext;

#undef main
#undef wmain

extern "C" int __cdecl _my_wmain(int argc, wchar_t *argv[], wchar_t *envp[]);
extern "C" int __cdecl _my_main(int argc, char *argv[], char *envp[]);

int _cdecl ext_main(int argc, argv_char_t *argv[], argv_char_t *envp[]) {
#if UCFG_WCE
	RegistryKey(HKEY_LOCAL_MACHINE, "Drivers\\Console").SetValue("OutputTo", 0);
#endif

	atexit(MainOnExit);

#if UCFG_ARGV_UNICODE
	return _my_wmain(argc, argv, envp);
#else
	return _my_main(argc, argv, envp);
#endif
}

#if UCFG_WCE
#	if UCFG_ARGV_UNICODE
#		pragma comment(linker, "/ENTRY:mainWCRTStartup")
#	else
#		pragma comment(linker, "/ENTRY:mainACRTStartup")
#	endif
#endif

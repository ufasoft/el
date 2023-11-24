#pragma once

#if UCFG_LIB_DECLS && !defined(_EXT_GUI) && !defined(_EXT)
//#	pragma comment(lib, "gui")
#endif


#ifdef _AFXDLL
#	ifdef _EXT_GUI
#		ifdef _WIN64
#			define EXT_GUI_API
#		else
#			define EXT_GUI_API DECLSPEC_DLLEXPORT
#		endif
#	else
#		ifdef _WIN64
#			define EXT_GUI_API
#		else
#			define EXT_GUI_API DECLSPEC_DLLIMPORT
#		endif
#	endif
#else
#	define EXT_GUI_API
#endif

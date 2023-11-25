#pragma once

#include <file_config.h>
#include <prj_config.h>

#ifndef VER_PRODUCTVERSION_BUILD
#	define VER_PRODUCTVERSION_BUILD 0
#endif

#define EXT_Q(x) #x
#define EXT_QUOTE_VER(mj, mn) EXT_Q(mj.mn)
#define EXT_QUOTE_VER3(mj, mn, build) EXT_Q(mj.mn.build)

#if defined(VER_PRODUCTVERSION_MAJOR)
#	if !defined(VER_PRODUCTVERSION)
#		define VER_PRODUCTVERSION VER_PRODUCTVERSION_MAJOR,VER_PRODUCTVERSION_MINOR,0,0
#	endif

#	if !defined(VER_PRODUCTVERSION_STR)
#		define VER_PRODUCTVERSION_STR EXT_QUOTE_VER(VER_PRODUCTVERSION_MAJOR, VER_PRODUCTVERSION_MINOR)
#	endif

#	if !defined(VER_PRODUCTVERSION_STR3)
#		define VER_PRODUCTVERSION_STR3 EXT_QUOTE_VER3(VER_PRODUCTVERSION_MAJOR, VER_PRODUCTVERSION_MINOR, VER_PRODUCTVERSION_BUILD)
#	endif

#	ifndef VER_FILEVERSION_STR
#		define VER_FILEVERSION_STR VER_PRODUCTVERSION_STR
#	endif

#	ifndef VER_FILEDESCRIPTION_STR
#		define VER_FILEDESCRIPTION_STR VER_PRODUCTNAME_STR
#	endif

#endif // VER_PRODUCTVERSION_MAJOR

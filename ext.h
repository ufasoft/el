/*######   Copyright (c) 2013-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#ifndef _LIBEXT_EXT_H
#define _LIBEXT_EXT_H

#ifdef _MSC_VER
#	pragma once
#endif

//#pragma warning(disable: 4786)  //!!! name truncated

#include <el/libext.h>

#ifdef WIN32
#	include <windows.h>
#endif

#if UCFG_USING_DEFAULT_NS
using namespace std;
using namespace Ext;

using _TR1_NAME(hash);

#endif

//!!!R #define _POST_CONFIG

#endif // _LIBEXT_EXT_H

#if !defined(VER_H) && UCFG_WIN32
#	ifdef RC_INVOKED
#		include <winver.h>
#	else
#		define RC_INVOKED
#			include <winver.h>
#		undef RC_INVOKED	
#	endif	
#	undef VER_H
#endif	



//!!!R #define NONLS

//#define _SLIST_HEADER_

//!!!typedef void *PSLIST_ENTRY;
//!!!typedef void *PSLIST_HEADER;
//!!!typedef void *SLIST_HEADER;


#include <el/inc/inc_configs.h>

#ifndef UCFG_MANUFACTURER
#	define UCFG_MANUFACTURER "Ufasoft"
#endif

#ifndef UCFG_MANUFACTURER_DOMAIN
#	define UCFG_MANUFACTURER_DOMAIN 		"ufasoft.com"
#endif

#define UCFG_MANUFACTURER_HTTP_URL 		"http://" UCFG_MANUFACTURER_DOMAIN

#ifndef UCFG_MANUFACTURER_NEWS_URL
#	define UCFG_MANUFACTURER_NEWS_URL 		UCFG_MANUFACTURER_HTTP_URL "/forum/"
#endif

#ifndef UCFG_MANUFACTURER_EMAIL
#	define UCFG_MANUFACTURER_EMAIL "support@" UCFG_MANUFACTURER_DOMAIN
#endif

#define UCFG_MANUFACTURER_NOTIFY_URL 	UCFG_MANUFACTURER_HTTP_URL "/cgi-bin/notify.cgi"
#define UCFG_MANUFACTURER_CRASHDUMP_URL UCFG_MANUFACTURER_HTTP_URL "/cgi-bin/crashdump.cgi"


//!!!R #include <manufacturer.h>



#ifdef _POST_CONFIG
	__BEGIN_DECLS
			WINBASEAPI LCID WINAPI GetUserDefaultLCID(void);
	__END_DECLS
#endif


#define VER_FILEFLAGSMASK			VS_FFI_FILEFLAGSMASK
#define VER_FILEFLAGS				0

#ifdef WIN32
	#define VER_FILEOS          		VOS__WINDOWS32
	#ifdef _USRDLL
		#define VER_FILETYPE VFT_DLL
	#else
		#define VER_FILETYPE VFT_APP
	#endif
	#define VER_FILESUBTYPE	VFT2_UNKNOWN
#else
	#define VER_FILEOS          		VOS_NT_WINDOWS32
	#define VER_FILETYPE				VFT_DRV
	#define VER_FILESUBTYPE	VFT2_DRV_SYSTEM
#endif

#ifndef VER_COMPANYNAME_STR
	#define VER_COMPANYNAME_STR UCFG_MANUFACTURER
#endif	

#ifndef VER_LEGALCOPYRIGHT_YEARS
#	define VER_LEGALCOPYRIGHT_YEARS "1997-2015"
#endif

#ifndef VER_LEGALCOPYRIGHT_STR
#	define VER_LEGALCOPYRIGHT_STR "Copyright (c) " VER_LEGALCOPYRIGHT_YEARS " " VER_COMPANYNAME_STR
#endif


#ifndef VER_PRODUCTNAME_STR
	#define VER_PRODUCTNAME_STR "ExtLIBs"
#endif






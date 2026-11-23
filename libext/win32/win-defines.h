#pragma once

#ifdef UNICODE
#	define EXT_WINAPI_WA_NAME(name) (#name "W")
#else
#	define EXT_WINAPI_WA_NAME(name) (#name "A")
#endif

#ifdef UNICODE
#	define GetTempPath  			GetTempPathW
#	define GetTempFileName  		GetTempFileNameW
#	define CreateDirectory  		CreateDirectoryW
#	define GetMessage  				GetMessageW
#	define SetEnvironmentVariable  	SetEnvironmentVariableW
#	define GetCurrentDirectory  	GetCurrentDirectoryW
#	define SetCurrentDirectory  	SetCurrentDirectoryW
#	define LoadIcon  				LoadIconW
#	define GetProfileString  		GetProfileStringW
#	define WriteProfileString  		WriteProfileStringW
#	define GetMessage  				GetMessageW
#	define PostMessage  			PostMessageW
#	define GetEnvironmentVariable  	GetEnvironmentVariableW
#	define QueryDosDevice  			QueryDosDeviceW
#else
#	define CreateDirectory  		CreateDirectoryA
#	define GetMessage  				GetMessageA
#	define SetEnvironmentVariable  	SetEnvironmentVariableA
#	define GetCurrentDirectory  	GetCurrentDirectoryA
#	define SetCurrentDirectory  	SetCurrentDirectoryA
#	define LoadIcon  				LoadIconA
#	define GetProfileString  		GetProfileStringA
#	define WriteProfileString  		WriteProfileStringA
#	define GetMessage  				GetMessageA
#	define PostMessage  			PostMessageA
#	define GetEnvironmentVariable  	GetEnvironmentVariableA
#endif // UNICODE


#define SFGAO_UNDOCUMENTED_80 0x00000080L

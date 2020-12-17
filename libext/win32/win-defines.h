/*######     Copyright (c) 1997-2013 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com #######################################
#                                                                                                                                                                          #
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation;  #
# either version 3, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the      #
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU #
# General Public License along with this program; If not, see <http://www.gnu.org/licenses/>                                                                               #
##########################################################################################################################################################################*/

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



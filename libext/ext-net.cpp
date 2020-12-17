/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <winsock2.h>
#	include <wininet.h>
#endif

#include "ext-net.h"

namespace Ext {
using namespace std;

DECLSPEC_NORETURN void AFXAPI ThrowWSALastError() {
#ifdef WIN32
	if (DWORD dw = WSAGetLastError())
		Throw(error_code((int)dw, system_category()));
	else
		Throw(ExtErr::UnknownSocketsError);
#else
	CCheck(-1);
	Throw(E_FAIL);
#endif
}

int AFXAPI SocketCheck(int code) {
	if (code == SOCKET_ERROR)
		ThrowWSALastError();
	return code;
}

void AFXAPI SocketCodeCheck(int code) {
	if (code)
		Throw(error_code(code, system_category()));
}





} // Ext::

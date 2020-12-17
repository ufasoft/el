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

#if UCFG_USE_REGEX

#if UCFG_WIN32
	static StaticWRegex
#else
	static StaticRegex
#endif
s_reUrlLoginPassword("^([A-Za-z][-+.A-Za-z0-9]*://)(.+)(@[^@]+)$"),
s_reUrl("^(?:([A-Za-z][-+.A-Za-z0-9]*)"		// scheme - 1
				 "(?:\\:\\/\\/)"
		 ")"
		 "(?:([\\w.]+)\\:([\\w.]+)(?:\\@))?"		// username - 2, // password - 3
		 "([^/\\r\\n\\:]+)?"		// domain - 4
		 "(\\:\\d+)?"				// port - 5
		 "([^?]*)?"			// path - 6
		 "(\\??" // trigraph
		 "(?:\\w+\\=[^\\#]+)(?:\\&?\\w+\\=\\w+)*)*"  // qrystr - 7
		 "(\\#.*)?$");		// bkmrk 8


Uri::~Uri() { // has preference when elst.lib linked
}

void Uri::EnsureAnalyzed() {
	if (!m_bAnalyzed) {
		String uri = m_uriString;

#if UCFG_WIN32
		Smatch m;
		if (regex_search(uri, m, *s_reUrlLoginPassword)) {
			String logpass = m[2];
			logpass.Replace("@", "%40");		
			uri = String(m[1]) + logpass + String(m[3]);
		}

		TCHAR scheme[256], host[256], username[256], password[256], path[1024], extra[1024];
		URL_COMPONENTS uc = { sizeof uc };

		uc.lpszScheme = scheme;
		uc.dwSchemeLength = size(scheme);
	
		uc.lpszHostName = host;
		uc.dwHostNameLength = size(host);
		
		uc.lpszUserName = username;
		uc.dwUserNameLength = size(username);

		uc.lpszPassword = password;
		uc.dwPasswordLength = size(password);

		uc.lpszUrlPath = path;
		uc.dwUrlPathLength = size(path);
		
		uc.lpszExtraInfo = extra;	
		uc.dwExtraInfoLength = size(extra);

		Win32Check(::InternetCrackUrl(uri, 0, ICU_ESCAPE, &uc));

		m_scheme = String(scheme).ToLower();
		m_host = host;
		m_username = username;
		m_password = password;
		m_path = path;
		m_extra = extra;
		if (0 == (m_port = uc.nPort))
			m_port = -1;
#else
		cmatch m;
		if (!regex_search(uri.c_str(), m, s_reUrl))
			Throw(E_FAIL);
		m_scheme = String(m[1]).ToLower();
		m_username = m[2];
		m_password = m[3];
		m_host =  m[4];
		m_path = m[6];
		m_extra = m[7];
		if (m[5].matched)
			m_port = Convert::ToUInt16(String(m[5]).substr(1));
		else
			m_port = -1;
		
#endif
		if (-1 == m_port) {
			if (m_scheme == "http")
				m_port = 80;
			else if (m_scheme == "https")
				m_port = 443;
			else if (m_scheme == "ftp")
				m_port = 21;
			else if (m_scheme == "mailto")
				m_port = 25;
			else if (servent *se = ::getservbyname(m_scheme, 0))
				m_port = ntohs(se->s_port);
		}
		m_bAnalyzed = true;
	}
}

#endif // UCFG_USE_REGEX


String Uri::UnescapeDataString(RCString s) {
#if UCFG_EXTENDED //!!!D
	int len = s.length() + 3;
	TCHAR *path = (TCHAR*)alloca(len*sizeof(TCHAR)),
		*query = (TCHAR*)alloca(len*sizeof(TCHAR));

	URL_COMPONENTS uc = { sizeof uc };
	uc.lpszUrlPath = path;
	uc.dwUrlPathLength = len;
	uc.lpszExtraInfo = query;
	uc.dwExtraInfoLength = len;
	Win32Check(::InternetCrackUrl("http://host/q?"+s, 0, ICU_ESCAPE, &uc));
	return String(query).substr(1);
#else
	class CUriEscape : public CEscape {
		void EscapeChar(ostream& os, char ch) {
			if (strchr("%;/?:@&=$,", ch))
				os << "%" << setw(2) << setfill('0') << hex << (int)ch;
			else
				os.put(ch);
		}

		int UnescapeChar(istream& is) {
			int ch = is.get();
			if (ch == '%') {
				int c1 = is.get();
				int c2 = is.get();
				if (c2 == EOF)
					return EOF;
				return (int)Convert::ToUInt32(String((char)c1)+String((char)c2), 16);
			} else
				return ch;
		}

	} esc;

	return CEscape::Unescape(esc, s);
#endif
}



} // Ext::



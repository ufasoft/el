/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <el/inet/http.h>


namespace Ext { namespace Inet {

int AFXAPI InetCheck(int i) {
	if (!i) {
		DWORD dw = GetLastError();
		if (dw == ERROR_INTERNET_EXTENDED_ERROR) {
			TCHAR buf[256];
			DWORD code,
				len = size(buf);
			Win32Check(::InternetGetLastResponseInfo(&code, buf, &len));
			code = code;
			//!!!! todo
		}
		if (dw)
			Throw(HRESULT_FROM_WIN32(dw));
		else
			Throw(ExtErr::UnknownWin32Error);
	}
	return i;
}

#if !UCFG_USE_LIBCURL

void CInternetFile::Attach(HINTERNET hInternet) {
	m_hInternet = hInternet;
}

CInternetFile::~CInternetFile() {
}

uint32_t CInternetFile::Read(void *lpBuf, size_t nCount, int64_t offset) {
	ASSERT(offset == CURRENT_OFFSET);
	DWORD dw;
	Win32Check(InternetReadFile(m_hInternet, lpBuf, nCount, &dw));
	return dw;
}

const int INTERNET_BUF_SIZE = 8192;

String CInternetFile::ReadString() {
	Blob buf(0, INTERNET_BUF_SIZE);
	DWORD dw;
	Win32Check(::InternetReadFile(m_hInternet, buf.data(), buf.size(), &dw));
	return String((const char*)buf.data(), dw);
}

DWORD CInternetFile::SetFilePointer(LONG offset, DWORD method) {
	return InternetSetFilePointer(m_hInternet, offset, 0, method, 0);
}

CInternetSession::CInternetSession(RCString agent, DWORD dwContext, DWORD dwAccessType, RCString pstrProxyName, RCString pstrProxyBypass, DWORD dwFlags) {
	Init(agent, dwContext, dwAccessType, pstrProxyName, pstrProxyBypass, dwFlags);
}

void CInternetSession::Init(RCString agent, DWORD dwContext, DWORD dwAccessType, RCString pstrProxyName, RCString pstrProxyBypass, DWORD dwFlags) {
	Attach(::InternetOpen(agent, dwAccessType, pstrProxyName, pstrProxyBypass, dwFlags));
//!!!	Win32Check(m_hSession != 0);
}

CInternetSession::~CInternetSession() {
	try {
		Close();
	} catch (RCExc) {
	}
}

/*!!!
void CInternetSession::Close() {
	if (m_hSession)
		Win32Check(::InternetCloseHandle(exchangeZero(m_hSession)));
}*/

void CInternetSession::ReleaseHandle(intptr_t h) const {
	Win32Check(::InternetCloseHandle((HINTERNET)h));
}

void CInternetSession::OpenUrl(CInternetConnection& conn, RCString url, LPCTSTR headers, DWORD headersLength, DWORD dwFlags, DWORD_PTR ctx) {
	conn.Attach(::InternetOpenUrl((HINTERNET)(intptr_t)BlockingHandleAccess(_self), url, headers, headersLength, dwFlags, ctx));
}

void CInternetSession::Connect(CInternetConnection& conn, RCString serverName, INTERNET_PORT port, RCString userName, RCString password, DWORD dwService, DWORD dwFlags, DWORD_PTR ctx) {
	conn.Attach(::InternetConnect((HINTERNET)(intptr_t)BlockingHandleAccess(_self), serverName, port, userName, password, dwService, dwFlags, ctx));
}

#endif // !UCFG_USE_LIBCURL


/*!!!R
int CInetStreambuf::overflow(int c) {
	return EOF;
}

int CInetStreambuf::uflow() {
	char c;
	int r = m_file.Read(&c, 1);
	return r > 0 ? c : EOF;
}

int CInetStreambuf::underflow() {
	int c = uflow();
	if (c != EOF)
		m_file.SetFilePointer(-1, FILE_CURRENT);
	return c;
}

void CInetIStream::open(CInternetSession& sess, RCString url) {
	sess.OpenURL(url, &((CInetStreambuf*)rdbuf())->m_file);
}*/

#if UCFG_WIN32
bool AFXAPI ConnectedToInternet() {
	DWORD dwState = 0,
		dwSize = sizeof(DWORD);
	Win32Check(::InternetQueryOption(0, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize));
	return !(dwState & INTERNET_STATE_DISCONNECTED_BY_USER);
}
#endif //UCFG_WIN32



}} // Ext::Inet::


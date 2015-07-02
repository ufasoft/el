/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_USE_LIBCURL
#	include <curl/curl.h>
#	pragma comment(lib, "curl")
#elif UCFG_WIN32
#	pragma comment(lib, "wininet")
#endif

#include "ext-http.h"

namespace Ext {

using namespace std;

static class HttpCategory : public ErrorCategoryBase {
	typedef ErrorCategoryBase base;
public:
	HttpCategory()
		:	base("HTTP", FACILITY_HTTP)
	{}

	string message(int errval) const override {
#if UCFG_WIN32
		return HResultToMessage(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, errval));
#else
		switch ((http_error)errval) {
		case http_error::unauthorized: return "Unauthorized";
		default:
			return "Unknown";
		}
#endif
	}
} s_http_category;

const error_category& http_category() {
	return s_http_category;
}



static StaticRegex s_reCharset("charset=([^()<>@,;:\\\\\"/\\[\\]?={} \t]+)");

Encoding *CHttpHeader::GetEncoding() {
	Encoding *enc = &Encoding::UTF8;
	String ct = Headers.Get("Content-Type");
	cmatch m;
	if (!!ct && regex_search(ct.c_str(), m, *s_reCharset)) {
		try {
			enc = Encoding::GetEncoding(String(m[1]));
		} catch (RCExc) {
		}
	}
	return enc;
}


#if UCFG_USE_LIBCURL


static class curl_error_category : public error_category {
	typedef error_category base;

	const char *name() const noexcept override { return "cURL"; }
	
	string message(int errval) const override {
	     return ::curl_easy_strerror((CURLcode)errval);
	}

	bool equivalent(int errval, const error_condition& c) const noexcept override {			//!!!TODO
		switch (errval) {
		case CURLE_COULDNT_CONNECT:		return c==errc::connection_refused;
		case CURLE_OPERATION_TIMEDOUT:	return c==errc::timed_out;
		case CURLE_REMOTE_ACCESS_DENIED:return c==errc::permission_denied;
		case CURLE_OUT_OF_MEMORY:		return c==not_enough_memory;
		default:
			return base::equivalent(errval, c);
		}
	}
} s_curlErrorCategory;

const error_category& curl_category() {
	return s_curlErrorCategory;
}

void CurlCheck(CURLcode rc, CurlSession *sess) {
	if (rc != CURLE_OK) {
		if (sess) {
			TRC(1, "CURLcode: " << rc << "  " << sess->m_errbuf);
			throw system_error(rc, curl_category(), sess->m_errbuf);
		} else
			Throw(error_code(rc, curl_category()));
	}
}


CurlSession::~CurlSession() {
	if (m_h) {
//!!!		SetNullFunctions();
		::curl_easy_cleanup(m_h);
	}
}


void CurlSession::SetOption(CURLoption option, long v) {
	CurlCheck(::curl_easy_setopt(m_h, option, v), this);
}

void CurlSession::SetOption(CURLoption option, const void *p) {
	CurlCheck(::curl_easy_setopt(m_h, option, p), this);
}

void CurlSession::SetNullFunctions() {
	Reset();
/*!!!
	if (m_h) {
		SetOption(CURLOPT_WRITEFUNCTION, 0);
		SetOption(CURLOPT_WRITEDATA, 0);
		SetOption(CURLOPT_READFUNCTION, 0);
		SetOption(CURLOPT_READDATA, 0);
		SetOption(CURLOPT_HEADERFUNCTION, 0);
		SetOption(CURLOPT_HEADERDATA, 0);
	}*/
}

static class CCurlManager {
public:
	static const int MAX_SESSIONS = 16;

	mutex Mtx;
	typedef list<ptr<CurlSession>> CSessions;
	CSessions Sessions;

	CCurlManager() {
		CurlCheck(::curl_global_init(0));
	}

	~CCurlManager() {
		::curl_global_cleanup();
	}

	ptr<CurlSession> GetSession(const IPEndPoint& ep) {
		EXT_LOCK (Mtx) {
			for (CSessions::iterator it=Sessions.begin(), e=Sessions.end(); it!=e; ++it) {
				if (ep == (*it)->EndPoint) {
					ptr<CurlSession> r = *it;
					Sessions.erase(it);
					return r;
				}
			}
		}
		return new CurlSession(ep);
	}

	void ReturnSession(ptr<CurlSession> p) {
		EXT_LOCK (Mtx) {
			Sessions.push_front(p);
			if (Sessions.size() >= MAX_SESSIONS)
				Sessions.pop_back();	
		}
	}
} g_curlManager;


size_t InetStream::WriteFunction(void *ptr, size_t size, size_t nmemb, void *stream) {
	InetStream& stm = *(InetStream*)stream;
	size_t n = size*nmemb;
	stm.ResultStream.WriteBuffer(ptr, n);

		/*!!!bug
	if (stm.m_io.ReceivedSize)
		return CURL_WRITEFUNC_PAUSE;
	memcpy(stm.m_io.RecvBuf, ptr, stm.m_io.ReceivedSize = n);
	if (stm.m_io.UserReadBuf)
		stm.m_io.ReadToUserBuf(stm.m_io.UserReadBuf, stm.m_io.UserReadSize); */
	return n;
}

size_t InetStream::ReadFunction(void *ptr, size_t size, size_t nmemb, void *stream) {
	InetStream& stm = *(InetStream*)stream;
	if (!stm.m_io.UserWriteBuf)
		return 0; //EOF CURL_READFUNC_PAUSE;
	size_t n = (std::min)(stm.m_io.UserWriteSize, size*nmemb);
	memcpy(ptr, stm.m_io.UserWriteBuf, n);
	if (stm.m_io.UserWriteSize -= n)
		stm.m_io.UserWriteBuf += n;
	else
		stm.m_io.UserWriteBuf = 0;
	return n;
}

static StaticRegex s_reStatus("HTTP/[^ ]+ \\d+\\s+(.*)");

size_t InetStream::HeaderFunction(void *ptr, size_t size, size_t nmemb, void *stream) {
	InetStream& stm = *(InetStream*)stream;
	size_t n = size*nmemb;
	String line = String((const char*)ptr, n).Trim();
	vector<String> v = line.Split(":", 2);
	HttpWebResponse::Impl *respImpl = (HttpWebResponse::Impl*)stm.m_pResponseImpl;
	if (v.size() >= 2) {
		String name = v[0].Trim(),
				val = v[1].Trim();
		respImpl->m_headers.Set(name, val);
	} else {
		cmatch m;
		if (regex_match(line.c_str(), m, *s_reStatus))
			respImpl->m_statusDesc = m[1];
	}
	return n;
}

void InetStream::Attach(ptr<CurlSession> curl) {
	m_curl = curl;
	m_curl->SetOption(CURLOPT_WRITEFUNCTION, (const void*)&WriteFunction);
	m_curl->SetOption(CURLOPT_WRITEDATA, this);
	m_curl->SetOption(CURLOPT_READFUNCTION, (const void*)&ReadFunction);
	m_curl->SetOption(CURLOPT_READDATA, this);
	m_curl->SetOption(CURLOPT_HEADERFUNCTION, (const void*)&HeaderFunction);
	m_curl->SetOption(CURLOPT_HEADERDATA, this);
}

#endif // UCFG_USE_LIBCURL

size_t InetStream::Read(void *buf, size_t size) const {
#if UCFG_USE_LIBCURL
	if (!m_io.ReadToUserBuf((byte*)buf, size)) {
		CurlCheck(::curl_easy_pause(m_curl, CURLPAUSE_SEND), m_curl);			//!!!bug  can be called only from callback
	}
	return m_io.ReceivedSize;	//!!!?
#else
	DWORD dw;
	bool b = ::InternetReadFile((HINTERNET)(intptr_t)BlockingHandle(*m_pConnection), buf, size, &dw);
	Win32Check(b);
	return dw;
#endif
}

int InetStream::ReadByte() const {
	byte by;
#if UCFG_USE_LIBCURL
	if (!m_io.ReadToUserBuf((byte*)&by, 1)) {
		CurlCheck(::curl_easy_pause(m_curl, CURLPAUSE_SEND), m_curl);			//!!!bug  can be called only from callback
	}
	if (m_io.UserReadSize)
		return -1;
	return by;
#else
	DWORD dw;
	Win32Check(::InternetReadFile((HINTERNET)(intptr_t)BlockingHandle(*m_pConnection), &by, 1, &dw));
	return dw ? by : -1;
#endif
}

void InetStream::WriteBuffer(const void *buf, size_t count) {
#if UCFG_USE_LIBCURL
	m_io.UserWriteBuf = (const byte *)buf;
	m_io.UserWriteSize = count;
	CurlCheck(::curl_easy_pause(m_curl, CURLPAUSE_RECV), m_curl);
#else
	DWORD dw;
	Win32Check(::InternetWriteFile((HINTERNET)(intptr_t)BlockingHandle(*m_pConnection), buf, count, &dw));
	if (dw != count)
		Throw(E_FAIL);
#endif
}


#if !UCFG_USE_LIBCURL

CInternetSession& WebRequest::Session() {
	if (!m_pimpl->m_sess) {
		DWORD accessType = INTERNET_OPEN_TYPE_PRECONFIG;
		String proxyName = nullptr;
		if (!Proxy)
			accessType = INTERNET_OPEN_TYPE_DIRECT;
		else if (Proxy->Type != ProxyType::Default) {
			accessType = INTERNET_OPEN_TYPE_PROXY;
			proxyName = Proxy->Address.Host+":"+Convert::ToString(Proxy->Address.Port);
			switch (Proxy->Type) {
			case ProxyType::Http: proxyName = "http="+proxyName; break;
			case ProxyType::Socks: proxyName = "socks="+proxyName; break;
			default:
				Throw(E_FAIL);
			}
		}
		m_pimpl->m_sess.Init("", 1,  accessType, proxyName);
	}
	return m_pimpl->m_sess;
}


#endif // !UCFG_USE_LIBCURL

CInternetConnection::~CInternetConnection() {
#if UCFG_USE_LIBCURL
	if (Session) {
		Session->SetNullFunctions();
		g_curlManager.ReturnSession(Session);
	}
#else
	Close();
#endif
}

#if !UCFG_USE_LIBCURL

void CInternetConnection::ReleaseHandle(intptr_t h) const {
	Win32Check(::InternetCloseHandle((HINTERNET)h));
}

void CInternetConnection::HttpOpen(CHttpInternetConnection& respConn, RCString verb, RCString objectName, RCString version, RCString referer, LPCTSTR* lplpszAcceptTypes, DWORD dwFlags, DWORD_PTR ctx) {
	respConn.Attach(::HttpOpenRequest((HINTERNET)(intptr_t)BlockingHandle(_self), verb, objectName, version, referer, lplpszAcceptTypes, dwFlags, ctx));
}

void CInternetConnection::AddHeaders(const WebHeaderCollection& headers, DWORD dwModifiers) {
	if (!headers.empty())
		Win32Check(::HttpAddRequestHeaders((HINTERNET)(intptr_t)Handle(_self), headers.ToString(), DWORD(-1), dwModifiers));
}

#endif // !UCFG_USE_LIBCURL


#if UCFG_USE_LIBCURL

ptr<CurlSession> WebRequest::Connect() {
#else
void WebRequest::Connect() {
#endif
	String userName = Credentials.UserName, 
		password = Credentials.Password;
#if UCFG_USE_LIBCURL
	ptr<CurlSession> curl = g_curlManager.GetSession(IPEndPoint(RequestUri.Host, (uint16_t)RequestUri.Port));

	if (!!userName || !!password) {
//		CurlCheck(::curl_easy_setopt(curl->m_h, CURLOPT_CONNECT_ONLY, long(1)), curl);
		CurlCheck(::curl_easy_setopt(curl->m_h, CURLOPT_HTTPAUTH, CURLAUTH_BASIC), curl);
		if (!!userName)
			curl->SetOption(CURLOPT_USERNAME, (const char*)userName);
		if (!!password)
			curl->SetOption(CURLOPT_PASSWORD, (const char*)password);
	}
	
	if (m_timeout > 0)
		CurlCheck(::curl_easy_setopt(curl->m_h, CURLOPT_TIMEOUT_MS, (long)m_timeout), curl);
	
	CURLoption opt = (CURLoption)0;
	if (Method == "GET") {
		opt = CURLOPT_HTTPGET;
	} else if (Method == "POST")
		opt = CURLOPT_POST;
	else if (Method == "PUT")
		opt = CURLOPT_UPLOAD;
	else {
		curl->SetOption(CURLOPT_CUSTOMREQUEST, (const char*)Method);
	}
	if (!!opt)
		CurlCheck(::curl_easy_setopt(curl->m_h, opt, 1), curl);

	curl->SetOption(CURLOPT_URL, (const char*)RequestUri.get_OriginalString());
//!!!	CurlCheck(::curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, long(1)), curl);

	return curl;
#else
	if (!userName) {
		userName = RequestUri.UserName;
		password = RequestUri.Password;	
	}
	Session().Connect(m_pimpl->m_conn, RequestUri.Host, (INTERNET_PORT)RequestUri.Port, userName, password, m_serviceType);
#endif
}

HttpWebRequest::HttpWebRequest(RCString url)
	:	ContentLength(-1)
	,	KeepAlive(true)
	,	ContentType(nullptr)
	,	Referer(nullptr)
	,	CacheLevel(RequestCacheLevel::Default)
//!!!	:	m_url(url)
{
#if !UCFG_USE_LIBCURL
	m_serviceType = INTERNET_SERVICE_HTTP;
#endif	
	Method = "GET";
	if (!!url) {
		RequestUri = url;
		SetPimpl(m_pimpl = new Impl);
	}
}

HttpWebRequest::~HttpWebRequest() {
#if UCFG_USE_LIBCURL
	::curl_slist_free_all(m_headers);
#endif
}

DWORD HttpWebRequest::get_Timeout() {
#if UCFG_USE_LIBCURL
	return m_timeout;
#else
	DWORD r, qlen = sizeof r;
	Win32Check(::InternetQueryOption((HINTERNET)(intptr_t)Handle(Session()), INTERNET_OPTION_RECEIVE_TIMEOUT, &r, &qlen));
	return r;
#endif
}

void HttpWebRequest::put_Timeout(DWORD v) {
#if UCFG_USE_LIBCURL
	m_timeout = v;
#else
	Win32Check(::InternetSetOption((HINTERNET)(intptr_t)Handle(Session()), INTERNET_OPTION_RECEIVE_TIMEOUT, &v, sizeof v));
#endif
}

String HttpWebRequest::get_UserAgent() {
#if UCFG_USE_LIBCURL
	return m_userAgent;
#else
	DWORD qlen = 0;
	Win32Check(::InternetQueryOption((HINTERNET)(intptr_t)Handle(Session()), INTERNET_OPTION_USER_AGENT, 0, &qlen),  ERROR_INSUFFICIENT_BUFFER);
	TCHAR *p = (TCHAR*)alloca(qlen);
	Win32Check(::InternetQueryOption((HINTERNET)(intptr_t)Handle(Session()), INTERNET_OPTION_USER_AGENT, p, &qlen));
	return String(p, qlen/sizeof(TCHAR));
#endif
}

void HttpWebRequest::put_UserAgent(RCString v) {
#if UCFG_USE_LIBCURL
	m_userAgent = v;
#else
	Win32Check(::InternetSetOption((HINTERNET)(intptr_t)Handle(Session()), INTERNET_OPTION_USER_AGENT, (void*)(const TCHAR*)v, v.length()));	// len in TCHARs for strings
#endif
}

#if 0 //!!!
ptr<WebProxy> HttpWebRequest::get_Proxy() {	
	return m_proxy;
/*!!!
#if UCFG_USE_LIBCURL
	Throw(E_NOTIMPL);
#else
	DWORD qlen = 0;	
	Win32Check(::InternetQueryOption(m_sess, INTERNET_OPTION_PROXY, 0, &qlen),  ERROR_INSUFFICIENT_BUFFER);
	INTERNET_PROXY_INFO *pi = (INTERNET_PROXY_INFO*)alloca(qlen);
	Win32Check(::InternetQueryOption(m_sess, INTERNET_OPTION_PROXY, pi, &qlen));
	WebProxy r;
	String s = pi->lpszProxy;
	if (s.Left(5) == "http=")
		r.Address = s.Mid(5);
	return r;
#endif*/
}

void HttpWebRequest::put_Proxy(WebProxy *proxy) {
	m_proxy = proxy;
#if UCFG_USE_LIBCURL
	Throw(E_NOTIMPL);
#else
//	String httpProxy = "http="+proxy->Address.Host+":"+Convert::ToString(proxy->Address.Port);
//	INTERNET_PROXY_INFO pi = { INTERNET_OPEN_TYPE_PROXY , httpProxy };
//	Win32Check(::InternetSetOption(m_sess, INTERNET_OPTION_PROXY , &pi, sizeof pi));
#endif
}
#endif

bool HttpWebRequest::EndRequest() {
#if UCFG_USE_LIBCURL
	return true;
#else
	if (!m_bSendEx)
		Throw(E_FAIL);
	int r = Win32Check(::HttpEndRequest((HINTERNET)(intptr_t)CInternetConnection::BlockingHandleAccess(m_response.m_pImpl->m_conn), 0, 0, 0), ERROR_INTERNET_FORCE_RETRY);
	return r;
#endif
}

void HttpWebRequest::ReleaseFromAPC() {
#if !UCFG_USE_LIBCURL
	if (SafeHandle::HandleAccess *ha = (SafeHandle::HandleAccess*)(void*)SafeHandle::t_pCurrentHandle)
		ha->Release();
	if (m_response.m_pImpl) {
		m_response.m_pImpl->m_conn.Close();
//!!!		m_pResponse->m_pImpl->m_req.m_pimpl->m_sess.Close();
	}
#endif
}

#if UCFG_USE_LIBCURL

void HttpWebRequest::AddHeader(CURL *curl, RCString name, RCString val) {
	m_headers.reset(::curl_slist_append(m_headers, name+": "+val));
}

ptr<CurlSession> HttpWebRequest::Connect() {
	ptr<CurlSession> curl = base::Connect();
	if (!m_userAgent.empty())
		curl->SetOption(CURLOPT_USERAGENT, (const char*)m_userAgent);

	if (Method=="POST" || Method=="PUT") {
		if (ContentLength != -1) {
			CurlCheck(::curl_easy_setopt(curl->m_h, CURLOPT_POSTFIELDSIZE, (int)ContentLength), curl);
//			AddHeader(curl->m_h, "Content-Length", Convert::ToString(ContentLength));
		}
	}
	if (CacheLevel == RequestCacheLevel::BypassCache)
		AddHeader(curl, "Cache-Control", "no-cache");
	for (auto it=Headers.begin(), e=Headers.end(); it!=e; ++it)
		AddHeader(curl, it->first, Headers.Get(it->first));
	return curl;
}
#endif

String HttpWebRequest::GetHeadersString() {
	if (!KeepAlive)
		Headers.Set("Connection", "close");
	String sHeaders;
	for (WebHeaderCollection::iterator i=Headers.begin(), e=Headers.end(); i!=e; ++i)
		sHeaders += i->first+": " + String::Join(";", i->second) + "\r\n";
	return sHeaders;
}

void HttpWebRequest::Send(const byte *p, size_t size) {
#if !UCFG_USE_LIBCURL
	m_bSendEx = true;

	INTERNET_BUFFERS ib = { sizeof ib },
		             *pib = 0;
	ib.lpvBuffer = (void*)p;
	ib.dwBufferLength = size;
	ib.dwBufferTotal = size;

	String sHeaders = GetHeadersString();
	if (!sHeaders.empty()) {
		pib = &ib;	
		ib.lpcszHeader = sHeaders;
		ib.dwHeadersLength = sHeaders.length();
	}

	if (p)
		pib = &ib;	
	Win32Check(::HttpSendRequestEx((HINTERNET)(intptr_t)SafeHandle::HandleAccess(m_response.m_pImpl->m_conn), pib, 0, 0, 0));
#endif
}

HttpWebResponse HttpWebRequest::GetResponse(const byte *p, size_t size) {
	if (!m_response.m_pImpl) {
#if UCFG_USE_LIBCURL
		ptr<CurlSession> curl = Connect();
		m_response = HttpWebResponse(_self);
		m_response.Attach(curl);

		CurlCheck(::curl_easy_setopt(curl->m_h, CURLOPT_HTTPHEADER, m_headers.get()), curl);
#else
/*!!!R		if (Method == "GET") {
			String sHeaders = GetHeadersString();
			m_pResponse = new HttpWebResponse(_self);

			DWORD dwFlags = 0;
			Session().OpenUrl(m_pResponse->m_pImpl->m_conn, RequestUri.OriginalString, sHeaders, DWORD(-1), dwFlags);
			m_pResponse->m_pImpl->m_stm.m_pConnection = &m_pResponse->m_pImpl->m_conn;
		} else */
		{
			Connect();
			m_response = HttpWebResponse(_self);

			String userName = Credentials.UserName, 
				password = Credentials.Password;
			if (!userName) {
				userName = RequestUri.UserName;
				password = RequestUri.Password;
			}
			if (!userName.empty()) {
				String s = userName + ":";
				if (!password.empty())
					s += password;
				const char *psz = s;
				Headers.Set("Authorization", "Basic "+Convert::ToBase64String(ConstBuf(psz, strlen(psz))));
			}

			DWORD flags = INTERNET_FLAG_NO_CACHE_WRITE;		// tp prevent ERROR_WINHTTP_OPERATION_CANCELLED
			if (RequestUri.Scheme == "https")
				flags |= INTERNET_FLAG_SECURE;
			if (Method == "GET") {
				if (CacheLevel == RequestCacheLevel::BypassCache)
					flags |= INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD;
			} else if (Method=="POST" || Method=="PUT") {
				flags |= INTERNET_FLAG_NO_CACHE_WRITE;		// to prevent ERROR_INTERNET_INCORRECT_HANDLE_STATE [KB177190]
				if (ContentLength != -1)
					Headers.Set("Content-Length", Convert::ToString(ContentLength));
				if (!!ContentType)
					Headers.Set("Content-Type", ContentType);

				/*!!!R
				if (ContentLength != -1) {
					WebHeaderCollection hh;
					hh.Set("Content-Length", Convert::ToString(ContentLength));
					m_pResponse->m_pImpl->m_conn.AddHeaders(hh);
				}
				if (!!ContentType) {
					WebHeaderCollection hh;
					hh.Set("Content-Type", ContentType);
					m_pResponse->m_pImpl->m_conn.AddHeaders(hh);
				}*/
			}	
			m_pimpl->m_conn.HttpOpen(m_response.m_pImpl->m_conn, Method, RequestUri.PathAndQuery, nullptr, Referer, nullptr, flags);
			m_response.m_pImpl->m_conn.AddHeaders(AdditionalHeaders);
			m_response.m_pImpl->m_stm.m_pConnection.reset(&m_response.m_pImpl->m_conn);

			Send(p, size);
		}
#endif
		if (m_bSendEx) {
			InetStream& stm = m_response.m_pImpl->m_stm;
			while (true) {
#if UCFG_USE_LIBCURL
				if (size > 0)
					stm.WriteBuffer(p, size);
#endif
				if (EndRequest())
					break;
				Send(p, size);
			}
		}
	}
	return m_response;
}

Stream& HttpWebRequest::GetRequestStream(const byte *p, size_t size) {
	return HttpWebRequest::GetResponse(p, size).m_pImpl->m_stm;
}

HttpWebResponse::~HttpWebResponse() {
}

int HttpWebResponse::get_StatusCode() {
#if UCFG_USE_LIBCURL
	long r;
	CurlCheck(::curl_easy_getinfo(m_pImpl->m_conn.Session->m_h, CURLINFO_RESPONSE_CODE, &r), m_pImpl->m_conn.Session);
	return r;	
#else
	DWORD r, dw = sizeof r;
	Win32Check(::HttpQueryInfo((HINTERNET)(intptr_t)SafeHandle::HandleAccess(m_pImpl->m_conn), HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &r, &dw, 0));
	return r;
#endif
}

WebHeaderCollection HttpWebResponse::get_Headers() {
#if UCFG_USE_LIBCURL
	return m_pImpl->m_headers;
#else
	DWORD dw = 0;
	Win32Check(::HttpQueryInfo((HINTERNET)(intptr_t)SafeHandle::HandleAccess(m_pImpl->m_conn), HTTP_QUERY_RAW_HEADERS, 0, &dw, 0) || ::GetLastError()==ERROR_INSUFFICIENT_BUFFER);
	TCHAR *buf = (TCHAR*)alloca(dw);
	Win32Check(::HttpQueryInfo((HINTERNET)(intptr_t)SafeHandle::HandleAccess(m_pImpl->m_conn), HTTP_QUERY_RAW_HEADERS, buf, &dw, 0));
	WebHeaderCollection r;
	int i = 0;
	for (; *buf; ++i, buf+=_tcslen(buf)+1) {
		if (i) {		// first line is Status Code
			String h = buf;
			vector<String> pp = h.Split(":", 2);
			if (pp.size() < 2)
				Throw(E_FAIL);
			r.Set(pp[0].Trim(), pp[1].Trim());
		}
	}
	return r;
#endif
}

#if UCFG_USE_LIBCURL
int64_t HttpWebResponse::get_ContentLength() {
	double r;
	CurlCheck(::curl_easy_getinfo(m_pImpl->m_conn.Session->m_h, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &r), m_pImpl->m_conn.Session);
	return (int64_t)r;
}

String HttpWebResponse::get_StatusDescription() {
	return m_pImpl->m_statusDesc;
}

#endif // UCFG_USE_LIBCURL


String HttpWebResponse::GetString(DWORD dwInfoLevel) {
#if UCFG_USE_LIBCURL
	const char *p;
	CurlCheck(::curl_easy_getinfo(m_pImpl->m_conn.Session->m_h, CURLINFO(dwInfoLevel), &p), m_pImpl->m_conn.Session);
	return p;	
#else
	DWORD dw = 0;
	Win32Check(::HttpQueryInfo((HINTERNET)(intptr_t)SafeHandle::HandleAccess(m_pImpl->m_conn), dwInfoLevel, 0, &dw, 0) || ::GetLastError()==ERROR_INSUFFICIENT_BUFFER);
	TCHAR *buf = (TCHAR*)alloca(dw);
	Win32Check(::HttpQueryInfo((HINTERNET)(intptr_t)SafeHandle::HandleAccess(m_pImpl->m_conn), dwInfoLevel, buf, &dw, 0));
#	if UCFG_WCE
	return String(buf, dw);
#	else
	return String(buf, dw/sizeof(TCHAR));
#	endif
#endif
}

class WebRequestKeeper {
	HttpWebRequest*& m_cur;
	HttpWebRequest *m_prev;
public:
	WebRequestKeeper(HttpWebRequest*& cur, HttpWebRequest& req)
		:	m_cur(cur)
		,	m_prev(cur)
	{
		cur = &req;
	}

	~WebRequestKeeper() {
		m_cur = m_prev;
	}
};

WebClient::WebClient()
	:	UserAgent(nullptr)
	,	CurrentRequest(0)
	,	Encoding(&Encoding::Default())
	,	CacheLevel(RequestCacheLevel::Default)
	,	Proxy(new WebProxy(nullptr))
{}

InetStream& WebClient::DoRequest(HttpWebRequest& req, const ConstBuf data) {
	OnHttpWebRequest(req);
	WebRequestKeeper keeper(CurrentRequest, req);
	req.Credentials = Credentials;
	req.CacheLevel = CacheLevel;
	req.Proxy = Proxy;
	req.Headers = Headers;
	if (!!UserAgent)
		req.UserAgent = UserAgent;
	const byte *psz = 0;
	size_t len = 0;
	if (data.P) {
		psz = data.P;
		len = data.Size;
		req.ContentLength = len;
		
		if (data.Size > 2000000)
			req.Timeout = 100000;			// for large files
	}
	InetStream& stm = static_cast<InetStream&>(req.GetRequestStream(psz, len));

#if UCFG_USE_LIBCURL
	if (psz) {
		req.m_postData = data;
		stm.m_io.UserWriteBuf = req.m_postData.constData();
		stm.m_io.UserWriteSize = req.m_postData.Size;
	}	
	CurlCheck(::curl_easy_perform(req.m_response.m_pImpl->m_conn.Session->m_h), req.m_response.m_pImpl->m_conn.Session);
#endif

	m_response = req.GetResponse(psz, len);

	CHttpHeader hh;
	hh.Headers = ResponseHeaders;
	Encoding.reset(hh.GetEncoding());

	int statusCode = m_response.StatusCode;
	if (!(statusCode>=200 && statusCode<=299)) {
		WebException ex((http_error)statusCode, m_response.StatusDescription);
#if UCFG_USE_LIBCURL
		ex.Result = Encoding::UTF8.GetChars(ConstBuf(stm.ResultStream));
#else
		ex.Result = StreamReader(m_response.GetResponseStream()).ReadToEnd();
#endif	
//!!!?		ex.Response = m_response;
		throw ex;
	}
	return stm;
}

Stream& WebClient::OpenRead(const Uri& uri) {
	return DoRequest(m_request = HttpWebRequest(uri.ToString()), ConstBuf());
}

Blob WebClient::DoRequest(HttpWebRequest& req, RCString data) {
	Blob bdata(nullptr);
	if (data != nullptr)
		bdata = Encoding->GetBytes(data);
	InetStream& stm = DoRequest(req, bdata);
#if UCFG_USE_LIBCURL
	return stm.ResultStream.Blob;
#else
	return BinaryReader(stm).ReadToEnd();
#endif
}

Blob WebClient::DownloadData(RCString address) {
	return DoRequest(m_request = HttpWebRequest(address));
}

Blob WebClient::UploadData(RCString address, const ConstBuf& data) {
	m_request = HttpWebRequest(address);
	WebRequestKeeper keeper(CurrentRequest, m_request);
	m_request.Method = "POST";
	InetStream& stm = DoRequest(m_request, data);
#if UCFG_USE_LIBCURL
	return stm.ResultStream.Blob;
#else
	return BinaryReader(stm).ReadToEnd();
#endif
}

Blob WebClient::UploadFile(RCString address, const path& fileName) {
	m_request = HttpWebRequest(address);

	MemoryStream ms;
	String boundary = EXT_STR("---------------------" << hex << Clock::now().Ticks);
	const char *pBoundary = boundary;
	m_request.AdditionalHeaders.Set("Content-Type", "multipart/form-data; boundary=" + boundary);

	ms.WriteBuffer("--", 2);
	ms.WriteBuffer(pBoundary, strlen(pBoundary));
	String header = "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"" + fileName.filename().native() +"\"\r\n"
					"Content-Type: application/octet-stream\r\n\r\n";
	const char *ps = header;
	ms.WriteBuffer(ps, strlen(ps));
	ms.WriteBuf(File::ReadAllBytes(fileName));		//!!!TODO:  change to stream reading for big files
	ms.WriteBuffer("\r\n--", 4);
	ms.WriteBuffer(pBoundary, strlen(pBoundary));
	ms.WriteBuffer("--\r\n", 4);

	WebRequestKeeper keeper(CurrentRequest, m_request);
	m_request.Method = "POST";	
	InetStream& stm = DoRequest(m_request, ms);
#if UCFG_USE_LIBCURL
	return stm.ResultStream.Blob;
#else
	return BinaryReader(stm).ReadToEnd();
#endif
}

String WebClient::DownloadString(RCString address) {
	return Encoding->GetChars(DownloadData(address));
}

String WebClient::UploadString(RCString address, RCString data) {
	m_request = HttpWebRequest(address);
	WebRequestKeeper keeper(CurrentRequest, m_request);
	m_request.Method = "POST";
	return Encoding->GetChars(DoRequest(m_request, data));
}

static StaticRegex s_reProxy("(socks|http)=([^:]+):(\\d+)");

ptr<WebProxy> WebProxy::FromString(RCString s) {
	cmatch m;
	if (regex_match(s.c_str(), m, *s_reProxy)) {
		ProxyType typ = m[1].str()=="socks" ? ProxyType::Socks : ProxyType::Http;
		return new WebProxy(String(m[2]), Convert::ToUInt16(String(m[3])), typ);
	} else
		Throw(errc::invalid_argument);
}

} // Ext::




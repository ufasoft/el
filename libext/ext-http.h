/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


#include <el/libext/ext-net.h>

#if UCFG_WIN32
#	include <wininet.h>
#endif

#if UCFG_USE_LIBCURL
#	include <curl/curl.h>
#endif


namespace Ext {

class CInternetConnection;
class CHttpInternetConnection;
class HttpWebResponse;

#if !UCFG_USE_LIBCURL

class CInternetFile : public File { //!!!
public:
	~CInternetFile();
	void Attach(HINTERNET hInternet);
	uint32_t Read(void *lpBuf, size_t nCount, int64_t offset) override;
	String ReadString();
	DWORD SetFilePointer(LONG offset, DWORD method);
protected:
	HINTERNET m_hInternet;
};

AFX_API int AFXAPI InetCheck(int i);

class CInternetSession : public SafeHandle {
public:
//	DWORD Flags;
//!!!	WebHeaderCollection Headers;

	CInternetSession(bool)
//!!!		:	Flags(0)
	{}

	CInternetSession(RCString agent = nullptr, DWORD dwContext = 1, DWORD dwAccessType = PRE_CONFIG_INTERNET_ACCESS,
		RCString pstrProxyName = nullptr, RCString pstrProxyBypass = nullptr, DWORD dwFlags = 0);

	void Init(RCString agent = nullptr, DWORD dwContext = 1, DWORD dwAccessType = PRE_CONFIG_INTERNET_ACCESS, RCString pstrProxyName = nullptr, RCString pstrProxyBypass = nullptr, DWORD dwFlags = 0);
	~CInternetSession();
	void Create() {}
//!!!R	void Close();
	void OpenUrl(CInternetConnection& conn, RCString url, LPCTSTR headers, DWORD headersLength = -1, DWORD dwFlags = 0, DWORD_PTR ctx = 0);
	void Connect(CInternetConnection& conn, RCString serverName, INTERNET_PORT port, RCString userName, RCString password, DWORD dwService, DWORD dwFlags = 0, DWORD_PTR ctx = 0);

	/*!!!
	operator HINTERNET() const {
		return m_hSession;
	}

	void Attach(HINTERNET h) {
		if (_self)
			Throw(ExtErr::AlreadyOpened);
		InetCheck(h != 0);
		m_hSession = h;
	}*/
protected:
	void ReleaseHandle(intptr_t h) const override;
};

/*!!!R
class CInetStreambuf : public std::streambuf {
public:
	CInternetFile m_file;

	int overflow(int c);
	int underflow();
	int uflow();
};

class CInetIStream : public std::istream {
public:
	CInetIStream()
		:	std::istream(new CInetStreambuf)
	{}

	void open(CInternetSession& sess, RCString url);
};*/

#endif // !UCFG_USE_LIBCURL


#if UCFG_USE_LIBCURL

class CurlSession : public Object {
public:
	typedef InterlockedPolicy interlocked_policy; //!!!

	IPEndPoint EndPoint;
	CURL *m_h;
	char m_errbuf[CURL_ERROR_SIZE];

	CurlSession(const IPEndPoint& ep)
		:	EndPoint(ep)
		,	m_h(::curl_easy_init())
	{
		ZeroStruct(m_errbuf);
		if (!m_h)
			Throw(E_FAIL);
		SetOption(CURLOPT_ERRORBUFFER, m_errbuf);
	}

	~CurlSession();

	void Reset() {
		if (m_h)
			::curl_easy_reset(m_h);
	}

	void SetOption(CURLoption option, long v);
	void SetOption(CURLoption option, const void *p);
	void SetNullFunctions();
};

void CurlCheck(CURLcode rc, CurlSession *sess = 0);

#endif // UCFG_USE_LIBCURL


#if UCFG_USE_LIBCURL
class CInternetConnection {
public:
	ptr<CurlSession> Session;
#else
class CInternetConnection : public SafeHandle {
	void ReleaseHandle(intptr_t h) const override;
public:
	void HttpOpen(CHttpInternetConnection& respConn, RCString verb, RCString objectName, RCString version, RCString referer, LPCTSTR* lplpszAcceptTypes = 0, DWORD dwFlags = 0, DWORD_PTR ctx = 0);
	void AddHeaders(const WebHeaderCollection& headers, DWORD dwModifiers = HTTP_ADDREQ_FLAG_ADD);
#endif
	~CInternetConnection();
};


class CHttpInternetConnection : public CInternetConnection {
	typedef CInternetConnection base;
public:	
};

ENUM_CLASS(ProxyType) {
	Default,
	Http,
	Socks
} END_ENUM_CLASS(ProxyType);


class WebProxy : public Object {
public:
	typedef InterlockedPolicy interlocked_policy;
	Uri Address;
	Ext::ProxyType Type;

	WebProxy()
		:	Type(ProxyType::Http)
	{
	}

	WebProxy(std::nullptr_t)
		:	Type(ProxyType::Default)
	{
	}

	WebProxy(RCString host, uint16_t port, ProxyType typ = ProxyType::Http)
		:	Address("http://" + host + ":" + Convert::ToString(port))					// http:// here only for Url conventions, because Proxy can be SOCKS
		,	Type(typ)
	{
	}

	static ptr<WebProxy> AFXAPI FromString(RCString s);
};


class NetworkCredential {
public:
	String UserName, Password;

	NetworkCredential(RCString userName = nullptr, RCString password = nullptr)
		:	UserName(userName)
		,	Password(password)
	{}
};

class WebRequest {
public:
	String Method;
	Uri RequestUri;
	NetworkCredential Credentials;
	ptr<WebProxy> Proxy;

#if UCFG_USE_LIBCURL
	WebRequest()
		:	m_timeout(-1)
	{
	}

	virtual ptr<CurlSession> Connect();
#else

	void Connect();
#endif
protected:
	class Impl : public Object {
	public:
#if !UCFG_USE_LIBCURL
		CInternetSession m_sess;
		CInternetConnection m_conn;

		Impl()
			:	m_sess(true)
		{}
#endif
	};

	DWORD m_serviceType;

#if !UCFG_USE_LIBCURL
	CInternetSession& Session();
#endif

	void SetPimpl(Impl *pimpl) {
		m_pimpl = pimpl;
	}
protected:
#if UCFG_USE_LIBCURL
	int m_timeout;
#endif
private:
	ptr<Impl> m_pimpl;
};

template <int WSIZE = 4096>
class CallbackedIoBuffer {
public:
	uint8_t RecvBuf[WSIZE];
	size_t ReceivedSize;
	uint8_t *UserReadBuf;
	size_t UserReadSize;
	const uint8_t *UserWriteBuf;
	size_t UserWriteSize;

	CallbackedIoBuffer()
		:	ReceivedSize(0)
		,	UserReadBuf(0)
		,	UserWriteBuf(0)
	{
	}

	bool ReadToUserBuf(uint8_t *buf, size_t size) {
		size_t n = std::min(size, ReceivedSize);
		memcpy(buf, RecvBuf, n);
		ReceivedSize -= n;
		if (n < size) {
			UserReadBuf = buf+n;
			UserReadSize = size-n;
			return false;
		} else {
			UserReadSize = 0;
			memmove(RecvBuf, RecvBuf+n, ReceivedSize);
			return true;
		}
	}
};


class InetStream : public Stream {
public:
#if UCFG_USE_LIBCURL
	ptr<CurlSession> m_curl;

	InetStream() {
	}

	void Attach(ptr<CurlSession> curl);
#else
	observer_ptr<CInternetConnection> m_pConnection;
#endif
	void *m_pResponseImpl;


	size_t Read(void *buf, size_t size) const override;
	int ReadByte() const override;
	void WriteBuffer(const void *buf, size_t count) override;
private:
#if UCFG_USE_LIBCURL
	mutable CallbackedIoBuffer<CURL_MAX_WRITE_SIZE> m_io;
	size_t m_writeSize;
	MemoryStream ResultStream;

	uint8_t *m_buf;
	int m_rest;

	static size_t WriteFunction(void *ptr, size_t size, size_t nmemb, void *stream);
	static size_t ReadFunction(void *ptr, size_t size, size_t nmemb, void *stream);
	static size_t HeaderFunction(void *ptr, size_t size, size_t nmemb, void *stream);
#endif

friend class WebClient;
};

class HttpWebRequest;

class HttpWebResponse : public Object {
	typedef HttpWebResponse class_type;
public:
	HttpWebResponse()
	{}

	HttpWebResponse(HttpWebRequest& req)
		:	m_pImpl(new Impl(req))
	{
	}

	~HttpWebResponse();

	Stream& GetResponseStream() { return m_pImpl->m_stm; }
 
#if UCFG_USE_LIBCURL
	void Attach(ptr<CurlSession> curl) {
		m_pImpl->m_conn.Session = curl;
		m_pImpl->m_stm.Attach(curl);
	}
/*#else
	void Attach(HINTERNET h) {
		m_pImpl->m_conn.Attach(h);
		m_pImpl->m_stm.m_hFile = h;
	}*/
#endif

	int get_StatusCode();
	DEFPROP_GET(int, StatusCode);

	WebHeaderCollection get_Headers();
	DEFPROP_GET(WebHeaderCollection, Headers);

#pragma push_macro("DEF_STRING_PROP")
#define DEF_STRING_PROP(name, dwInfoLevel)					\
	String get_##name() { return GetString(dwInfoLevel); }	\
	DEFPROP_GET(String, name);

#if UCFG_USE_LIBCURL
	int64_t get_ContentLength();

	String get_StatusDescription();
	DEFPROP_GET(String, StatusDescription);
#else
	DEF_STRING_PROP(ContentEncoding,	HTTP_QUERY_CONTENT_ENCODING)
	DEF_STRING_PROP(ContentType,		HTTP_QUERY_CONTENT_TYPE)
	DEF_STRING_PROP(Method,				HTTP_QUERY_REQUEST_METHOD)
	DEF_STRING_PROP(StatusDescription,	HTTP_QUERY_STATUS_TEXT)
	DEF_STRING_PROP(Server,				HTTP_QUERY_SERVER)

	int64_t get_ContentLength() {
		return Convert::ToInt64(GetString(HTTP_QUERY_CONTENT_LENGTH));
	}	
#endif
	DEFPROP_GET(int64_t, ContentLength);

#pragma pop_macro("DEF_STRING_PROP") 

	
private:
	class Impl : public Object {
	public:
		HttpWebRequest& m_req;
		CHttpInternetConnection m_conn;
		InetStream m_stm;

		Impl(HttpWebRequest& req)
			:	m_req(req)
		{
			m_stm.m_pResponseImpl = this;
		}

#if UCFG_USE_LIBCURL
		String m_statusDesc;
		WebHeaderCollection m_headers;
#endif
	};

	ptr<Impl> m_pImpl;

	String GetString(DWORD dwInfoLevel);


friend class HttpWebRequest;
friend class WebClient;
friend class InetStream;
};

ENUM_CLASS(http_error) {
	continue_request = 100,
	switching_protocols = 101,
	ok = 200,
	unauthorized = 401,	
	gateway_timeout = 504,
	version_not_supported = 505
} END_ENUM_CLASS(http_error);


const error_category& http_category();

inline error_code make_error_code(http_error e) { return error_code(int(e), http_category()); }
} namespace std {
	template <> struct is_error_code_enum<http_error> : true_type {};
} namespace Ext {

class WebException : public Exception {
	typedef Exception base;
public:
	String Result;
//	HttpWebResponse Response;

	WebException(http_error errval, RCString msg = nullptr)
		:	base(make_error_code(errval), msg)
	{}

	~WebException() noexcept {}
};

ENUM_CLASS(RequestCacheLevel) {
	Default,
	BypassCache
} END_ENUM_CLASS(RequestCacheLevel);


class HttpWebRequest : public WebRequest {
	typedef HttpWebRequest class_type;
	typedef WebRequest base;
public:
	WebHeaderCollection Headers;
	WebHeaderCollection AdditionalHeaders;
	bool KeepAlive;
	CInt<int64_t> ContentLength;
	String ContentType, Referer;
	RequestCacheLevel CacheLevel;

	HttpWebRequest(RCString url = nullptr);
	
	~HttpWebRequest();

	HttpWebResponse GetResponse(const uint8_t *p = 0, size_t size = 0);

	DWORD get_Timeout();
	void put_Timeout(DWORD v);
	DEFPROP(DWORD, Timeout);

	String get_UserAgent();
	void put_UserAgent(RCString v);
	DEFPROP(String, UserAgent);

//!!!	ptr<WebProxy> get_Proxy();
//!!!	void put_Proxy(WebProxy *proxy);
//!!!R	DEFPROP(ptr<WebProxy>, Proxy);

	void Send(const uint8_t *p = 0, size_t size = 0);
	Stream& GetRequestStream(const uint8_t *p = 0, size_t size = 0);
	bool EndRequest();
	void ReleaseFromAPC();

#if UCFG_USE_LIBCURL
	observer_ptr<curl_slist> m_headers;

	void AddHeader(CURL *curl, RCString name, RCString val);
	ptr<CurlSession> Connect() override;
#endif
private:
	class Impl : public base::Impl {
	public:
	};

	ptr<Impl> m_pimpl;

	HttpWebResponse m_response;
	ptr<WebProxy> m_proxy;
	CBool m_bSendEx;
#if UCFG_USE_LIBCURL
	String m_userAgent;
	Blob m_postData;
#endif

	String GetHeadersString();

	friend class WebClient;
};

class WebClient : public Object {
	typedef WebClient class_type;
public:
	NetworkCredential Credentials;
	ptr<WebProxy> Proxy;

	String UserAgent;
	WebHeaderCollection Headers;
	HttpWebRequest *CurrentRequest;
	observer_ptr<Ext::Encoding> Encoding;
	RequestCacheLevel CacheLevel;

	WebClient();

	WebHeaderCollection get_ResponseHeaders() {
		return m_response.Headers;
	}

	DEFPROP_GET(WebHeaderCollection, ResponseHeaders);

	Stream& OpenRead(const Uri& uri);

	Blob DownloadData(RCString address);
	Blob UploadData(RCString address, const ConstBuf& data);
	Blob UploadFile(RCString address, const path& fileName);
	String DownloadString(RCString address);
	String UploadString(RCString address, RCString Data);
protected:
	HttpWebRequest m_request;
	HttpWebResponse m_response;

	virtual void OnHttpWebRequest(HttpWebRequest& req) {}
private:
	InetStream& DoRequest(HttpWebRequest& req, const ConstBuf data);
	Blob DoRequest(HttpWebRequest& req, RCString data = nullptr);
};

} // Ext::


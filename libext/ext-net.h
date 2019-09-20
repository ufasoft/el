/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#if UCFG_WIN32

#	undef inet_ntop
#	undef inet_pton

#	include <winsock2.h>
#	if UCFG_WCE
#		include <ws2tcpip.h>
#	else
#		include <ws2ipdef.h>
#	endif
//#	include <ws2tcpip.h>

#	define inet_ntop API_inet_ntop
#	define inet_pton API_inet_pton
#endif

#if UCFG_USE_POSIX
#	include <netdb.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#endif

namespace Ext {
using namespace std;

class Socket;
class CSocketHandleKeeper;


__forceinline uint32_t Fast_ntohl(uint32_t v) { return be32toh(v); }
__forceinline uint16_t Fast_ntohs(uint16_t v) { return be16toh(v); }
__forceinline uint32_t Fast_htonl(uint32_t v) { return htobe32(v); }
__forceinline uint16_t Fast_htons(uint16_t v) { return htobe16(v); }

#if UCFG_WIN32
class AFX_CLASS CWSAEvent {
public:
	WSAEVENT m_hEvent;

	CWSAEvent();
	virtual ~CWSAEvent();
};
#endif

class AFX_CLASS CUsingSockets {
public:
#if UCFG_WIN32
	WSAData m_data;
#endif
private:
	CBool m_bInited;
public:
	CUsingSockets(bool bInit = true) {
		if (bInit)
			EnsureInit();
	}

	~CUsingSockets();
	void EnsureInit();
	void Close();
};

ENUM_CLASS(AddressFamily) {
	Unknown				= -1
	, Unspecified		= AF_UNSPEC
	, Unix				= AF_UNIX
	, AppleTalk			= AF_APPLETALK
	, Ipx				= AF_IPX
	, InterNetwork		= AF_INET
	, InterNetworkV6	= AF_INET6
	, NetBios			= AF_NETBIOS
} END_ENUM_CLASS(AddressFamily);

ENUM_CLASS(SocketType) {
	Unknown = -1
	, Stream = SOCK_STREAM
	, Dgram = SOCK_DGRAM
	, Raw = SOCK_RAW
	, Rdm = SOCK_RDM
	, Seqpacket = SOCK_SEQPACKET
} END_ENUM_CLASS(SocketType);

ENUM_CLASS(ProtocolType) {
	Unknown			= -1
	, Unspecified	= 0
	, Igmp			= IPPROTO_IGMP
	, Tcp			= IPPROTO_TCP
	, Udp			= IPPROTO_UDP
} END_ENUM_CLASS(ProtocolType);

class IPAddress : public CPrintable {
	typedef IPAddress class_type;

	void CreateCommon();
public:
	EXT_DATA static const IPAddress Any, Broadcast, Loopback, None, IPv6Any, IPv6Loopback, IPv6None;

	static size_t FamilySize(AddressFamily fam) {
		switch (fam) {
		case Ext::AddressFamily::Unspecified: return 0;
		case Ext::AddressFamily::InterNetwork: return 4;
		case Ext::AddressFamily::InterNetworkV6: return 16;
		default:
			return 0;
		}
	}

	union {
		sockaddr m_sockaddr;
		sockaddr_in m_sin;
		sockaddr_in6 m_sin6;
	};

	Ext::AddressFamily get_AddressFamily() const { return Ext::AddressFamily(m_sockaddr.sa_family); }
	void put_AddressFamily(Ext::AddressFamily fam) { m_sockaddr.sa_family = (short)fam; }
	DEFPROP_CONST(Ext::AddressFamily, AddressFamily);

	IPAddress();

	IPAddress(const IPAddress& ha) {
		operator=(ha);
	}

	explicit IPAddress(uint32_t nboIp4);		// Network byte order

	IPAddress(const sockaddr& sa);

	explicit IPAddress(RCSpan mb);
//!!!	IPAddress(RCString domainname);
	//!!!RIPAddress& operator=(RCString domainname);
	IPAddress& operator=(const IPAddress& ha);

	size_t GetHashCode() const;
	bool operator<(const IPAddress& ha) const;
	bool operator==(const IPAddress& ha) const;
	bool operator!=(const IPAddress& ha) const { return !operator==(ha); }
	void Print(ostream& os) const override;

	Blob GetAddressBytes() const;

	uint32_t get_ScopeId() const { return m_sin6.sin6_scope_id; }
	void put_ScopeId(uint32_t v) {  m_sin6.sin6_scope_id = v; }
	DEFPROP_CONST(uint32_t, ScopeId);

	uint32_t GetIP() const;
	static IPAddress AFXAPI Parse(RCString ipString);
	static bool AFXAPI TryParse(RCString s, IPAddress& ip);

	bool IsEmpty() const {
		return AddressFamily==Ext::AddressFamily::InterNetwork && !GetIP();
	}

	bool IsGlobal() const;

	bool get_IsIPv4MappedToIPv6() const;
	DEFPROP_GET(bool, IsIPv4MappedToIPv6);

	bool get_IsIPv6Teredo() const;
	DEFPROP_GET(bool, IsIPv6Teredo);

	friend class IPEndPoint;
};

} // Ext::

namespace EXT_HASH_VALUE_NS {
inline size_t hash_value(const Ext::IPAddress& ha) { return ha.GetHashCode(); }
}

EXT_DEF_HASH(Ext::IPAddress)

namespace Ext {

BinaryWriter& AFXAPI operator<<(BinaryWriter& wr, const IPAddress& ha);
const BinaryReader& AFXAPI operator>>(const BinaryReader& rd, IPAddress& ha);

class EndPoint : public Object, public CPrintable {
	typedef EndPoint class_type;
public:
	virtual ~EndPoint() {}

	virtual Ext::AddressFamily get_AddressFamily() const { return AddressFamily::Unspecified; }
	DEFPROP_VIRTUAL_GET_CONST(Ext::AddressFamily, AddressFamily);
};

class InternetEndPoint : public EndPoint {
	typedef EndPoint base;
	typedef InternetEndPoint class_type;

	uint16_t m_port;
public:
	virtual uint16_t get_Port() const { return m_port; }
	virtual void put_Port(uint16_t port) { m_port = port; }
	DEFPROP_VIRTUAL_CONST(uint16_t, Port);
};

class IPEndPoint : public InternetEndPoint {
	typedef InternetEndPoint base;
	typedef IPEndPoint class_type;
public:
	IPAddress Address;

	explicit IPEndPoint(uint32_t host = 0, uint16_t port = 0)
		: Address(host)
	{
		Port = port;
	}

	explicit IPEndPoint(const IPAddress& a, uint16_t port = 0)
		: Address(a)
	{
		Port = port;
	}

	explicit IPEndPoint(RCSpan mb)
		: Address(mb)
	{
	}

	IPEndPoint(const sockaddr& sa)
		: Address(sa)
	{
		switch (sa.sa_family) {
		case AF_INET:
			Port = ntohs(((const sockaddr_in&)sa).sin_port);
			break;
		case AF_INET6:
			Port = ntohs(((const sockaddr_in6&)sa).sin6_port);
			break;
		}
	}

	IPEndPoint(const IPEndPoint& ep) {
		operator=(ep);
	}

	IPEndPoint& operator=(const IPEndPoint& ep) {
		Address = ep.Address;
		Port = ep.Port;
		return *this;
	}

	Ext::AddressFamily get_AddressFamily() const override { return Address.AddressFamily; }
	DEFPROP_GET_CONST(Ext::AddressFamily, AddressFamily);

	uint16_t get_Port() const override { return ntohs(Address.m_sin.sin_port); }

	void put_Port(uint16_t port) override {
		base::put_Port(port);
		Address.m_sin.sin_port = htons(port);
	}

	const sockaddr *c_sockaddr() const;
	size_t sockaddr_len() const;

	void Print(ostream& os) const override;
	EXT_API static IPEndPoint AFXAPI Parse(RCString s);

	bool operator<(const IPEndPoint& hp) const { return Address<hp.Address || Address==hp.Address && Port<hp.Port; }
	bool operator==(const IPEndPoint& hp) const { return Address==hp.Address && Address.m_sin.sin_port==hp.Address.m_sin.sin_port; }
	bool operator!=(const IPEndPoint& hp) const { return !operator==(hp); }
};

class DnsEndPoint : public InternetEndPoint {
	typedef InternetEndPoint base;
public:
	String Host;

	DnsEndPoint(RCString host, uint16_t port)
		: Host(host)
	{
		Port = port;
	}

	Ext::AddressFamily get_AddressFamily() const override { return AddressFamily::Unspecified; }
	DEFPROP_GET_CONST(Ext::AddressFamily, AddressFamily);

	String ToString() const override { return Host + ":" + Convert::ToString(Port); }
};

} // Ext::

namespace EXT_HASH_VALUE_NS {
	inline size_t hash_value(const Ext::IPEndPoint& ep) { return hash_value(ep.Address) ^ std::hash<uint16_t>()(ep.Port); }
	inline size_t hash_value(const Ext::DnsEndPoint& ep) { return hash_value(ep.Host) ^ std::hash<uint16_t>()(ep.Port); }
}

EXT_DEF_HASH(Ext::IPEndPoint)
EXT_DEF_HASH(Ext::DnsEndPoint)

namespace Ext {

inline BinaryWriter& AFXAPI operator<<(BinaryWriter& wr, const IPEndPoint& hp) {
	return wr << hp.Address << (uint16_t)hp.Port;
}

inline const BinaryReader& AFXAPI operator>>(const BinaryReader& rd, IPEndPoint& hp) {
	rd >> hp.Address;
	hp.Port = rd.ReadUInt16();
	return rd;
}

class IPHostEntry {
public:
	String HostName;
	std::vector<IPAddress> AddressList;
	std::vector<String> Aliases;

	explicit IPHostEntry(hostent *phost = 0);
};

class Dns {
public:
	AFX_API static String GetHostName();
	AFX_API static IPHostEntry AFXAPI GetHostEntry(RCString hostNameOrAddress);
	AFX_API static IPHostEntry AFXAPI GetHostEntry(const IPAddress& address);
	AFX_API static vector<IPAddress> GetHostAddresses(RCString hostNameOrAddress);
};

class IPAddrInfo : noncopyable {
	addrinfo* m_ai;
public:
	IPAddrInfo(RCString hostname = Dns::GetHostName());
	~IPAddrInfo();
	vector<IPAddress> GetIPAddresses() const;
};

template <class C, class B> class SafeHandleAdapter : public B {
public:
	SafeHandleAdapter(C& c)
		: B(c)
	{}

	operator typename C::handle_type() {
		return (typename C::handle_type)(intptr_t)(B::operator intptr_t());
	}
};

struct LingerOption {
	int LingerTime;
	bool Enabled;

	LingerOption(bool enabled = false, int lingerTime = 0)
		: LingerTime(lingerTime)
		, Enabled(enabled)
	{}
};

class Socket : public SafeHandle {
	typedef SafeHandle base;
	typedef Socket class_type;
	EXT_MOVABLE_BUT_NOT_COPYABLE(Socket);

	CBool m_bBlocking;
public:
	typedef SOCKET handle_type;

	typedef SafeHandleAdapter<Socket, SafeHandle::HandleAccess> HandleAccess;
	typedef SafeHandleAdapter<Socket, SafeHandle::BlockingHandleAccess> BlockingHandleAccess;

	class COSSupportsIPver {
		AddressFamily m_af;
		int m_supports;
	public:
		COSSupportsIPver(AddressFamily af)
			: m_af(af)
			, m_supports(-1)
		{}

		operator bool();
	};

	EXT_DATA static COSSupportsIPver OSSupportsIPv4, OSSupportsIPv6;

	Socket()
		: m_bBlocking(true)
	{
	}

	Socket(AddressFamily af, SocketType sockType = SocketType::Stream, ProtocolType protoType = ProtocolType::Tcp) {
		Create(af, sockType, protoType);
	}

	Socket(EXT_RV_REF(Socket) rv)
		: base(static_cast<EXT_RV_REF(SafeHandle)>(rv))
		, m_bBlocking(rv.m_bBlocking)
	{}

	virtual ~Socket();

	EXT_API void Create(AddressFamily af, SocketType sockType = SocketType::Stream, ProtocolType protoType = ProtocolType::Tcp);

	Socket& operator=(EXT_RV_REF(Socket) rv) {
		base::operator=(static_cast<EXT_RV_REF(SafeHandle)>(rv));
		m_bBlocking = rv.m_bBlocking;
		return *this;
	}

	AFX_API static void AFXAPI ReleaseFromAPC();
//!!!R	void Create(uint16_t nPort = 0, int nSocketType = SOCK_STREAM, uint32_t host = 0);
	virtual int Receive(void *buf, int len, int flags = 0);
	virtual int Send(const void *buf, int len, int flags = 0);
	virtual void SendTo(RCSpan cbuf, const IPEndPoint& ep);
	//!!!  void Close();

	IPEndPoint get_LocalEndPoint();
	DEFPROP_GET(IPEndPoint, LocalEndPoint);

	IPEndPoint get_RemoteEndPoint();
	DEFPROP_GET(IPEndPoint, RemoteEndPoint);

	bool Connect(RCString hostAddress, uint16_t hostPort);

	bool Connect(const EndPoint& ep) { return ConnectHelper(ep); }

	void Bind(const IPEndPoint& ep = IPEndPoint());
	void Listen(int backLog = SOMAXCONN);
	pair<Socket, IPEndPoint> Accept();

#if UCFG_WIN32
	void EventSelect(HANDLE hEvent = 0, long lEvents = 0);
	WSANETWORKEVENTS EnumNetworkEvents(HANDLE hEvent = 0);

	int Ioctl(DWORD ioCode, const void *pIn, DWORD cbIn, void *pOut = 0, DWORD cbOut = 0, WSAOVERLAPPED *pov = 0, LPWSAOVERLAPPED_COMPLETION_ROUTINE pfCR = 0);
	EXT_API void Create(int af, int type, int protocol, LPWSAPROTOCOL_INFO lpProtocolInfo, DWORD dwFlag);
	EXT_API WSAPROTOCOL_INFO Duplicate(DWORD dwProcessid);
#endif

	void Shutdown(int how = SHUT_RDWR);
	int ReceiveFrom(void *buf, int len, IPEndPoint& ep);
	void Attach(SOCKET s);
	SOCKET Detach();

	void GetSocketOption(int optionLevel, int optionName, void *pVal, socklen_t& len);

	int GetSocketOption(int optionLevel, int optionName) {
		int n;
		socklen_t size = sizeof n;
		GetSocketOption(optionLevel, optionName, &n, size);
		if (size != sizeof n)
			Throw(E_FAIL);
		return n;
	}

	void SetSocketOption(int optionLevel, int optionName, RCSpan mb);

	void SetSocketOption(int optionLevel, int optionName, bool v) {
		int n = v;
		SetSocketOption(optionLevel, optionName, Span((uint8_t*)&n, sizeof n));
	}

	void SetSocketOption(int optionLevel, int optionName, int v) {
		SetSocketOption(optionLevel, optionName, Span((uint8_t*)&v, sizeof v));
	}

	LingerOption get_LingerState() {
		linger lng;
		socklen_t size = sizeof lng;
		GetSocketOption(SOL_SOCKET, SO_LINGER, &lng, size);
		if (size != sizeof lng)
			Throw(E_FAIL);
		return LingerOption(lng.l_onoff, lng.l_linger);
	}

	void put_LingerState(const LingerOption& lo) {
		linger lng;
		lng.l_onoff = lo.Enabled;
		lng.l_linger = (u_short)lo.LingerTime;
		SetSocketOption(SOL_SOCKET, SO_LINGER, Span((uint8_t*)&lng, sizeof lng));
	}
	DEFPROP(LingerOption, LingerState);

	bool get_ReuseAddress() {
		uint32_t v;
		socklen_t size = sizeof v;
		GetSocketOption(SOL_SOCKET, SO_REUSEADDR, &v, size);
		if (size != sizeof v)
			Throw(E_FAIL);
		return v;
	}
	void put_ReuseAddress(bool b) {
		uint32_t v = b;
		SetSocketOption(SOL_SOCKET, SO_REUSEADDR, Span((uint8_t*)&v, sizeof v));
	}
	DEFPROP(bool, ReuseAddress);

#	define	DEF_INT_PROPERTY(propname, level, name)								\
	int get_##propname() { return GetSocketOption(level, name); }				\
	void put_##propname(int v) { return SetSocketOption(level, name, v); }		\
	DEFPROP(int, propname);

	DEF_INT_PROPERTY(ReceiveTimeout, SOL_SOCKET, SO_RCVTIMEO);
	DEF_INT_PROPERTY(SendTimeout, SOL_SOCKET, SO_SNDTIMEO);
	DEF_INT_PROPERTY(ReceiveBufferSize, SOL_SOCKET, SO_RCVBUF);
	DEF_INT_PROPERTY(SendBufferSize, SOL_SOCKET, SO_SNDBUF);
	DEF_INT_PROPERTY(Ttl, IPPROTO_IP, IP_TTL);

#	undef DEF_INT_PROPERTY


	void IOControl(int code, const span<uint8_t>& mb);

	void SetKeepAliveTime(int ms);

	int get_Available();
	DEFPROP_GET(int, Available);

#if UCFG_USE_POSIX
	int get_Flags() { return CCheck(fcntl(HandleAccess(_self), F_GETFL, 0)); }
	void put_Flags(int v) { CCheck(fcntl(HandleAccess(_self), F_SETFL, v)); }
	DEFPROP(int, Flags);
#endif

	bool get_Blocking() { return m_bBlocking; }
	void put_Blocking(bool b) {
#if UCFG_USE_POSIX
		Flags = (Flags & ~O_NONBLOCK) | (b ? 0 : O_NONBLOCK);
#else
		DWORD d = !b;
		IOControl(FIONBIO, span<uint8_t>((uint8_t*)&d, sizeof d));
#endif
		m_bBlocking = b;
	}
	DEFPROP(bool, Blocking);
protected:
	virtual bool ConnectHelper(const EndPoint& ep);
	void ReleaseHandle(intptr_t h) const;
};

template <class T>
class CSocketKeeper {
public:
	Socket& m_sock;
	T& m_tr;

	CSocketKeeper(T& tr, Socket& sock)	//, int nPort = 0, int nSocketType = SOCK_STREAM, DWORD host = 0)
		: m_sock(sock)
		, m_tr(tr)
	{
		EXT_LOCK (m_tr.MtxCallingAPI) {
			if (tr.m_bStop)
				Throw(ExtErr::ThreadInterrupted);
			m_tr.m_arKeepers.push_back(this);
//!!!R			if (!m_sock.Valid() && nPort!=-1)
//!!!R				m_sock.Create((WORD)nPort, nSocketType, host);
		}
	}

	~CSocketKeeper() {
		EXT_LOCK (m_tr.MtxCallingAPI) {
			m_tr.m_arKeepers.erase(std::remove(m_tr.m_arKeepers.begin(), m_tr.m_arKeepers.end(), this), m_tr.m_arKeepers.end());
			m_sock.Close();
		}
	}
};

template <class B>
class SocketThreadWrap : public B {
	typedef B base;
public:
	typedef CSocketKeeper<SocketThreadWrap> SocketKeeper;

	std::mutex MtxCallingAPI;
	std::vector<SocketKeeper*> m_arKeepers;
	CBool m_bClosing;

	SocketThreadWrap()
//!!!R		: m_lock(&m_csCallingAPI)
	{}

	void Stop() override {
		base::Stop();
		m_bClosing = true;
#if UCFG_WCE
		EXT_LOCK (MtxCallingAPI) {
			for (int i=0; i<m_arKeepers.size(); i++) {
				try {
					m_arKeepers[i]->m_sock.Shutdown();
				} catch (RCExc) {
				}
			}
		}
#endif

		EXT_LOCK (MtxCallingAPI) {
			for (size_t i = 0; i < m_arKeepers.size(); i++)
				m_arKeepers[i]->m_sock.Close();
#if UCFG_WIN32_FULL
			QueueAPC();
#endif
		}
	}
protected:
	void OnAPC() override {
//		FUN_TRACE;

		Socket::ReleaseFromAPC();

		//!!!	m_lock.Unlock();
	}

	friend class CSocketKeeper<SocketThreadWrap>;
};

class SocketThread : public SocketThreadWrap<Thread> {
public:
	SocketThread(thread_group *tr = 0) {
		m_owner.reset(tr);
	}
};

#ifdef _WIN32
typedef SocketThreadWrap<WorkItem> SocketWorkItem;
#endif

template <class T>
class ListenerThread : public SocketThread {
	typedef SocketThread base;
public:
	Socket m_sockListen;
	SocketKeeper m_socketKeeper;
	IPEndPoint m_ep;

	ListenerThread(thread_group& tg, const IPEndPoint& ep)
		: base(&tg)
		, m_ep(ep)
		, m_sockListen(ep.Address.AddressFamily, SocketType::Stream, ProtocolType::Tcp)
		, m_socketKeeper(_self, m_sockListen)
	{
	}
protected:
	void BeforeStart() override {
		TRC(2, "Listening on " << m_ep);

		m_sockListen.Bind(m_ep);
		m_sockListen.Listen();
	}

	void Execute() override {
		Name = "CListenThread";

		DBG_LOCAL_IGNORE_CONDITION(errc::interrupted);

		while (!Ext::this_thread::interruption_requested()) {
			ptr<T> p = new T;
			p->m_owner.reset(&GetThreadRef());
			p->m_sock = move(m_sockListen.Accept().first);
			p->Start();
		}
	}
};

class CSocketLooper {
public:
	static const int BUF_SIZE = 8192;		// default SO_RCVBUF

	bool m_bDoShutdown;
	CBool NoSignal;

	CSocketLooper()
		: m_bDoShutdown(true)
	{
	}

	virtual ~CSocketLooper() {}
	virtual Blob ProcessSrc(RCSpan cbuf, bool& bDisconnectAfterData) { return nullptr; }
	virtual Blob ProcessDest(RCSpan cbuf, bool& bDisconnectAfterData) { return nullptr; }

	virtual void Send(Socket& sock, RCSpan mb);
	virtual TimeSpan GetTimeout() { return TimeSpan(-1); }
	virtual bool NeedToCancel() { return false; }
	void Loop(Socket& sockS, Socket& sockD);
};

AFX_API int AFXAPI SocketCheck(int code);
AFX_API void AFXAPI SocketCodeCheck(int code);
//!!!R AFX_API DWORD AFXAPI NameToHost(const String& name);
DECLSPEC_NORETURN AFX_API void AFXAPI ThrowWSALastError();


class NetworkStream : public Stream {
public:
	Socket& m_sock;
	CBool NoSignal;

	NetworkStream(Socket& sock)
		: m_sock(sock)
	{}


	size_t Read(void *buf, size_t count) const override;
	void WriteBuffer(const void *buf, size_t count) override;
//!!!R	int ReadByte() const override;
	bool Eof() const override;
	void Close() const override;
private:
	mutable CBool m_bEof;
};

class TcpClient : noncopyable {
public:
	Socket Client;
	NetworkStream Stream;

	TcpClient()
		: Stream(Client)
	{}

	virtual void Connect(const EndPoint& ep) {
		Client.Connect(ep);
	}
};


void _cdecl SocketsCleanup();

} // Ext::

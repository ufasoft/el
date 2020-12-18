/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


#include <el/libext/ext-net.h>

#if UCFG_WCE
#	pragma comment(lib, "ws2")
#endif

#if UCFG_USE_POSIX
#	include <netinet/tcp.h>
#endif

using namespace std;

#if UCFG_WIN32
#	include <mstcpip.h>
#	define WSA(e) WSA##e
#else
#	define WSA(e) e

int WSAGetLastError() {
	return errno;
}
#endif // !UCFG_WIN32


namespace Ext {


Socket::COSSupportsIPver::operator bool() {
	if (-1 == m_supports) {
		try {
			Socket sock;
			sock.Create(m_af, SocketType::Stream, ProtocolType::Tcp);
			m_supports = 1;
		} catch (RCExc) {
			m_supports = 0;
		}
	}
	return m_supports;
}

Socket::COSSupportsIPver
	Socket::OSSupportsIPv4(AddressFamily::InterNetwork),
	Socket::OSSupportsIPv6(AddressFamily::InterNetworkV6);

Socket::~Socket() {
	Close(true);
}

void Socket::Create(AddressFamily af, SocketType socktyp, ProtocolType protoTyp) {
	if (Valid())
		Throw(ExtErr::AlreadyOpened);
	SOCKET h = ::socket((int)af, (int)socktyp, (int)protoTyp);
	if (h == INVALID_SOCKET)
		ThrowWSALastError();
	Attach(h);
}

void Socket::ReleaseHandle(intptr_t h) const {
#if UCFG_WIN32
	SocketCheck(::closesocket(SOCKET(h)));
#else
	SocketCheck(::close(static_cast<SOCKET>(h)));
#endif
}

void Socket::Bind(const IPEndPoint& ep) {
	SocketCheck(::bind(HandleAccess(_self), ep.c_sockaddr(), ep.sockaddr_len()));
}

bool Socket::ConnectHelper(const EndPoint& ep) {
	TRC(4, "Connecting to " << ep);
	const DnsEndPoint *dnsEp = dynamic_cast<const DnsEndPoint*>(&ep);
	IPEndPoint ipEp = dnsEp
		? IPEndPoint(Dns::GetHostAddresses(dnsEp->Host).at(0), dnsEp->Port)
		: dynamic_cast<const IPEndPoint&>(ep);
	if (Valid()) {
		//!!!TODO enum IPs
		if (::connect(BlockingHandleAccess(_self), ipEp.c_sockaddr(), ipEp.sockaddr_len()) != SOCKET_ERROR)
			return true;
	} else {
		Socket s(ipEp.AddressFamily, SocketType::Stream, ProtocolType::Tcp);
		if (::connect(BlockingHandleAccess(s), ipEp.c_sockaddr(), ipEp.sockaddr_len()) != SOCKET_ERROR) {
			_self = move(s);
			return true;
		}
	}
	if (WSAGetLastError() != WSA(EWOULDBLOCK))
		ThrowWSALastError();
	return false;
}

bool Socket::Connect(RCString hostAddress, uint16_t hostPort) {
	return ConnectHelper(IPEndPoint(IPAddress::Parse(hostAddress), hostPort));
}

void Socket::SendTo(RCSpan cbuf, const IPEndPoint& ep) {
	SocketCheck(::sendto(BlockingHandleAccess(_self), (const char*)cbuf.data(), cbuf.size(), 0, ep.c_sockaddr(), ep.sockaddr_len()));
}

void Socket::Listen(int backLog) {
	SocketCheck(::listen(HandleAccess(_self), backLog));
}

pair<Socket, IPEndPoint> Socket::Accept() {
	uint8_t sa[50];
	socklen_t addrlen = sizeof(sa);
	SOCKET s = ::accept(BlockingHandleAccess(_self), (sockaddr*)sa, &addrlen);
	pair<Socket, IPEndPoint> r;
	if (s != INVALID_SOCKET) {
		r.first.Attach(s);
		r.second = IPEndPoint(*(const sockaddr*)sa);

		TRC(5, "from " << r.second);
	} else if (WSAGetLastError() != WSA(EWOULDBLOCK))
		ThrowWSALastError();
	return move(r);
}

#ifndef WDM_DRIVER
void Socket::ReleaseFromAPC() {
	if (SafeHandle::HandleAccess *ha = (SafeHandle::HandleAccess*)(void*)SafeHandle::t_pCurrentHandle)
		ha->Release();
}
#endif

#if UCFG_WIN32

void Socket::EventSelect(HANDLE hEvent, long lEvents) {
	SocketCheck(::WSAEventSelect(HandleAccess(_self), hEvent, lEvents));
}

WSANETWORKEVENTS Socket::EnumNetworkEvents(HANDLE hEvent) {
	WSANETWORKEVENTS r;
	SocketCheck(::WSAEnumNetworkEvents(HandleAccess(_self), hEvent, &r));
	return r;
}

int Socket::Ioctl(DWORD ioCode, const void *pIn, DWORD cbIn, void *pOut, DWORD cbOut, WSAOVERLAPPED *pov, LPWSAOVERLAPPED_COMPLETION_ROUTINE pfCR) {
	DWORD r;
	if (::WSAIoctl(BlockingHandleAccess(_self), ioCode, (void*)pIn, cbIn, pOut, cbOut, &r, pov, pfCR) == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK)
			ThrowWSALastError();
		return SOCKET_ERROR;
	}
	return r;
}

#	if !UCFG_WCE
WSAPROTOCOL_INFO Socket::Duplicate(DWORD dwProcessid) {
	WSAPROTOCOL_INFO pi;
	SocketCheck(::WSADuplicateSocket(HandleAccess(_self), dwProcessid, &pi));
	return pi;
}
#	endif

void Socket::Create(int af, int type, int protocol, LPWSAPROTOCOL_INFO lpProtocolInfo, DWORD dwFlag) {
	SOCKET h = ::WSASocket(af, type, protocol, lpProtocolInfo, 0, dwFlag);
	if (h == INVALID_SOCKET)
		ThrowWSALastError();
	Attach(h);
}

int Socket::get_Available() {
	int r;
	IOControl(FIONREAD, span<uint8_t>((uint8_t*)&r, sizeof r));
	return r;
}

CWSAEvent::CWSAEvent()
	:	m_hEvent(WSACreateEvent())
{
	if (m_hEvent == WSA_INVALID_EVENT)
		ThrowWSALastError();
}

CWSAEvent::~CWSAEvent() {
	SocketCheck(WSACloseEvent(m_hEvent));
}


#endif // UCFG_WIN32

void Socket::Shutdown(int how) {
	SocketCheck(::shutdown(HandleAccess(_self), how));
}

int Socket::Receive(void *buf, int len, int flags) {
	int r = ::recv(BlockingHandleAccess(_self), (char*)buf, len, flags);
	if (SOCKET_ERROR == r && WSAGetLastError() != WSA(EWOULDBLOCK))
		ThrowWSALastError();
	return r;
}

int Socket::Send(const void *buf, int len, int flags) {
	int r = ::send(BlockingHandleAccess(_self), (char*)buf, len, flags);
	if (SOCKET_ERROR == r && WSAGetLastError() != WSA(EWOULDBLOCK))
		ThrowWSALastError();
	return r;
}

int Socket::ReceiveFrom(void *buf, int len, IPEndPoint& ep) {
	uint8_t bufSockaddr[100];
	ZeroStruct(bufSockaddr);
	sockaddr& sa =  *(sockaddr*)bufSockaddr;
	sa.sa_family = AF_INET;
	socklen_t addrLen = sizeof(bufSockaddr);
	int r = ::recvfrom(BlockingHandleAccess(_self), (char*)buf, len, 0, &sa, &addrLen);
	if (r == SOCKET_ERROR && WSAGetLastError() != WSA(EWOULDBLOCK))
		ThrowWSALastError();
	ep = IPEndPoint(sa);
	return r;
}

void Socket::Attach(SOCKET s) {
	Close();
	SafeHandle::Attach(intptr_t(s));
	//!!!  m_hSocket = s;
}

SOCKET Socket::Detach() {
	return static_cast<SOCKET>(SafeHandle::Detach());

	//!!!return exchange(m_hSocket, INVALID_SOCKET);
}

void Socket::GetSocketOption(int optionLevel, int optionName, void *pVal, socklen_t& len) {
	SocketCheck(getsockopt(HandleAccess(_self), optionLevel, optionName, (char *)pVal, &len));
}

void Socket::SetSocketOption(int optionLevel, int optionName, RCSpan mb) {
	SocketCheck(::setsockopt(HandleAccess(_self), optionLevel, optionName, (const char*)mb.data(), mb.size()));
}

void Socket::IOControl(int code, const span<uint8_t>& mb) {
#if UCFG_WIN32
	SocketCheck(::ioctlsocket(HandleAccess(_self), code, (u_long*)mb.data()));
#else
	SocketCheck(::ioctl(HandleAccess(_self), code, (u_long*)mb.data()));
#endif
}

void Socket::SetKeepAliveTime(int ms) {
#if UCFG_WIN32
	if (-1 != ms) {
		tcp_keepalive ka = { 1, (ULONG)ms, 1000 };
		Ioctl(SIO_KEEPALIVE_VALS, &ka, sizeof ka);
	}
#else
	SetSocketOption(IPPROTO_TCP, TCP_KEEPIDLE, ms/1000);
#endif
	SetSocketOption(SOL_SOCKET, SO_KEEPALIVE, ms != -1);
}

IPEndPoint Socket::get_LocalEndPoint() {
	IPEndPoint ep;
	socklen_t len = sizeof(sockaddr_in6);
	SocketCheck(::getsockname(HandleAccess(_self), &ep.Address.m_sockaddr, &len));
	return ep;
}

IPEndPoint Socket::get_RemoteEndPoint() {
	IPEndPoint ep;
	socklen_t len = sizeof(sockaddr_in6);
	SocketCheck(::getpeername(HandleAccess(_self), &ep.Address.m_sockaddr, &len));
	return ep;
}

size_t NetworkStream::Read(void *buf, size_t count) const {
	int n = m_sock.Receive(buf, (int)count);
	if (!n)
		m_bEof = true;
	return n;
}

void NetworkStream::WriteBuffer(const void *buf, size_t count) {
	int flags = 0;
#if UCFG_USE_POSIX
	flags |= NoSignal ? MSG_NOSIGNAL : 0;
#endif
	while (count) {
		if (int n = m_sock.Send(buf, (int)count, flags)) {
			if (n < 0)
				Throw(E_FAIL);
			count -= n;
			(const uint8_t*&)buf += n;
		} else if (count)
			Throw(E_FAIL);
	}
}

bool NetworkStream::Eof() const {
	return m_bEof;
}

void NetworkStream::Close() const {
	m_sock.Close();
}

void CSocketLooper::Send(Socket& sock, RCSpan mb) {
	NetworkStream stm(sock);
	stm.NoSignal = NoSignal;
	stm.Write(mb);
}

void CSocketLooper::Loop(Socket& sockS, Socket& sockD) {
	TRC(5, "");

	DBG_LOCAL_IGNORE_CONDITION(errc::connection_reset);
	DBG_LOCAL_IGNORE_CONDITION(errc::connection_aborted);

	class CLoopKeeper {
	public:
		CSocketLooper& m_socketLooper;
		Socket &m_sock, &m_sockOther;
		Socket::BlockingHandleAccess m_hp;
		bool m_bLive, m_bAccepts, m_bIncoming;

		CLoopKeeper(CSocketLooper& socketLooper, Socket& sock, Socket& sockOther, bool bIncoming = false)
			: m_socketLooper(socketLooper)
			, m_sock(sock)
			, m_hp(m_sock)
			, m_sockOther(sockOther)
			, m_bLive(true)
			, m_bAccepts(true)
			, m_bIncoming(bIncoming)
		{}

		bool Process(fd_set *fdset) {
			if (FD_ISSET((intptr_t)m_hp, fdset)) {
				uint8_t buf[BUF_SIZE];
				int r = m_sock.Receive(buf, sizeof buf);
				if (m_bLive = r) {
					bool bDisconnectAfterData = false;
					Span cbuf = Span(buf, r);
					Blob blob = m_bIncoming ? m_socketLooper.ProcessDest(cbuf, bDisconnectAfterData) : m_socketLooper.ProcessSrc(cbuf, bDisconnectAfterData);
					if (!!blob)
						cbuf = blob;
					try {
						m_socketLooper.Send(m_sockOther, cbuf);
					} catch (RCExc) {
						m_bAccepts = false;
						if (m_socketLooper.m_bDoShutdown)
							m_sock.Shutdown(SHUT_RD);
					}
					if (bDisconnectAfterData)
						return false;
				} else {
					if (m_socketLooper.m_bDoShutdown)
						m_sockOther.Shutdown(SHUT_WR);
				}

			}
			return true;
		}
	};

	CLoopKeeper loopS(_self, sockS, sockD, true),
				loopD(_self, sockD, sockS);
	while (loopS.m_bLive && loopS.m_bAccepts ||
			loopD.m_bLive && loopD.m_bAccepts)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		int nfds = 0;
		if (loopS.m_bLive && loopS.m_bAccepts) {
			FD_SET((SOCKET)loopS.m_hp, &readfds);
			nfds = std::max(nfds, int(1+(SOCKET)loopS.m_hp));
		}
		if (loopD.m_bLive && loopD.m_bAccepts) {
			FD_SET((SOCKET)loopD.m_hp, &readfds);
			nfds = std::max(nfds, int(1+(SOCKET)loopD.m_hp));
		}
		TimeSpan span = GetTimeout();
		timeval timeout,
			*pTimeout = 0;
		if (span.count() != -1) {
			pTimeout = &timeout;
			span.ToTimeval(timeout);
		}
		if (NeedToCancel())
			break;
		if (!SocketCheck(::select(nfds, &readfds, 0, 0, pTimeout))) {
			TRC(1, "Session timed-out!");
			break;
		}
		if (!loopS.Process(&readfds) || !loopD.Process(&readfds))
			break;
	}
}



} // Ext::

#if UCFG_WIN32
#	undef NTDDI_VERSION
#	define NTDDI_VERSION 0x05000000
#	include <WS2tcpip.h>
#	undef gai_strerror
extern "C" char *gai_strerror(int ecode) {
	return gai_strerrorA(ecode);	//!!!?
}
#endif


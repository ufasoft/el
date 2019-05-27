/*######   Copyright (c) 2013-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/inet/proxy-client.h>
#include <el/libext/conf.h>

#include "p2p-peers.h"

#ifndef UCFG_P2P_SEND_THREAD
#	define UCFG_P2P_SEND_THREAD 0
#endif

namespace Ext { namespace Inet { namespace P2P {
class Net;
class Link;
}}} // namespace Ext::Inet::P2P

namespace Ext {
template <> struct ptr_traits<Ext::Inet::P2P::Link> { typedef InterlockedPolicy interlocked_policy; };
} // namespace Ext

namespace Ext { namespace Inet { namespace P2P {

const int P2P_CONNECT_TIMEOUT = 5000;
const int PERIODIC_SECONDS = 10;
const int PERIODIC_SEND_SECONDS = 10;

class Message : public Object, public CPersistent {
public:
	typedef InterlockedPolicy interlocked_policy;

	DateTime Timestamp;
	ptr<P2P::Link> LinkPtr;

	virtual void ProcessMsg(P2P::Link& link) {}
};

#if UCFG_P2P_SEND_THREAD

class LinkSendThread : public SocketThread {
	typedef SocketThread base;

public:
	P2P::Link& Link;
	AutoResetEvent m_ev;

	LinkSendThread(P2P::Link& link);

	void Stop() override {
		base::Stop();
		m_ev.Set();
	}

protected:
#	if UCFG_WIN32
	void OnAPC() override {
		base::OnAPC();
		Socket::ReleaseFromAPC();
	}
#	endif

	void Execute() override;
};

#endif // UCFG_P2P_SEND_THREAD

class Link : public LinkBase {
	typedef LinkBase base;

public:
	typedef InterlockedPolicy interlocked_policy;

	observer_ptr<P2P::Net> Net;
#if UCFG_P2P_SEND_THREAD
	ptr<LinkSendThread> SendThread;
#endif

	vector<ptr<Message>> OutQueue;
	ProxyClient Tcp;
	DateTime m_dtCheckLastRecv, m_dtLastRecv, m_dtLastSend;
	DateTime DtStallingSince; // DateTime, when stalling of providing requested info was detected
	Blob DataToSend;

	DateTime LastPingTimestamp;
	TimeSpan PingTimeout, MinPingTime;
		
	TimeSpan TimeOffset;
	int FirstByte;
	int PeerVersion;
	CBool UseMagic;
	CBool LineBased; //!!!TODO
	CBool IsOneShot; // seed
	CBool Whitelisted;

	Link(P2P::NetManager* netManager, thread_group* tr)
		: base(netManager, tr)
		, PingTimeout(TimeSpan::FromMinutes(20))
		, FirstByte(-1)
		, PeerVersion(0)
		, UseMagic(true)
		, MinPingTime(TimeSpan::MaxValue)
	{
	}

	virtual void SendBinary(RCSpan buf);
	virtual void Send(ptr<P2P::Message> msg); // ptr<> to prevent Memory Leak in Send(new Message) construction
	virtual size_t GetMessageHeaderSize();
	virtual size_t GetMessagePayloadSize(RCSpan buf);
	virtual ptr<Message> RecvMessage(const BinaryReader& rd);
	virtual void OnMessage(Message* m);
	virtual void ReceiveAndProcessMessage(const BinaryReader& rd);
	virtual void ReceiveAndProcessLineMessage(RCSpan bufLine);
	virtual void OnCloseLink();
	virtual void OnPingTimeout();

	void OnSelfLink();

	void Stop() override;

protected:
#if UCFG_WIN32
	void OnAPC() override {
		base::OnAPC();
		Socket::ReleaseFromAPC();
	}
#endif

	void BeforeStart() override;
	void Execute() override;
	virtual void OnPeriodic(const DateTime& now) {}

	virtual bool OnStartConnection() { return true; }

	friend class LinkSendThread;
};

class ListeningThread : public SocketThread {
	typedef SocketThread base;

public:
	P2P::NetManager& NetManager;

	ListeningThread(P2P::NetManager& netManager, thread_group& tr, AddressFamily af = AddressFamily::InterNetwork);

	static void StartListener(P2P::NetManager& netManager, thread_group& tr, AddressFamily family);
	static void StartListeners(P2P::NetManager& netManager, thread_group& tr);
protected:
	void BeforeStart() override;
	void Execute() override;

private:
	AddressFamily m_af;
	Socket m_sock;

	void ChosePort();
};

class Net : public PeerManager {
	typedef PeerManager base;

public:
	thread_group m_tr;
	TimeSpan StallingTimeout;
	CBool Runned;
	uint32_t ProtocolMagic;
	bool Listen;

	Net(P2P::NetManager& netManager);

	virtual void Start() {
		PeerManager::m_owner.reset(&m_tr);
		//!!!		(MsgThread = new MsgLoopThread(_self, tr))->Start();

		Runned = true;
	}

	bool IsRandomlyTrickled() { return EXT_LOCKED(MtxPeers, Ext::Random().Next(Links.size()) == 0); }

	//	ptr<Peer> GetPeer(const IPEndPoint& ep);
	//	virtual void Send(Link& link, Message& msg) =0;

protected:
	virtual size_t GetMessageHeaderSize() = 0;
	virtual size_t GetMessagePayloadSize(RCSpan buf) = 0;
	virtual ptr<Message> RecvMessage(Link& link, const BinaryReader& rd) = 0;
	virtual void OnPingTimeout(Link& link) {}
	virtual void OnInitLink(Link& link) {}

	virtual void OnInitMsgLoop() {}
	virtual void OnCloseMsgLoop() {}
	virtual void OnMessage(Message* m) {}

	virtual void OnPeriodicMsgLoop(const DateTime& now) { PeerManager::OnPeriodic(now); }

	friend class Link;
	friend class MsgLoopThread;
};

class P2PConf : public Conf {
public:
	int MaxConnections;
	int Port;
	String ProxyString, OnlyNet;
	vector<String> Connect;
	bool Listen;

	P2PConf();
	static P2PConf*& Instance();
};



}}} // namespace Ext::Inet::P2P

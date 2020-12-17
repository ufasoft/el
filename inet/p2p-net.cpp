/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


#include "p2p-net.h"

namespace Ext { namespace Inet { namespace P2P {

const int INACTIVE_PEER_SECONDS = 90*60;

#if UCFG_P2P_SEND_THREAD

LinkSendThread::LinkSendThread(P2P::Link& link)
	:	Link(link)
	,	base(link.m_owner)
{
}

void LinkSendThread::Execute() {
	Name = "LinkSendThread";

	DateTime dtLast;

	while (!m_bStop) {
		vector<ptr<Message>> messages;
		EXT_LOCK (Link.m_cs) {
			Link.OutQueue.swap(messages);
		}
		DateTime now = DateTime::UtcNow();
		if (messages.empty()) {
			if (now-dtLast > TimeSpan::FromSeconds(PERIODIC_SEND_SECONDS)) {
				Link.OnPeriodic();
				dtLast = now;

				EXT_LOCK (Link.m_cs) {
					if (now-Link.Peer->LastLive > TimeSpan::FromSeconds(INACTIVE_PEER_SECONDS))
						Link.Stop();
				}
			} else {
				if (now-m_dtLastSend > seconds(PING_SECONDS))
					if (ptr<Message> m = Link.CreatePingMessage())
						Link.Send(m);
				m_ev.Lock(PERIODIC_SEND_SECONDS * 1000);
			}
		} else {
			for (int i=0; i<messages.size(); ++i)
				Link.Net.SendMessage(Link, *messages[i]);
			m_dtLastSend = now;
		}
	}
}

#endif // UCFG_P2P_SEND_THREAD

void Link::SendBinary(const ConstBuf& buf) {
	EXT_LOCK (Mtx) {
		m_dtLastSend = DateTime::UtcNow();
		bool bHasToSend = DataToSend.Size;
		DataToSend += buf;
		if (!bHasToSend) {
			int rc = Tcp.Client.Send(DataToSend.constData(), DataToSend.Size);
			if (rc >= 0)
				DataToSend.Replace(0, rc, ConstBuf(0, 0));
		}
	}
}

void Link::Send(ptr<P2P::Message> msg) {
#if UCFG_P2P_SEND_THREAD
	EXT_LOCK (Mtx) {
		OutQueue.push_back(msg);
		SendThread->m_ev.Set();
	}
#else
	SendBinary(EXT_BIN(*msg));
#endif
}

void Link::OnSelfLink() {
	Stop();

	EXT_LOCK (Net->MtxPeers) {
		this->NetManager->AddLocal(Peer->get_EndPoint().Address);
	}
}

size_t Link::GetMessageHeaderSize() {
	return Net ? Net->GetMessageHeaderSize() : 1;
}

size_t Link::GetMessagePayloadSize(const ConstBuf& buf) {
	return Net ? Net->GetMessagePayloadSize(buf) : 0;
}

ptr<Message> Link::RecvMessage(const BinaryReader& rd) {
	return Net ? Net->RecvMessage(_self, rd) : nullptr;
}

void Link::OnMessageReceived(Message *m) {
	if (Net)
		Net->OnMessageReceived(m);
}

void Link::ReceiveAndProcessMessage(const BinaryReader& rd) {
	ptr<Message> msg;
	try {
//!!!				DBG_LOCAL_IGNORE_NAME(E_EXT_Protocol_Violation, ignE_EXT_Protocol_Violation);

		msg = RecvMessage(rd);
	} catch (RCExc&) {
		if (Peer)
			Peer->Misbehavings += 100;
		if (NetManager)
			NetManager->BanPeer(*Peer);
		throw;
	}
	msg->Link = this;
	DateTime now = DateTime::UtcNow();
	EXT_LOCK (Mtx) {
		m_dtLastRecv = now;
		if (Peer)
			Peer->LastLive = now;
	}
	OnMessageReceived(msg);
}

void Link::ReceiveAndProcessLineMessage(const ConstBuf& bufLine) {
}

void Link::OnPingTimeout() {
	if (Net)
		Net->OnPingTimeout(_self);
}

void Link::OnCloseLink() {
	if (Net)
		Net->OnCloseLink(_self);
}

void Link::BeforeStart() {
//!!!R?	if (!Incoming)
//!!!R		Tcp.Client.Create(Peer->get_EndPoint().Address.AddressFamily, SocketType::Stream, ProtocolType::Tcp);
}

void Link::Execute() {
	Name = "LinkThread";

	IPEndPoint epRemote;
	try {
		if (Incoming)
			epRemote = Tcp.Client.RemoteEndPoint;

		DBG_LOCAL_IGNORE_CONDITION(errc::connection_refused);	
		DBG_LOCAL_IGNORE_CONDITION(errc::connection_aborted);
		DBG_LOCAL_IGNORE_CONDITION(errc::connection_reset);
		DBG_LOCAL_IGNORE_CONDITION(errc::not_a_socket);

		if (!OnStartConnection())
			goto LAB_EOF;

		bool bMagicReceived = false;
		if (Incoming) {
			DBG_LOCAL_IGNORE_CONDITION(ExtErr::EndOfStream);

			if (UseMagic) {
				uint32_t magic = BinaryReader(Tcp.Stream).ReadUInt32();
				EXT_LOCK (NetManager->MtxNets) {
					EXT_FOR (P2P::Net *net, NetManager->m_nets) {
						if (net->Listen && net->ProtocolMagic == magic) {
							Net.reset(net);
							goto LAB_FOUND;
						}
					}
					return;
LAB_FOUND:
					ThreadBase::Delete();
					m_owner.reset(&Net->m_tr);
					m_owner->add_thread(this);
				}
				bMagicReceived = true;
			}

			if (Net) {
				Peer = Net->CreatePeer();						// temporary
				Peer->EndPoint = epRemote;
			}
		} else {
			epRemote = Peer->EndPoint;
			TRC(3, "Connecting to " << epRemote);
			DBG_LOCAL_IGNORE_CONDITION(errc::timed_out);
			DBG_LOCAL_IGNORE_CONDITION(errc::address_not_available);
			DBG_LOCAL_IGNORE_CONDITION(errc::network_unreachable);
			DBG_LOCAL_IGNORE_CONDITION(errc::host_unreachable);
			
			if (Tcp.ProxyString.empty()) {
				Tcp.Client.Create(Peer->get_EndPoint().Address.AddressFamily, SocketType::Stream, ProtocolType::Tcp);
				Tcp.Client.ReuseAddress = true;
				Tcp.Client.Bind(epRemote.Address.AddressFamily==AddressFamily::InterNetwork ? IPEndPoint(IPAddress::Any, NetManager->LocalEp4.Port) : IPEndPoint(IPAddress::IPv6Any, NetManager->LocalEp6.Port));
				Tcp.Client.ReceiveTimeout = P2P_CONNECT_TIMEOUT;	//!!!?
			}
#ifdef X_DEBUG//!!!D
			epRemote = IPEndPoint::Parse("192.168.0.103:8668");
#endif
			Tcp.Connect(epRemote);
			Tcp.Client.ReceiveTimeout = 0;
			m_dtCheckLastRecv = DateTime::UtcNow()+TimeSpan::FromMinutes(1);
			Net->Attempt(Peer);
		}
		if (Net)
			Net->AddLink(this);

#if UCFG_P2P_SEND_THREAD
		(SendThread = new LinkSendThread(_self))->Start();
#endif

		epRemote = Tcp.Client.RemoteEndPoint;

		if (Net)
			Net->OnInitLink(_self);
		Tcp.Client.Blocking = false;

		DateTime dtNextPeriodic = DateTime::UtcNow() + seconds(P2P::PERIODIC_SEND_SECONDS);
		const size_t cbHdr = GetMessageHeaderSize();
		Blob blobMessage(0, cbHdr);
		bool bReceivingPayload = false;
		size_t cbReceived = 0;
		if (FirstByte != -1) {
			cbReceived = 1;
			blobMessage.Size = std::max(size_t(blobMessage.Size), size_t(1));
			blobMessage.data()[0] = (byte)exchange(FirstByte, -1);
		}

		if (bMagicReceived) {
			cbReceived = sizeof(uint32_t);
			*(uint32_t*)blobMessage.data() = Net->ProtocolMagic;
		}

		if (LineBased)
			blobMessage.Size = std::max(size_t(blobMessage.Size), size_t(1024));

		Socket::BlockingHandleAccess hp(Tcp.Client);

		timeval timevalTimeOut;
		TimeSpan::FromSeconds(std::min(std::min((int)duration_cast<seconds>(PingTimeout).count(), INACTIVE_PEER_SECONDS), P2P::PERIODIC_SEND_SECONDS)).ToTimeval(timevalTimeOut);
		while (!m_bStop) {
			bool bHasToSend = EXT_LOCKED(Mtx, DataToSend.Size);
			fd_set readfds, writefds;
			FD_ZERO(&readfds);
			FD_ZERO(&writefds);
			FD_SET((SOCKET)hp, &readfds);
			FD_SET((SOCKET)hp, &writefds);

			timeval timeout = timevalTimeOut;
			SocketCheck(::select(int(1+(SOCKET)hp), &readfds, (bHasToSend ? &writefds : 0), 0, &timeout));

			DateTime now = DateTime::UtcNow();

			if (FD_ISSET(hp, &readfds)) {
				while (!m_bStop) {
				    if (LineBased) {
						const byte *p = blobMessage.constData();
						for (const byte *q; q = (const byte*)memchr(p, '\n', cbReceived); ) {
							size_t cbMsg = q - p + 1;
							ReceiveAndProcessLineMessage(ConstBuf(p, cbMsg));
							memmove(blobMessage.data(), blobMessage.data()+cbMsg, cbReceived -= cbMsg);
						}						
					}					

					int rc = Tcp.Client.Receive(blobMessage.data()+cbReceived, blobMessage.Size-cbReceived);
					if (!rc)
						goto LAB_EOF;
					if (rc < 0)
						break;
					cbReceived += rc;
				
					if (!LineBased && cbReceived == blobMessage.Size) {
						do {
							if (!bReceivingPayload) {
								bReceivingPayload = true;
								if (size_t cbPayload = GetMessagePayloadSize(blobMessage)) {
									blobMessage.put_Size(blobMessage.Size + cbPayload);
									break;
								}
							}					
							if (bReceivingPayload) {
								CMemReadStream stm(blobMessage);
								ReceiveAndProcessMessage(BinaryReader(stm));

								blobMessage.Size = cbHdr;
								bReceivingPayload = false;
								cbReceived = 0;
							}
						} while (false);
					}
				}
			}

			if (m_bStop)
				break;

			if (bHasToSend && FD_ISSET(hp, &writefds)) {
				EXT_LOCK (Mtx) {
					while (DataToSend.Size) {
						int rc = Tcp.Client.Send(DataToSend.constData(), DataToSend.Size);
						if (rc < 0)
							break;
						DataToSend.Replace(0, rc, ConstBuf(0, 0));
					}
				}
			}

			EXT_LOCK (Mtx) {
				if (Peer && now - Peer->LastLive > seconds(INACTIVE_PEER_SECONDS))
					break;
			}
			if (now > dtNextPeriodic) {
				OnPeriodic();
				dtNextPeriodic =  now + seconds(P2P::PERIODIC_SEND_SECONDS);
			}
			if (now-m_dtLastSend > PingTimeout)
				OnPingTimeout();
		}
	} catch (RCExc) {
	}
LAB_EOF:
	TRC(3, "Disconnecting " << epRemote << "  Socket " << (int64_t)Tcp.Client.DangerousGetHandleEx());
#if UCFG_P2P_SEND_THREAD
	if (SendThread) {
		if (!SendThread->m_bStop)
			SendThread->interrupt();
		SendThread->Join();
	}
#endif
	OnCloseLink();
}

void Link::Stop() {
#if UCFG_P2P_SEND_THREAD
	if (SendThread)
		SendThread->Stop();
#endif
	base::Stop();
#if UCFG_WCE
	try {
		Tcp.Client.Shutdown();
	} catch (RCExc) {
	}
#endif
	Tcp.Client.Close();
#if UCFG_WIN32_FULL
	QueueAPC();
#endif
}

ListeningThread::ListeningThread(P2P::NetManager& netManager, thread_group& tr, AddressFamily af)
	:	base(&tr)
	,	NetManager(netManager)
	,	m_af(af)
{
//	StackSize = UCFG_THREAD_STACK_SIZE;
}

void ListeningThread::StartListeners(P2P::NetManager& netManager, thread_group& tr) {
	DBG_LOCAL_IGNORE_CONDITION(errc::address_family_not_supported);

	if (Socket::OSSupportsIPv4) {
		try {
			(new P2P::ListeningThread(netManager, tr, AddressFamily::InterNetwork))->Start();
		} catch (RCExc) {
		}
	}
	if (Socket::OSSupportsIPv6) {
		try {
			(new P2P::ListeningThread(netManager, tr, AddressFamily::InterNetworkV6))->Start();
		} catch (RCExc) {
		}
	}
}

void ListeningThread::BeforeStart() {
	if (m_af == AddressFamily::InterNetwork) {
		{
			Socket sock;
			sock.Create(m_af, SocketType::Stream, ProtocolType::Tcp);
			if (NetManager.SoftPortRestriction) {
				try {
					DBG_LOCAL_IGNORE_CONDITION(errc::address_in_use);

					sock.Bind(IPEndPoint(IPAddress::Any, (uint16_t)NetManager.ListeningPort));
				} catch (system_error& ex) {
					if (ex.code() != errc::address_in_use)
						throw;
					sock.Bind(IPEndPoint(IPAddress::Any, 0));
				}
				NetManager.ListeningPort = sock.get_LocalEndPoint().Port;
			} else {
				sock.Bind(IPEndPoint(IPAddress::Any, (uint16_t)NetManager.ListeningPort));
			}
		}
	
		m_sock.Create(m_af, SocketType::Stream, ProtocolType::Tcp);
		m_sock.ReuseAddress = true;
		m_sock.Bind(IPEndPoint(IPAddress::Any, (uint16_t)NetManager.ListeningPort));
		TRC(2, "Listening on TCP IPv4 port " << NetManager.ListeningPort);
	} else {
		m_sock.Create(m_af, SocketType::Stream, ProtocolType::Tcp);
		m_sock.ReuseAddress = true;
		m_sock.Bind(IPEndPoint(IPAddress::IPv6Any, (uint16_t)NetManager.ListeningPort));
		NetManager.LocalEp6 = m_sock.LocalEndPoint;
		TRC(2, "Listening on TCP IPv6 port " << NetManager.ListeningPort);
	}

	m_sock.Listen();
}

void ListeningThread::Execute() {
	Name = "ListeningThread";

	try {
		SocketKeeper sockKeeper(_self, m_sock);
		DBG_LOCAL_IGNORE_CONDITION(errc::interrupted);

		while (!Ext::this_thread::interruption_requested()) {
			pair<Socket, IPEndPoint> sep = m_sock.Accept();
			if (NetManager.IsBanned(sep.second.Address)) {
				TRC(2, "Denied connect from banned " << sep.second.Address);
			} else if (NetManager.IsTooManyLinks()) {
				TRC(2, "Incoming connection refused: Too many links");
			} else {
				TRC(3, "Connected from " << sep.second << "  Socket " << sep.first.DangerousGetHandle());

				ptr<Link> link = NetManager.CreateLink(*m_owner);
				link->Incoming = true;
				link->Tcp.Client = move(sep.first);
				link->Start();
			}
		}
	} catch (RCExc) {
	}
}

Link *NetManager::CreateLink(thread_group& tr) {
	return new Link(this, &tr);
}

bool NetManager::IsTooManyLinks() {
	int links = 0, limSum = 0;
	EXT_LOCK (MtxNets) {
		for (int i=0; i<m_nets.size(); ++i) {
			PeerManager& pm = *m_nets[i];
			EXT_LOCK (pm.MtxPeers) {
				links += pm.Links.size();
			}
			limSum += pm.MaxLinks;
		}
		return links >= limSum;
	}
}

}}} // Ext::Inet::P2P::


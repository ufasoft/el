/*######   Copyright (c) 2013-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <random>

#include "p2p-peers.h"
#include "p2p-net.h"

namespace Ext { namespace Inet { namespace P2P {

P2PConf g_Conf, *g_pConf = &g_Conf;

P2PConf*& P2PConf::Instance() {
	return g_pConf;
}

bool Peer::IsTerrible(const DateTime& now) const {
	if (get_LastTry().Ticks && (now - LastTry) < TimeSpan::FromMinutes(1))
		return false;
	return !get_LastPersistent().Ticks
		|| (now - get_LastPersistent()) > TimeSpan::FromDays(30)
		|| !get_LastLive().Ticks && Attempts>=3
		|| (now - LastLive) > TimeSpan::FromDays(7) && Attempts>=10;
}

double Peer::GetChance(const DateTime& now) const {
	TimeSpan sinceLastSeen = std::max(TimeSpan(), now - LastPersistent);

	double r = (double)duration_cast<seconds>(sinceLastSeen).count();
	r = 1 - r / (600 + r);
	if (now - LastTry < TimeSpan::FromMinutes(10))
		r /= 100;
	return r * ::pow(0.6, (int)Attempts);
}

Blob Peer::GetGroup() const {
	vector<uint8_t> v;
	const IPAddress& ip = get_EndPoint().Address;
	Blob blob = ip.GetAddressBytes();
	if (4 == blob.Size) {
		v.push_back(4);
		v.push_back(blob.data()[0]);
		v.push_back(blob.data()[1]);
	} else if (ip.IsIPv4MappedToIPv6) {
		v.push_back(4);
		v.push_back(blob.data()[12]);
		v.push_back(blob.data()[13]);
	} else if (ip.IsIPv6Teredo) {
		v.push_back(4);
		v.push_back(blob.data()[12] ^ 0xFF);
		v.push_back(blob.data()[13] ^ 0xFF);
	} else {
		v.push_back(6);
		v.push_back(blob.data()[0]);
		v.push_back(blob.data()[1]);
		v.push_back(blob.data()[2]);
		v.push_back(blob.data()[3]);
	}
	return Blob(&v[0], v.size());
}

size_t Peer::GetHash() const {
	return hash<Blob>()(GetGroup()) + hash<IPEndPoint>()(EndPoint);
}

void PeerManager::Remove(Peer *peer) {
	IpToPeer.erase(peer->get_EndPoint().Address);
	Ext::Remove(VecRandom, peer);
}

void PeerBucket::Shrink() {
	DateTime now = Clock::now();
	ptr<Peer> peerOldest;
	for (int i=0; i<m_peers.size(); ++i) {
		ptr<Peer>& peer = m_peers[i];
		if (peer->IsTerrible(now)) {
			Buckets->Manager.Remove(peer.get());
			m_peers.erase(m_peers.begin()+i);
			return;
		}
		if (!peerOldest || peer->LastPersistent < peerOldest->LastPersistent)
			peerOldest = peer;
	}
	Buckets->Manager.Remove(peerOldest);
	Remove(m_peers, peerOldest);
}

size_t PeerBuckets::size() const {
	size_t r = 0;
	for (int i=0; i<m_vec.size(); ++i)
		r += m_vec[i].m_peers.size();
	return r;
}

bool PeerBuckets::Contains(Peer *peer) {
	ptr<Peer> p = peer;
	for (int i=0; i<m_vec.size(); ++i)
		if (ContainsInLinear(m_vec[i].m_peers, p))
			return true;
	return false;
}

ptr<Peer> PeerBuckets::Select() {
	Ext::Random rng;
	DateTime now = Clock::now();
	for (double fac=1;;) {
		PeerBucket& bucket = m_vec[rng.Next(m_vec.size())];
		if (!bucket.m_peers.empty()) {
			const ptr<Peer>& p = bucket.m_peers[rng.Next(bucket.m_peers.size())];
			if (p->GetChance(now)*fac > 1)
				return p;
			fac *= 1.2;
		}
	}
}

vector<ptr<Peer>> PeerManager::GetPeers(int nMax) {
	EXT_LOCK (MtxPeers) {
		default_random_engine rng(Rand());
		std::shuffle(VecRandom.begin(), VecRandom.end(), rng);
		return vector<ptr<Peer>>(VecRandom.begin(), VecRandom.begin()+std::min(size_t(nMax), VecRandom.size()));
	}
}

ptr<Peer> PeerManager::Select() {
	int size = TriedBuckets.size() + NewBuckets.size();
	if (!size)
		return nullptr;

	default_random_engine rng(Rand());
	PeerBuckets& buckets = uniform_int_distribution<int>(0, size - 1)(rng) < TriedBuckets.size() ? TriedBuckets : NewBuckets;
	ptr<Peer> r = buckets.Select();
	if (r) {
		TRC(2, "Selected Peer: " << r->EndPoint);
	}
	return r;
}

void PeerManager::Attempt(Peer *peer) {
	EXT_LOCK (MtxPeers) {
		peer->LastTry = Clock::now();
		peer->Attempts++;
	}
}

void PeerManager::Good(Peer *peer) {
	EXT_LOCK (MtxPeers) {
		DateTime now = Clock::now();
		peer->LastTry = now;
		peer->LastLive = now;
		peer->LastPersistent = now;
		peer->Attempts = 0;
		if (!TriedBuckets.Contains(peer)) {
			ptr<Peer> p = peer;
			for (int i=0; i<NewBuckets.m_vec.size(); ++i)
				Ext::Remove(NewBuckets.m_vec[i].m_peers, p);

			PeerBucket& bucket = TriedBuckets.m_vec[peer->GetHash() % TriedBuckets.m_vec.size()];
			if (bucket.m_peers.size() < ADDRMAN_TRIED_BUCKET_SIZE) {
				bucket.m_peers.push_back(peer);
				return;
			}

			ptr<Peer> peerOldest;
			for (int i=0; i<bucket.m_peers.size(); ++i) {
				const ptr<Peer>& p = bucket.m_peers[i];
				if (!peerOldest || p->LastPersistent < peerOldest->LastPersistent)
					peerOldest = p;
			}
			Ext::Remove(bucket.m_peers, peerOldest);
			NewBuckets.m_vec[peerOldest->GetHash() % NewBuckets.m_vec.size()].m_peers.push_back(peerOldest);
		}
	}
}

ptr<Peer> PeerManager::Find(const IPAddress& ip) {
	CPeerMap::iterator it = IpToPeer.find(ip);
	return it!=IpToPeer.end() ? it->second : nullptr;
}


/*
void PeerManager::AddPeer(Peer& peer) {
	TRC(3, peer.EndPoint);


	bool bSave = false;
	EXT_LOCK (MtxPeers) {
		auto pp = Peers.insert(CPeerMap::value_type(peer.EndPoint.Address, &peer));
		if (pp.second)
			m_nPeersDirty = true;
		else {
			Peer& peerFound = *pp.first->second;
			if ((peer.LastLive-peerFound.LastLive).TotalHours > ((Clock::now()-peer.LastLive).TotalHours < 24 ? 1 : 24)) {
				m_nPeersDirty = true;
				peerFound.LastLive = peer.LastLive;
			}
		}
	}
}
*/

bool PeerManager::IsRoutable(const IPAddress& ip) {
	return !ip.IsEmpty()
		&& ip.IsGlobal()
		&& !NetManager.IsLocal(ip);
}

ptr<Peer> PeerManager::Add(const IPEndPoint& ep, uint64_t services, DateTime dt, TimeSpan penalty, bool bRequireRoutable) {
//	TRC(3, ep);

	if (bRequireRoutable && !IsRoutable(ep.Address))
		return nullptr;

	if (NetManager.IsBanned(ep.Address)) {
		TRC(2, "Banned peer ignored " << ep);
		return nullptr;
	}

	m_aPeersDirty = true;
	ptr<Peer> peer;
	EXT_LOCK (MtxPeers) {
		if (peer = Find(ep.Address)) {
			if (dt.Ticks)
				peer->LastPersistent = std::max(DateTime(), dt - penalty);
			peer->put_Services(peer->get_Services() | services);
			if (TriedBuckets.Contains(peer))
				return nullptr;
		} else {
			peer = CreatePeer();
			peer->EndPoint = ep;
			peer->Services = services;
			peer->LastPersistent = std::max(DateTime(), dt - penalty);
			IpToPeer.insert(make_pair(ep.Address, peer));
			VecRandom.push_back(peer.get());
		}
		PeerBucket& bucket = NewBuckets.m_vec[peer->GetHash() % NewBuckets.m_vec.size()];
		if (!ContainsInLinear(bucket.m_peers, peer)) {
			if (bucket.m_peers.size() >= ADDRMAN_NEW_BUCKET_SIZE)
				bucket.Shrink();
			bucket.m_peers.push_back(peer);
		}
	}
	peer->IsDirty = true;
	return peer;
}

void PeerManager::OnPeriodic(const DateTime& now) {
	EXT_LOCK (MtxPeers) {
		int nOutgoing = 0;
		for (int i = 0; i < Links.size(); ++i) {
			Link& link = *static_cast<Link*>(Links[i].get());
			if (link.m_dtLastRecv == DateTime() && now > link.m_dtCheckLastRecv)
				link.Stop();
			nOutgoing += !link.Incoming;
		}

		if (Links.size() >= MaxLinks)
			return;

		unordered_set<Blob> setConnectedSubnet;
		for (int i = 0; i < Links.size(); ++i)
			setConnectedSubnet.insert(Links[i]->Peer->GetGroup());

		for (int n = std::max(MaxOutboundConnections - nOutgoing, 0); n--;) {
			if (ptr<Peer> peer = Select()) {
				if (setConnectedSubnet.insert(peer->GetGroup()).second) {
					ptr<Link> link = NetManager.CreateLink(*m_owner);
					link->Net.reset(dynamic_cast<Net*>(this));
					link->Peer = peer;
					peer->LastTry = now;
					try {
						link->Start();
					} catch (RCExc) {
//						TRC(3, ex.what());		// TRC crashes in inet_ntop() if IPv6 disabled
					}
				} else {
					TRC(3, "Subnet already connected for " << peer->get_EndPoint().Address);
				}
			}
		}
	}

	int expected = true;
	if (m_aPeersDirty.compare_exchange_weak(expected, false))
		SavePeers();
}

PeerManager::PeerManager(P2P::NetManager& netManager)
	: NetManager(netManager)
	, MaxLinks(MAX_LINKS)
	, MaxOutboundConnections(P2PConf::Instance()->MaxConnections) //!!!
	, m_aPeersDirty(0)
	, DefaultPort(0)
	, TriedBuckets(_self, ADDRMAN_TRIED_BUCKET_COUNT)
	, NewBuckets(_self, ADDRMAN_NEW_BUCKET_COUNT)
{
}

void PeerManager::AddLink(LinkBase *link) {
	EXT_LOCK (MtxPeers) {
		Links.push_back(link);
	}
}

void PeerManager::OnCloseLink(LinkBase& link) {
	EXT_LOCK (MtxPeers) {
		Ext::Remove(Links, &link);
	}
}


}}} // Ext::Inet::P2P::


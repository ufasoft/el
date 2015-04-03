/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


#include <el/libext/ext-net.h>

namespace Ext { namespace Inet { namespace P2P {

const int MAX_OUTBOUND_CONNECTIONS = 4;

const int MAX_LINKS = MAX_OUTBOUND_CONNECTIONS * 2;
const int MAX_PEER_MISBEHAVINGS = 100;

const int MAX_SAVED_PEERS = 1000;

class NetManager;
class Net;
class Link;
class PeerBuckets;
class PeerManager;

// total number of buckets for tried addresses
#define ADDRMAN_TRIED_BUCKET_COUNT 64

// maximum allowed number of entries in buckets for tried addresses
#define ADDRMAN_TRIED_BUCKET_SIZE 64

// total number of buckets for new addresses
#define ADDRMAN_NEW_BUCKET_COUNT 256

// maximum allowed number of entries in buckets for new addresses
#define ADDRMAN_NEW_BUCKET_SIZE 64


class Peer : public Object, public CPersistent {
	typedef Peer class_type;
public:
	typedef InterlockedPolicy interlocked_policy;

	CInt<int> Misbehavings;
	CInt<int> Attempts;
	CBool IsDirty;

	Peer()
		:	m_services(0)
	{}

	void Write(BinaryWriter& wr) const override {
		wr << m_endPoint << get_LastPersistent() << m_services;
	}

	void Read(const BinaryReader& rd) override {
		DateTime lastPersistent;
		rd >> m_endPoint >> lastPersistent >> m_services;
		LastPersistent = lastPersistent;
	}

	bool IsTerrible(const DateTime& now) const;
	double GetChance(const DateTime& now) const;
	Blob GetGroup() const;
	size_t GetHash() const;

	uint64_t get_Services() const { return m_services; }
	void put_Services(uint64_t v) {
		m_services = v;
		IsDirty = true;
	}
	DEFPROP(uint64_t, Services);

	IPEndPoint get_EndPoint() const { return m_endPoint; }
	void put_EndPoint(const IPEndPoint& ep) {
		m_endPoint = ep;
		IsDirty = true;
	}
	DEFPROP(IPEndPoint, EndPoint);

	DateTime get_LastLive() const { return m_lastLive; }
	void put_LastLive(const DateTime& dt) {
		m_lastLive = dt;
		IsDirty = true;
	}
	DEFPROP(DateTime, LastLive);

	DateTime get_LastPersistent() const { return m_lastPersistent; }
	void put_LastPersistent(const DateTime& dt) {
		m_lastPersistent = dt;
		IsDirty = true;
	}
	DEFPROP(DateTime, LastPersistent);

	DateTime get_LastTry() const { return m_lastTry; }
	void put_LastTry(const DateTime& dt) {
		m_lastTry = dt;
		IsDirty = true;
	}
	DEFPROP(DateTime, LastTry);

	bool get_Banned() const { return m_banned; }
	void put_Banned(bool v) {
		m_banned = v;
		IsDirty = true;
	}
	DEFPROP(bool, Banned);
protected:
	IPEndPoint m_endPoint;

	uint64_t m_services;	
	DateTime m_lastLive, 
			m_lastPersistent,
			m_lastTry;			// non-persistent
	
	CBool m_banned;
};

class LinkBase : public Thread {
	typedef Thread base;
public:
	typedef InterlockedPolicy interlocked_policy;
	
	observer_ptr<P2P::NetManager> NetManager;

	mutex Mtx;
	ptr<P2P::Peer> Peer;
	CBool Incoming;

	typedef unordered_set<ptr<P2P::Peer>> CSetPeersToSend;
	CSetPeersToSend m_setPeersToSend;

	LinkBase(P2P::NetManager *netManager, thread_group *tr)
		:	NetManager(netManager)
		,	base(tr)
	{
//		StackSize = UCFG_THREAD_STACK_SIZE;
	}

	void Push(P2P::Peer *peer) {
		EXT_LOCK (Mtx) {
			m_setPeersToSend.insert(peer);
		}
	}
};


class NetManager {
public:
	IPEndPoint LocalEp4, LocalEp6;

	mutex MtxNets;
	vector<Net*> m_nets;

	int ListeningPort;
	CBool SoftPortRestriction;

	NetManager()
		:	ListeningPort(0)
	{}

	virtual Link *CreateLink(thread_group& tr);

	virtual bool IsBanned(const IPAddress& ip) {
		return EXT_LOCKED(MtxBannedIPs, BannedIPs.count(ip));
	}

	virtual void BanPeer(Peer& peer) {
		peer.Banned = true;
		EXT_LOCKED(MtxBannedIPs, BannedIPs.insert(peer.get_EndPoint().Address));
	}

	virtual bool IsTooManyLinks();

	bool IsLocal(const IPAddress& ip) {
		return EXT_LOCKED(MtxLocalIPs, LocalIPs.count(ip));
	}

	void AddLocal(const IPAddress& ip) {
		EXT_LOCKED(MtxLocalIPs, LocalIPs.insert(ip));
	}

protected:
	mutex MtxLocalIPs;
	set<IPAddress> LocalIPs;

	mutex MtxBannedIPs;
	unordered_set<IPAddress> BannedIPs;
};


class PeerBucket {
public:
	observer_ptr<PeerBuckets> Buckets;

	vector<ptr<Peer>> m_peers;

	void Shrink();
};

class PeerBuckets {
public:
	PeerManager& Manager;

	vector<PeerBucket> m_vec;

	PeerBuckets(PeerManager& manager, size_t size)
		:	Manager(manager)
		,	m_vec(size)
	{
		for (int i=0; i<m_vec.size(); ++i)
			m_vec[i].Buckets.reset(this);
	}

	size_t size() const;
	bool Contains(Peer *peer);
	ptr<Peer> Select();
};


class PeerManager {
public:
	P2P::NetManager& NetManager;

	mutex MtxPeers;
	typedef vector<ptr<LinkBase>> CLinks;
	CLinks Links;

	int MaxLinks;
	int MaxOutboundConnections;
	
	PeerManager(P2P::NetManager& netManager)
		:	NetManager(netManager)
		,	MaxLinks(MAX_LINKS)
		,	MaxOutboundConnections(MAX_OUTBOUND_CONNECTIONS)
		,	m_aPeersDirty(0)
		,	DefaultPort(0)
		,	TriedBuckets(_self, ADDRMAN_TRIED_BUCKET_COUNT)
		,	NewBuckets(_self, ADDRMAN_NEW_BUCKET_COUNT)
	{}

	virtual void SavePeers() {}
//	void AddPeer(Peer& peer);

	virtual Peer *CreatePeer() {
		return new Peer;
	}

	virtual void AddLink(LinkBase *link);
	virtual void OnCloseLink(LinkBase& link);

	vector<ptr<Peer>> GetPeers(int nMax);
	ptr<Peer> Select();
	void Attempt(Peer *peer);
	void Good(Peer *peer);
	bool IsRoutable(const IPAddress& ip);
	ptr<Peer> Add(const IPEndPoint& ep, uint64_t services, DateTime dt, TimeSpan penalty = TimeSpan(0));

	vector<ptr<Peer>> GetAllPeers() {
		EXT_LOCK (MtxPeers) {
			return vector<ptr<Peer>>(VecRandom.begin(), VecRandom.end());
		}
	}
protected:
	uint16_t DefaultPort;
	observer_ptr<thread_group> m_owner;
	atomic<int> m_aPeersDirty;	

	virtual void OnPeriodic();
private:
	typedef unordered_map<IPAddress, ptr<Peer>> CPeerMap;
	CPeerMap IpToPeer;

	PeerBuckets TriedBuckets, NewBuckets;
	vector<Peer*> VecRandom;

	ptr<Peer> Find(const IPAddress& ip);
	void Remove(Peer *peer);


	friend class PeerBucket;
};


}}} // Ext::Inet::P2P::


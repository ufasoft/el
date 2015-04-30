/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/libext/ext-http.h>

#ifdef _AFXDLL
#	ifdef _LIBTOR
#		define AFX_TOR_CLASS __declspec(dllexport)
#	else
#		define AFX_TOR_CLASS __declspec(dllimport)
#	endif
#else
#	define AFX_TOR_CLASS
#endif

#ifndef _LIBTOR
#	pragma comment(lib, "tor")
#endif

namespace Ext {
namespace Inet {

struct TorParams {
	path torrc;
	String sPortSocks, sPortControl;
};

class TorProxy;

class AFX_TOR_CLASS TorThread : public Thread {
public:
	TorParams m_torParams;
	CBool Quiet;

	TorThread(thread_group& tr)
		:	Thread(&tr)
	{}

	~TorThread();
	void Stop() override;
	void Execute() override;
};

class AFX_TOR_CLASS TorStatusThread : public Thread {
public:
	CCriticalSection m_cs;
	queue<String> m_queue;
	AutoResetEvent m_ev;

	TorStatusThread(thread_group& tr, TorProxy& torProxy)
		:	Thread(&tr)
		,	TorProxy(torProxy)
	{}
private:
	TorProxy& TorProxy;

	typedef map<int, vector<String> > CCirMap;
	CCirMap m_curCircs;

	void BeforeStart() override;
	void Execute() override;
};


class AFX_TOR_CLASS TorProxy : public WebProxy {
public:
	IPEndPoint m_epSocks, m_epControl;

	static ptr<TorProxy> __stdcall GetSingleton(bool bQuiet = false);
	~TorProxy();

	void SendSignal(RCString signal) {
		EXT_LOCK (TorStatusThread->m_cs) {
			TorStatusThread->m_queue.push("SIGNAL " + signal);
			TorStatusThread->m_ev.Set();
		}
	}

	pair<String, vector<String> > ReadValue(bool bShouldBeEvent = false);
	map<String, vector<String> > ReadValues();
	void ReadEvents();

	map<String, vector<String> > Send(RCString s);

	void Authenticate() {
		Send(String("AUTHENTICATE ") + "\"foo\"");
	}

	void ChangeProxyChain() {
		SendSignal("NEWNYM");
	}
protected:
	Socket m_sockControl;
	NetworkStream ControlStream;

	thread_group m_tr;
	ptr<TorThread> m_torThread;
	ptr<class TorStatusThread> TorStatusThread;

	TorProxy(bool bStart = true, bool bQuiet = false);
	virtual void ProcessCircuitEvent(RCString line) {}
	virtual void ProcessStreamEvent(RCString line) {}
	virtual void ProcessStatusGeneralEvent(RCString rest) {}
	virtual void ProcessNewdescEvent(RCString rest) {}
	virtual void ProcessStatusClientEvent(RCString rest) {}
	virtual void ProcessAddrmapEvent(RCString rest) {}

	friend class TorStatusThread;
};


}} // Ext::Inet::



#pragma once

namespace Ext { namespace Inet { namespace P2P {

class DetectGlobalIp;

class DetectIpThread : public Thread {
	typedef Thread base;
public:
	P2P::DetectGlobalIp& DetectGlobalIp;

	DetectIpThread(P2P::DetectGlobalIp& dgi, thread_group *tr)
		:	base(tr)
		,	DetectGlobalIp(dgi)
	{
	}

	~DetectIpThread();
	void Execute() override;
};

class DetectGlobalIp {
public:
    ptr<DetectIpThread> Thread;

	~DetectGlobalIp();
	
	void Start(thread_group *tr) {
		Thread = new DetectIpThread(_self, tr);
		Thread->Start();	
	}
protected:	
	virtual void OnIpDetected(const IPAddress& ip) {}

private:
	mutex m_mtx;

friend class DetectIpThread;
};


}}} // Ext::Inet::P2P::


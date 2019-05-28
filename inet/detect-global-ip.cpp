#include <el/ext.h>

#include <el/inet/http.h>

#include "detect-global-ip.h"

namespace Ext { namespace Inet { namespace P2P {

static wregex s_reCurrentIp(L"IP Address:\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)");

DetectIpThread::~DetectIpThread() {
}

void DetectIpThread::Execute() {
	Name = "DetectIpThread";

	try {
		while (!m_bStop) {
			try {
				DBG_LOCAL_IGNORE_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED);
				DBG_LOCAL_IGNORE_WIN32(ERROR_INTERNET_TIMEOUT);
				DBG_LOCAL_IGNORE_WIN32(ERROR_HTTP_INVALID_SERVER_RESPONSE);

				String s = WebClient().DownloadString("http://checkip.dyndns.org");
				Smatch m;
				if (regex_search(s, m, s_reCurrentIp)) {
					IPAddress ip = IPAddress::Parse(m[1]);
					if (ip.IsGlobal())
						DetectGlobalIp.OnIpDetected(ip);
					break;
				}
			} catch (RCExc) {
			}
			Sleep(60000);
		}
	} catch (RCExc) {
	}
	EXT_LOCK (DetectGlobalIp.m_mtx) {
		DetectGlobalIp.Thread = nullptr;
	}
}

DetectGlobalIp::~DetectGlobalIp() {
	ptr<DetectIpThread> t;

	EXT_LOCK (m_mtx) {
		t = Thread;
	}
	if (t) {
		t->interrupt();
		t->Join();
	}
}


}}} // Ext::Inet::P2P::


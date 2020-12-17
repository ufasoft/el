/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "proxy-client.h"
#include "proxy.h"

#if UCFG_USE_TOR
#	include "tor.h"
#endif


namespace Ext {
namespace Inet {

class ProxyClientObj {
public:
#if UCFG_USE_TOR
	ptr<TorProxy> m_tor;
#endif
};

ProxyClient::~ProxyClient() {	// non-inlined for ptr<> dtor
	delete m_obj;
}

void ProxyClient::Connect(const IPEndPoint& ep) {
	if (ProxyString.empty())
		return base::Connect(ep);
	Uri uri(ProxyString);
#if UCFG_USE_TOR
	if (ProxyString.ToUpper() == "TOR") {
		if (m_obj)
			Throw(E_FAIL);
		m_obj = new ProxyClientObj;
		uri = Uri("socks5://127.0.0.1:" + Convert::ToString((m_obj->m_tor = TorProxy::GetSingleton(true))->Address.Port));
	}
#endif
	ptr<CProxyBase> proxy;
	String scheme = uri.Scheme;
	if (scheme == "socks5")
		proxy = new CSocks5Proxy;
	else if (scheme == "socks4")
		proxy = new CSocks4Proxy;
	else if (scheme == "connect")
		proxy = new CHttpProxy;
	else
		Throw(errc::invalid_argument);
	Client.Connect(IPEndPoint(uri.Host, uint16_t(uri.Port)));
	NetworkStream stm(Client);
	CProxyQuery q = { QueryType::Connect, ep };
	proxy->Connect(stm, q);
}



}} // Ext::Inet::


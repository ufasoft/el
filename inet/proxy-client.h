/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/libext/ext-net.h>

#ifndef UCFG_USE_TOR
#	define UCFG_USE_TOR 0
#endif

namespace Ext {
namespace Inet {

class ProxyClientObj;

class ProxyClient : public TcpClient {
	typedef TcpClient base;
public:
	String ProxyString;

	ProxyClient()
		:	m_obj(0)
	{}

	~ProxyClient();
	void Connect(const EndPoint& ep) override;
private:
	ProxyClientObj *m_obj;
};

}} // Ext::Inet::


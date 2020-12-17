/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/win/nt.h>

namespace Ext {


class IdleMonitor {
public:
	IdleMonitor()
		:	XpOrOlder(Environment::OSVersion.Version.Major <= 5)
		,	m_minRemaining(0)
		,	m_maxRemaining(0)
	{}

	TimeSpan GetIdleTime();
private:
	uint32_t m_minRemaining, m_maxRemaining;
	bool XpOrOlder;
};


} // Ext::


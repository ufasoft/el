/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include "idle-monitor.h"

namespace Ext {

TimeSpan IdleMonitor::GetIdleTime() {
	if (XpOrOlder) {
		uint32_t lii = Environment::GetLastInputInfo();															// GetLastInputInfo() before GetTickCount()
		return TimeSpan::FromMilliseconds(max((uint32_t)Environment::TickCount() - lii, uint32_t(0)));
	} else {
		auto spi = NtSystem::GetSystemPowerInformation();
		if (spi.TimeRemaining > m_minRemaining) {
			m_minRemaining = m_maxRemaining = spi.TimeRemaining;
			return TimeSpan(0);
		}
		m_minRemaining = spi.TimeRemaining;
		return TimeSpan::FromSeconds(m_maxRemaining - m_minRemaining);
	}
}


} // Ext::

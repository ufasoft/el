/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext { namespace Inet {
using namespace Ext;

class MailAddress : public CPrintable {
	typedef MailAddress class_type;
public:
	MailAddress(RCString address);

	String get_User() const { return m_user; }
	DEFPROP_GET(String, User);

	String get_Host() const { return m_host; }
	DEFPROP_GET(String, Host);

	String ToString() const override { return User + "@" + Host; }
private:
	String m_user, m_host;
};

void SendEmail(RCString recipient, RCString subj, RCString msg, RCString from = nullptr);
	


}} // Ext::Inet::


/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

typedef NTSTATUS *PNTSTATUS;
#include <ntlsa.h>

#include <el/win/secdesc.h>

namespace Ext { namespace Lsa {

class Policy : noncopyable {
public:
	Policy(RCString server = nullptr, ACCESS_MASK desiredAccess = POLICY_ALL_ACCESS);
	~Policy();
	void AddAccountRight(Sid& sid, RCString spriv);
	void RemoveAccountRight(Sid& sid, RCString spriv);
private:
	LSA_HANDLE m_h;
};


Sid GetAccountSid(RCString acc, RCString server = nullptr);

}} // Ext::Lsa::



/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/ext.h>

#include <windows.h>
#include <securitybaseapi.h>

#include "policy.h"

namespace Ext { namespace Lsa {

static LSA_UNICODE_STRING AsLsaString(RCString s) {
	LSA_UNICODE_STRING r = { USHORT(sizeof(WCHAR) * (!s.empty() ? wcslen(s) :  0)), 0, (PWSTR)(const wchar_t*)s };
	r.MaximumLength = r.Length + sizeof(WCHAR);
	return r;
}

Sid GetAccountSid(RCString acc, RCString server) {
	DWORD dwSize = 0;
	DWORD dwBuf = 0;
	SID_NAME_USE siu;
	Win32Check(::LookupAccountName(server, acc, 0, 	&dwSize, 0, &dwBuf, &siu));
	Sid sid(dwSize);
	Win32Check(::LookupAccountName(server, acc, sid, &dwSize, 0, &dwBuf, &siu));
	return sid;
}

Policy::Policy(RCString server, ACCESS_MASK desiredAccess) {
	LSA_OBJECT_ATTRIBUTES oa = { 0 };	
	NtCheck(::LsaOpenPolicy(&AsLsaString(server), &oa, desiredAccess, &m_h));
}

Policy::~Policy() {
	NtCheck(::LsaClose(m_h));
}

void Policy::AddAccountRight(Sid& sid, RCString spriv) {
	NtCheck(::LsaAddAccountRights(m_h, sid, &AsLsaString(spriv), 1));
}

void Policy::RemoveAccountRight(Sid& sid, RCString spriv) {
	NtCheck(::LsaRemoveAccountRights(m_h, sid, FALSE, &AsLsaString(spriv), 1));
}



}} // Ext::Lsa::

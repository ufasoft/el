/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <el/libext/win32/ext-win.h>
#include "secdesc.h"

#include <sddl.h>

#pragma comment(lib, "advapi32")

namespace Ext {


Sid Sid::World("S-1-1-0"),
		Sid::Local("S-1-2-0");

Sid::operator SID *() const {
	if (!m_vec.empty())
		return (SID*)&m_vec[0];
	if (!m_pSid && !m_ssid.empty()) {
		vector<String> v = m_ssid.Split("-");
		if (v[0] != "S")
			Throw(E_FAIL);
		int nSuba = v.size()-3;
		SID_IDENTIFIER_AUTHORITY auth = { 0 };
		auth.Value[5] = (BYTE)atoi(v[2]) ;
		DWORD a[8] = { 0 };
		for (int i=0; i<nSuba; i++)
			a[i] = atoi(v[3+i]);
		Win32Check(::AllocateAndInitializeSid(&auth, (BYTE)nSuba, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], (PSID*)&m_pSid));
	}
	return m_pSid;
}

Sid::Sid(RCString ssid)
	:	m_ssid(ssid)
{
}

Sid::Sid(const Sid& sid) {
	DWORD len = ::GetLengthSid(sid);
	m_vec.resize(len);
	memcpy(&m_vec[0], sid.operator SID *(), len);
}

Sid::Sid(SID *pSid) {
	DWORD len = ::GetLengthSid(pSid);
	m_vec.resize(len);
	memcpy(&m_vec[0], pSid, len);
}

String Sid::ToString() const {
	LPTSTR pstr;
	Win32Check(::ConvertSidToStringSid(_self, &pstr));
	String r = pstr;
	LocalFree(pstr);
	return r;
}


Ace::Ace(ACE_HEADER *pace)
	:	m_blob(pace, pace->AceSize)
{
}

Acl::Acl(ACL *pacl) {
	ACL_SIZE_INFORMATION asi;
	Win32Check(::GetAclInformation(pacl, &asi, sizeof asi, AclSizeInformation));
	m_blob = Blob(pacl, asi.AclBytesInUse);
}

Acl::Acl(size_t size)
	:	m_blob(0, size)
{
	Win32Check(::InitializeAcl(_self, size, ACL_REVISION));
}

size_t Acl::get_Count() const {
	ACL_SIZE_INFORMATION asi;
	Win32Check(::GetAclInformation(_self, &asi, sizeof asi, AclSizeInformation));
	return asi.AceCount;
}

Ace Acl::operator[](int idx) {
	ACE_HEADER *pace;
	Win32Check(::GetAce(_self, idx, (void**)&pace));
	return Ace(pace);
}

void Acl::CopyTo(Acl& aclDest) {
	size_t count = Count;
	for (size_t i=0; i<count; ++i)
		aclDest.Add(_self[i]);
}

void Acl::Add(const Ace& ace, DWORD dwStartingAceIndex) {
	Win32Check(::AddAce(_self, ACL_REVISION, dwStartingAceIndex, ace, ace.Size));
}

void Acl::AddAccessAllowedAce(DWORD dwAceRevision, DWORD AccessMask, const Sid& sid) {
	if (::AddAccessAllowedAce(_self, dwAceRevision, AccessMask, sid))
		return;
	Win32Check(::GetLastError()==ERROR_ALLOTTED_SPACE_EXCEEDED);
	size_t cbAcl = m_blob.Size+sizeof(ACCESS_ALLOWED_ACE)+sid.Size;
	Acl nacl(cbAcl);
	CopyTo(nacl);
	m_blob = nacl.m_blob;
	Win32Check(::AddAccessAllowedAce(_self, dwAceRevision, AccessMask, sid));
}

SecurityDescriptor::SecurityDescriptor(SECURITY_DESCRIPTOR *psd)
	:	m_blob(psd, ::GetSecurityDescriptorLength(psd))
{
}

SecurityDescriptor::SecurityDescriptor(RCString s) {
	PSECURITY_DESCRIPTOR pSD;
	ULONG size;
	Win32Check(::ConvertStringSecurityDescriptorToSecurityDescriptor(s, SDDL_REVISION_1, &pSD, &size));
	m_blob = Blob(pSD, size);
	Win32Check(!::LocalFree(pSD));
}

Acl SecurityDescriptor::get_Dacl() {
	BOOL bDaclPresent, bDaclDefaulted;
	ACL *pacl;
	Win32Check(::GetSecurityDescriptorDacl(_self, &bDaclPresent, &pacl, &bDaclDefaulted));
	if (!bDaclPresent)
		Throw(E_FAIL);
	return Acl(pacl);
}

void SecurityDescriptor::put_Dacl(const Acl& acl, bool bDaclDefaulted) {
	Win32Check(::SetSecurityDescriptorDacl(_self, true, acl, bDaclDefaulted));
}

AbsoluteSecurityDescriptor::AbsoluteSecurityDescriptor(const SecurityDescriptor& sd) {
	DWORD dwSD = 0, dwOwner = 0, dwGroup = 0, dwDacl = 0, dwSacl = 0;
	if (::MakeAbsoluteSD(sd, NULL, &dwSD, NULL, &dwDacl, NULL, &dwSacl, NULL, &dwOwner, NULL, &dwGroup))
		Throw(E_FAIL);
	Win32Check(::GetLastError() == ERROR_INSUFFICIENT_BUFFER);
	m_blob.Size = dwSD;
	m_owner.Size = dwOwner;
	m_group.Size = dwGroup;
	m_dacl.Size = dwDacl;
	m_sacl.Size = dwSacl;
	Win32Check(::MakeAbsoluteSD(sd, m_blob.data(), &dwSD, (PACL)m_dacl.data(), &dwDacl, (PACL)m_sacl.data(), &dwSacl, m_owner.data(), &dwOwner, m_group.data(), &dwGroup));
}

SecurityDescriptor GetServiceSecurityDacl(ServiceController& service) {
	DWORD dw;
	if (::QueryServiceObjectSecurity(service.m_handle, DACL_SECURITY_INFORMATION, &dw, 0, &dw))
		Throw(E_FAIL);
	Win32Check(::GetLastError()==ERROR_INSUFFICIENT_BUFFER);
	SECURITY_DESCRIPTOR *psd = (SECURITY_DESCRIPTOR*)alloca(dw);
	Win32Check(::QueryServiceObjectSecurity(service.m_handle, DACL_SECURITY_INFORMATION, psd, dw, &dw));
	return SecurityDescriptor(psd);
}

void SetServiceSecurityDacl(ServiceController& service, const SecurityDescriptor& sd) {
	Win32Check(::SetServiceObjectSecurity(service.m_handle, DACL_SECURITY_INFORMATION, sd));
}

AccessToken::AccessToken(Process& process) {
	HANDLE h;
	Win32Check(::OpenProcessToken((HANDLE)(intptr_t)HandleAccess(*process.m_pimpl), MAXIMUM_ALLOWED, &h));
	Attach(h);
}

Sid AccessToken::get_User() {
	BYTE buf[1000];
	DWORD dw;
	Win32Check(::GetTokenInformation((HANDLE)(intptr_t)HandleAccess(_self), TokenUser, buf, sizeof(buf), &dw));
	return (SID*)((TOKEN_USER*)buf)->User.Sid;
}

void AccessToken::AdjustPrivilege(RCString spriv) {
	TOKEN_PRIVILEGES tp = { 1 };
	Win32Check(::LookupPrivilegeValue(0, spriv, &tp.Privileges[0].Luid));
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	::SetLastError(0);
	Win32Check(::AdjustTokenPrivileges((HANDLE)(intptr_t)HandleAccess(_self), FALSE, &tp, 0, (PTOKEN_PRIVILEGES)0, 0) && !::GetLastError());	// can return TRUE even when ERROR_NOT_ALL_ASSIGNED
}

} // Ext::


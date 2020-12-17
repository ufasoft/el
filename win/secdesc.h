/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/win/service.h>

namespace Ext {
using namespace Ext::ServiceProcess;

class Sid : public CPrintable {
	String m_ssid;
	observer_ptr<SID> m_pSid;
	vector<uint8_t> m_vec;
public:
	static Sid World,
		         Local;

	Sid()
	{}

	Sid(const Sid& sid);
	Sid(SID *pSid);
	Sid(RCString ssid);
	
	explicit Sid(size_t size)
		:	m_vec(size)
	{}
	
	~Sid() {
		if (m_pSid)
			::FreeSid(m_pSid);
	}

	operator SID *() const;
	
	size_t get_Size() const { return ::GetLengthSid(_self); }
	DEFPROP_GET_CONST(size_t,Size);

	String ToString() const;
};

inline bool operator==(const Sid& sid1, const Sid& sid2) { return ::EqualSid(sid1, sid2); }

class Ace {
	Blob m_blob;
public:
	Ace(ACE_HEADER *pace);

	operator ACE_HEADER*() const { return (ACE_HEADER*)m_blob.data(); }

	size_t get_Size() const { return ((ACE_HEADER*)m_blob.data())->AceSize; }
	DEFPROP_GET_CONST(size_t,Size);
};

class Acl {
	Blob m_blob;
public:
	Acl(ACL *pacl);
	Acl(size_t size);
	operator bool() const { return m_blob.size() != 0; }
	operator ACL*() const { return  *this ? (ACL*)m_blob.data() : 0; }

	size_t get_Count() const ;
	DEFPROP_GET_CONST(size_t, Count);

	Ace operator[](int idx);

	void CopyTo(Acl& aclDest);
	void Add(const Ace& ace, DWORD dwStartingAceIndex = MAXDWORD);
	void AddAccessAllowedAce(DWORD dwAceRevision, DWORD AccessMask, const Sid& sid);

friend class SecurityDescriptor;
};

class SecurityDescriptor {
public:
	SecurityDescriptor() {}
	SecurityDescriptor(SECURITY_DESCRIPTOR *psd);
	SecurityDescriptor(RCString s);

	operator SECURITY_DESCRIPTOR*() const { return (SECURITY_DESCRIPTOR*)m_blob.data(); }

	bool get_ValidP() const { return ::IsValidSecurityDescriptor(_self); }
	DEFPROP_GET_CONST(bool,ValidP);

	Acl get_Dacl();
	void put_Dacl(const Acl& acl, bool bDaclDefaulted = false);
	DEFPROP(Acl,Dacl);
protected:
	Blob m_blob;
};

class AbsoluteSecurityDescriptor : public SecurityDescriptor {
	Blob m_owner, m_group, m_dacl, m_sacl;
public:
	AbsoluteSecurityDescriptor(const SecurityDescriptor& sd);
};

SecurityDescriptor GetServiceSecurityDacl(ServiceController& service);
void SetServiceSecurityDacl(ServiceController& service, const SecurityDescriptor& sd);


class AccessToken : public SafeHandle {
public:
	AccessToken(Process& process);

	Sid get_User();
	DEFPROP_GET(Sid, User);

	void AdjustPrivilege(RCString spriv);
};



} // Ext::

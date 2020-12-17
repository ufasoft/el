/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

namespace Ext {

#if !UCFG_WIN32
	typedef HANDLE HKEY;
#endif

class CRegistryValuesIterator;

class AFX_CLASS CRegistryValue {
public:
	typedef CRegistryValue class_type;

	Blob m_blob;
	String m_name;
	uint32_t m_type;

	CRegistryValue(uint32_t typ, byte *p, int len);
	CRegistryValue(int v);
	CRegistryValue(uint32_t v);
	CRegistryValue(uint64_t v);
	//!!!	CRegistryValue(LPCSTR s);
	CRegistryValue(RCString s, bool bExpand = false);
	CRegistryValue(const ConstBuf& mb);
	CRegistryValue(const CStringVector& ar) { Init(ar); }

	String get_Name() const;
	DEFPROP_GET(String, Name);

	operator uint32_t() const;

#if	UCFG_SEPARATE_LONG_TYPE
	CRegistryValue(unsigned long v);
	operator unsigned long() const { return operator uint32_t(); }
#endif

	operator uint64_t() const;
	operator String() const;
	operator Blob() const;
	operator bool() const { return operator uint64_t(); }
	EXT_API operator CStringVector() const;

	CStringVector ToStrings() const { return operator CStringVector(); }
	//!!!  VARIANT GetAsSafeArray();
private:
	EXT_API void Init(const CStringVector& ar);

	friend class RegistryKey;
	friend class CRegistryValues;
};

class AFX_CLASS CRegistryValues {
	RegistryKey& m_key;
public:
	typedef CRegistryValues class_type;
	CRegistryValues(RegistryKey& key)
		:	m_key(key)
	{}
	CRegistryValuesIterator begin();
	CRegistryValuesIterator end();
	CRegistryValue operator[](int i);
	CRegistryValues operator[](RCString name);

	int get_Count();
	DEFPROP_GET(int, Count);
};

class AFX_CLASS CRegistryValuesIterator {
	int m_i;
	CRegistryValues& m_values;
public:
	CRegistryValuesIterator(CRegistryValues& values)
		:	m_i(0)
		,	m_values(values)
	{}

	CRegistryValuesIterator(const CRegistryValuesIterator& i)
		:	m_i(i.m_i)
		,	m_values(i.m_values)
	{}

	void operator++(int);
	bool operator!=(CRegistryValuesIterator& i);
	CRegistryValue operator*();
};

class AFX_CLASS RegistryKey : public SafeHandle {
	typedef RegistryKey class_type;
public:
	struct CRegKeyInfo {
		DWORD SubKeys,
			MaxSubKeyLen,
			MaxClassLen,
			Values,
			MaxValueNameLen,
			MaxValueLen,
			SecurityDescriptor;
		DateTime LastWriteTime;

		CRegKeyInfo() {
			ZeroStruct(*this);
		}
	};

	String m_subKey;
	ACCESS_MASK AccessRights;

	RegistryKey()
		:	AccessRights(MAXIMUM_ALLOWED)
	{
	}

	RegistryKey(HKEY key)
		:	SafeHandle((intptr_t)key)
		,	AccessRights(MAXIMUM_ALLOWED)
	{
	}

	RegistryKey(HKEY key, RCString subKey, bool create = true);
	~RegistryKey();
	void Create() const;
	void Open(bool create = false);
	void Open(HKEY key, RCString subKey, bool create = true);
	void Load(RCString subKey, RCString fileName);
	void Save(RCString fileName);
	void UnLoad(RCString subKey);
	void Flush();
	void DeleteSubKey(RCString subkey);
	void DeleteSubKeyTree(RCString subkey);
	void DeleteValue(RCString name, bool throwOnMissingValue = true);
	bool ValueExists(RCString name);
	CRegistryValue QueryValue(RCString name = nullptr);
	CRegistryValue TryQueryValue(RCString name, DWORD def);
	CRegistryValue TryQueryValue(RCString name, RCString def);
	CRegistryValue TryQueryValue(RCString name, const CRegistryValue& def);
	CRegKeyInfo GetRegKeyInfo();
	void SetValue(RCString name, DWORD value);
	void SetValue(RCString name, RCString value);
	void SetValue(RCString name, const CRegistryValue& rv);
	int GetMaxValueNameLen();
	int GetMaxValueLen();
	void GetSubKey(int idx, RegistryKey& sk);
	EXT_API CStringVector GetSubKeyNames();

	String get_Name();
	DEFPROP_GET(String, Name);

	bool get_Reflected();
	void put_Reflected(bool v);
	DEFPROP(bool, Reflected);

	operator HKEY() const;
	bool KeyExists(RCString subKey);
protected:
#if UCFG_WIN32
	void ReleaseHandle(intptr_t h) const override;
#endif
private:
	HKEY m_parent;
	bool m_create;
};

class Registry {
public:
	EXT_DATA static RegistryKey ClassesRoot,
		CurrentUser,
		LocalMachine;
};

class Wow64RegistryReflectionKeeper {
public:
	RegistryKey& m_key;
	CBool m_bChanged, m_bPrev;

	Wow64RegistryReflectionKeeper(RegistryKey& key)
		:	m_key(key)
	{
	}

	~Wow64RegistryReflectionKeeper() {
		if (m_bChanged)
			m_key.Reflected = m_bPrev;
	}

	void Change(bool v) {
		m_bChanged = true;
		m_bPrev = m_key.Reflected;
		m_key.Reflected = v;
	}
};


}	// Ext::

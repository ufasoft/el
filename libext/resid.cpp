/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <windows.h>

#	include <el/libext/win32/ext-win.h>
#endif

#if UCFG_WIN32_FULL
#	include <el/libext/win32/ext-full-win.h>
#endif


namespace Ext {
using namespace std;


CResID::CResID(UINT nResId)
	: m_resId(nResId)
{
}

CResID::CResID(const char *lpName)
    : m_resId(0)
{
	_self = lpName;
}

CResID::CResID(const String::value_type *lpName)
	: m_resId(0)
{
	_self = lpName;
}

CResID& CResID::operator=(const char *resId) {
	m_name = "";
	m_resId = 0;
	if (HIWORD(resId))
		m_name = resId;
	else
		m_resId = (uint32_t)(uintptr_t)resId;
	return _self;
}

CResID& CResID::operator=(const String::value_type *resId) {
	m_name = "";
	m_resId = 0;
	if (HIWORD(resId))
		m_name = resId;
	else
		m_resId = (uint32_t)(uintptr_t)resId;
	return _self;
}

AFX_API CResID& CResID::operator=(RCString resId) {
	m_resId = 0;
	m_name = resId;
	return _self;
}

CResID::operator const char *() const {
	if (m_name.empty())
		return (const char*)m_resId;
	else
		return m_name;
}

CResID::operator const String::value_type *() const {
	if (m_name.empty())
		return (const String::value_type *)m_resId;
	else
		return m_name;
}

#ifdef WIN32
CResID::operator UINT() const {
	return (uint32_t)(uintptr_t)(operator LPCTSTR());
}
#endif

String CResID::ToString() const {
	return m_name.empty() ? Convert::ToString((DWORD)m_resId) : m_name;
}

void CResID::Read(const BinaryReader& rd) {
	rd >> m_resId >> m_name;
}

void CResID::Write(BinaryWriter& wr) const {
	wr << m_resId << m_name;
}


} // Ext::

/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


namespace Ext {
using namespace std;


#if UCFG_USE_REGEX
static StaticRegex s_reGuid("\\{([0-9A-Fa-f]{8,8})-([0-9A-Fa-f]{4,4})-([0-9A-Fa-f]{4,4})-([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})-([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})([0-9A-Fa-f]{2,2})\\}");
#endif

Guid::Guid(RCString s) {
#if UCFG_COM
	OleCheck(::CLSIDFromString(Bstr(s), this));
#elif UCFG_USE_REGEX
	cmatch m;
	if (regex_search(s.c_str(), m, *s_reGuid)) {
		Data1 = Convert::ToUInt32(m[1], 16);
		Data2 = Convert::ToUInt16(m[2], 16);
		Data3 = Convert::ToUInt16(m[3], 16);
		for (int i = 0; i < 8; ++i)
			Data4[i] = (uint8_t)Convert::ToUInt16(m[4 + i], 16);
	}
	else
		Throw(E_FAIL);
#else
	Throw(E_NOTIMPL);
#endif
}

Guid Guid::NewGuid() {
	Guid guid;
#if UCFG_COM
	OleCheck(::CoCreateGuid(&guid));
#else
	Random rng;
	rng.NextBytes(span<uint8_t>((uint8_t*)&guid, sizeof(GUID)));
#endif
	return guid;
}

#if UCFG_COM
Guid Guid::FromProgId(RCString progId) {
	Guid guid;
	OleCheck(::CLSIDFromProgID(progId, &guid));
	return guid;
}
#endif

String Guid::ToString(RCString format) const {
#if UCFG_WIN32
	WCHAR buf[40];
	if (!::StringFromGUID2(_self, buf, size(buf)))
		Throw(E_FAIL);
#else
	char buf[40];
	sprintf(buf, "{%08X-%08X-%04X-%04X-%02X%02-%02%02%02%02%02%02}", Data1, Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
#endif
	switch (format.empty() ? 'D' : format.c_wstr()[0]) {
	case 0:
	case 'D':
		return String(buf + 1, 36);
	case 'P':
		buf[0] = '(';
		buf[38] = ')';
	case 'B':
		break;
	case 'N':
		memmove(buf, buf + 1, 8 * sizeof(buf[0]));
		memmove(buf+8, buf + 10, 4 * sizeof(buf[0]));
		memmove(buf + 12, buf + 15, 4 * sizeof(buf[0]));
		memmove(buf + 16, buf + 20, 4 * sizeof(buf[0]));
		memmove(buf + 20, buf + 25, 12 * sizeof(buf[0]));
		buf[32] = 0;
		break;
	case 'X':
		char code[70];
		sprintf(code, "{0x%08X,0x%04X,0x%04X,{0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X}}", Data1, Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
		return code;
	default:
		Throw(E_INVALIDARG);
	}
	return buf;
}



} // Ext::

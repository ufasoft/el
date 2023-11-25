/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>



namespace Ext {
using namespace std;

CMessageProcessor g_messageProcessor;


#if UCFG_COM

HRESULT AFXAPI AfxProcessError(HRESULT hr, EXCEPINFO* pexcepinfo) {
	return CMessageProcessor::Process(hr, pexcepinfo);
}

HRESULT CMessageProcessor::Process(HRESULT hr, EXCEPINFO* pexcepinfo) {
	//!!!  HRESULT hr = ProcessOleException(e);
	if (pexcepinfo) {
		pexcepinfo->bstrDescription = HResultToMessage(hr).AllocSysString();
		pexcepinfo->wCode = 0;
		pexcepinfo->scode = hr;
		return DISP_E_EXCEPTION;
	}
	else
		return hr;
}
#endif

void CMessageProcessor::RegisterModule(DWORD lowerCode, DWORD upperCode, RCString moduleName) {
	g_messageProcessor.CheckStandard();
	CModuleInfo info;
	info.Init(lowerCode, upperCode, moduleName.c_str());
	g_messageProcessor.m_vec.push_back(info);
}

static String CombineErrorMessages(const char hex[], RCString msg, bool bWithErrorCode) {
	return bWithErrorCode
		? String(hex) + ":  " + msg
		: msg;
}

String CMessageProcessor::ProcessInst(HRESULT hr, bool bWithErrorCode) {
	CheckStandard();
	char hex[30] = "";
	if (bWithErrorCode)
		sprintf(hex, "Error %8.8X", hr);
	TCHAR buf[256];
	char* ar[5] = { 0, 0, 0, 0, 0 };
	char** p = 0;
	/*!!!R
	String *ps = m_param;
	if (ps && !ps->empty()) {
		ar[0] = (char*)ps->c_str();
		p = ar;
	}*/
	for (size_t i = 0; i < m_ranges.size(); i++) {
		String msg = m_ranges[i]->CheckMessage(hr);
		if (!msg.empty())
			return CombineErrorMessages(hex, msg, bWithErrorCode);
	}

	int fac = HRESULT_FACILITY(hr);

#if UCFG_WIN32
	if (::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, hr, 0, buf, sizeof buf, p))
		return CombineErrorMessages(hex, buf, bWithErrorCode);
	switch (fac) {
	case FACILITY_INTERNET:
		if (FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, LPCVOID(GetModuleHandle(_T("urlmon.dll"))), hr, 0, buf, sizeof buf, 0))
			return CombineErrorMessages(hex, buf, bWithErrorCode);
		break;
	case FACILITY_WIN32:
		return win32_category().message(WORD(HRESULT_CODE(hr)));
	}
#endif // UCFG_WIN32

	if (const error_category* cat = ErrorCategoryBase::Find(fac)) {
		return cat->message(hr & 0xFFFF);
	}

	for (vector<CModuleInfo>::iterator i(m_vec.begin()); i != m_vec.end(); ++i) {
		String msg = i->GetMessage(hr);
		if (!!msg)
			return CombineErrorMessages(hex, msg, bWithErrorCode);
	}
	String msg = m_default.GetMessage(hr);
	if (!!msg)
		return msg;
#if UCFG_HAVE_STRERROR
	if (HRESULT_FACILITY(hr) == FACILITY_C) {
		if (const char* s = strerror(HRESULT_CODE(hr)))
			return CombineErrorMessages(hex, s, bWithErrorCode);
	}
#endif
#if UCFG_WIN32
	HMODULE hModuleThis = 0;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)&CombineErrorMessages, &hModuleThis);
	if (::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS, LPCVOID(hModuleThis), hr, 0, buf, sizeof buf, 0))
		return CombineErrorMessages(hex, buf, bWithErrorCode);
#endif
	return hex;
}

String CMessageProcessor::CModuleInfo::GetMessage(HRESULT hr) {
	if (uint32_t(hr) < m_lowerCode || uint32_t(hr) >= m_upperCode)
		return nullptr;
#if UCFG_USE_POSIX
	if (!m_mcat) {
		if (exchange(m_bCheckedOpen, true) || !exists(m_moduleName) && !exists(System.GetExeDir() / m_moduleName))
			return nullptr;
		try {
			m_mcat.Open(m_moduleName.c_str());
		} catch (RCExc) {
			if (!m_moduleName.parent_path().empty())
				return nullptr;
			try {
				m_mcat.Open((System.get_ExeFilePath().parent_path() / m_moduleName).c_str());
			} catch (RCExc) {
				return nullptr;
			}
		}
	}
	return m_mcat.GetMessage((hr >> 16) & 0xFF, hr & 0xFFFF);
#elif UCFG_WIN32
	TCHAR buf[256];
	if (::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY, LPCVOID(GetModuleHandle(String(m_moduleName.native()))), hr, 0, buf, sizeof buf, 0))
		return buf;
	else
		return nullptr;
#else
	return nullptr;
#endif
}

String AFXAPI HResultToMessage(HRESULT hr, bool bWithErrorCode) {
	return g_messageProcessor.ProcessInst(hr, bWithErrorCode);
}

void CMessageProcessor::CheckStandard() {
#if UCFG_EXTENDED
	if (m_vec.size() == 0) {
		/*!!!R		CModuleInfo info;
				info.m_lowerCode = E_EXT_BASE;
				info.m_upperCode = (DWORD)E_EXT_UPPER;

		#ifdef _AFXDLL
				AFX_MODULE_STATE *pMS = AfxGetStaticModuleState();
		#else
				AFX_MODULE_STATE *pMS = &_afxBaseModuleState;
		#endif
				info.m_moduleName = pMS->FileName.filename();
				m_vec.push_back(info);
				*/
	}
#endif
}

CMessageProcessor::CMessageProcessor() {
	m_default.Init(0, uint32_t(-1), System.get_ExeFilePath().stem());
}

void CMessageProcessor::CModuleInfo::Init(uint32_t lowerCode, uint32_t upperCode, const path& moduleName) {
	m_lowerCode = lowerCode;
	m_upperCode = upperCode;
	m_moduleName = moduleName;
	if (!m_moduleName.has_extension()) {
#if UCFG_USE_POSIX
		m_moduleName += ".cat";
#elif UCFG_WIN32
		m_moduleName += ".dll";
#endif
	}
}


/*!!!Rvoid CMessageProcessor::SetParam(RCString s) {
	String *ps = g_messageProcessor.m_param;
	if (!ps)
		g_messageProcessor.m_param.reset(ps = new String);
	*ps = s;
}*/

CMessageRange::CMessageRange() {
	g_messageProcessor.m_ranges.push_back(this);
}

CMessageRange::~CMessageRange() {
	g_messageProcessor.m_ranges.erase(std::remove(g_messageProcessor.m_ranges.begin(), g_messageProcessor.m_ranges.end(), this), g_messageProcessor.m_ranges.end());
}


} // Ext::

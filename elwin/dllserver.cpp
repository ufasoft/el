#include <el/ext.h>
#include <el/libext/win32/ext-win.h>

using namespace Ext;

STDAPI DllRegisterServer() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	try {
		return CDllServer::I->OnRegister();
	} catch (RCExc ex) {
		return HResultInCatch(ex);
	}
}

STDAPI DllUnregisterServer() {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	try {
		return CDllServer::I->OnUnregister();
	} catch (RCExc ex) {
		return HResultInCatch(ex);
	}
}

#if UCFG_COM_IMPLOBJ

class CFlag {
public:
	bool m_bool;

	CFlag()
		: m_bool(true) {
	}

	~CFlag() {
		m_bool = false;
	}
};

CFlag g_flag;

CDllServer* CDllServer::I;
static CDllServer s_dllServer;

STDAPI DllCanUnloadNow() {
	if (g_flag.m_bool) {
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		return AfxDllCanUnloadNow();
	}
	else
		return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllGetClassObject(rclsid, riid, ppv);
}
#endif // UCFG_COM_IMPLOBJ

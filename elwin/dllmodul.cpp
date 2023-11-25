#include <el/ext.h>

#include <el/libext/ext-os-api.h>
#include <el/libext/win32/extwin32.h>
//!!!R #include <el/libext/win32/extmfc.h>

extern "C" {
#ifdef _M_IX86
	int _afxForceUSRDLL;
#else
	int __afxForceUSRDLL;
#endif
}


#pragma warning(disable: 4073)
#pragma init_seg(lib)

namespace Ext {

static AFX_MODULE_STATE afxModuleState(true);

#ifdef _AFXDLL

static LRESULT CALLBACK AfxWndProcDllStatic(HWND, UINT, WPARAM, LPARAM);

static AFX_MODULE_STATE afxModuleState(true, &AfxWndProcDllStatic);

static LRESULT CALLBACK AfxWndProcDllStatic(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam) {
	AFX_MANAGE_STATE(&afxModuleState);
	MSG msg = { hWnd, nMsg, wParam, lParam };
	return AfxWndProc(msg);
}

AFX_MODULE_STATE* _AfxGetOleModuleState() {
	return &afxModuleState;
}

AFX_MODULE_STATE * AFXAPI AfxGetStaticModuleState() {
	AFX_MODULE_STATE* pModuleState = &afxModuleState;
	return pModuleState;
}

#else

AFX_MODULE_STATE* AFXAPI AfxGetStaticModuleState() {
	AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
	return pModuleState;
}

#endif

} // Ext::

static AFX_MODULE_STATE *s_prevModuleState;

extern "C" BOOL WINAPI RawDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID) {

#if UCFG_OS_IMPTLS
	if (dwReason == DLL_PROCESS_ATTACH) {
		BOOL r = Ext::InitTls(hInstance);
		if (!r)
			return r;
	}
#endif


//#ifdef _AFXDLL
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		afxModuleState.m_hCurrentInstanceHandle = (HMODULE)hInstance;
		afxModuleState.m_hCurrentResourceHandle = (HMODULE)hInstance;
		AfxSetModuleState(&afxModuleState);
		break;
	case DLL_PROCESS_DETACH:
#ifdef _AFXDLL
		AfxRestoreModuleState();
#endif
		;
	}
//#endif
	return TRUE;
}

extern "C" BOOL (WINAPI* _pRawDllMain)(HINSTANCE, DWORD, LPVOID) = &RawDllMain;

extern "C" BOOL WINAPI DllMain(HINST hInstance, DWORD dwReason, LPVOID ) {		//!!!
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		{
		TRC(1, "DLL_PROCESS_ATTACH");
#ifdef _AFXDLL
			AfxCoreInitModule();										// wire up resources from core DLL
#endif
			AfxWinInit((HINSTANCE)hInstance, 0, _T(""), 0);
			CWinApp *pApp = AfxGetApp();
			if (pApp)
				pApp->InitInstance();
	#ifdef _AFXDLL
			AfxRestoreModuleState();
	#else
//!!!?			AfxInitLocalData(hInstance);
	#endif
		}
		break;
	case DLL_PROCESS_DETACH: {
		TRC(1, "DLL_PROCESS_DETACH");
#	ifdef _AFXDLL
 			AfxSetModuleState(&afxModuleState);
#	endif
			CWinApp* pApp = AfxGetApp();
			if (pApp)
				pApp->ExitInstance();
		}
#	ifndef _AFXDLL
//!!!?		AfxTermLocalData(hInstance, true);
#	endif
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
#	ifdef _AFXDLL
		AFX_MANAGE_STATE(&afxModuleState);
		AfxTermThread((HINSTANCE)hInstance);
#	endif
		break;
	}
	return TRUE;
}




#ifdef _AFXDLL
void __cdecl AfxTermDllState() {
	AfxTlsRelease();
}
#endif

//!!!char _afxTermDllState = (char)(AfxTlsAddRef(), atexit(&AfxTermDllState));

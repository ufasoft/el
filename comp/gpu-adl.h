/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <amd-adl/adl_sdk.h>

namespace Ext { namespace Gpu {
using namespace Ext;

class AdlEngine;


class AdlException : public Exception {
	typedef Exception base;
public:
	AdlException(HRESULT hr)
		:	base(hr)
	{}
};

int AdlCheck(int v);



class AdlDevice {
	typedef AdlDevice class_type;
public:
	int Index;
	String UDID;

	AdlDevice(AdlEngine& eng, int idx);

	bool get_Active();
	DEFPROP_GET(bool, Active);

	Temperature get_Temperature();
	DEFPROP_GET(Temperature, Temperature);
protected:
	AdlEngine& m_eng;

	void UpdateIndex();
};

typedef int (__cdecl *ADL_MAIN_CONTROL_CREATE )(ADL_MAIN_MALLOC_CALLBACK, int );
typedef int (__cdecl *ADL_EMPTRY_ARGS)();
typedef int (__cdecl *ADL_ADAPTER_NUMBEROFADAPTERS_GET) (int*);
typedef int (__cdecl *ADL_ADAPTER_ACTIVE_GET)(int iAdapterIndex, int *lpStatus);
typedef int (__cdecl *ADL_ADAPTER_ADAPTERINFO_GET) ( LPAdapterInfo, int);
typedef int (__cdecl *ADL_OVERDRIVE5_TEMPERATURE_GET)(int iAdapterIndex, int iThermalControllerIndex, ADLTemperature *lpTemperature);
typedef int (__cdecl *ADL_ADAPTER_ID_GET)(int iAdapterIndex, int *lpAdapterID);
	
class AdlEngine {
	typedef AdlEngine class_type;
public:
	AdlEngine();

	operator bool() {
		return m_bLoaded;
	}

	int get_DeviceCount();
	DEFPROP_GET(int, DeviceCount);

	AdlDevice GetDevice(int idx);
protected:
	CDynamicLibrary m_dll;
	CBool m_bLoaded;

	DlProcWrap<ADL_MAIN_CONTROL_CREATE>				ADL_Main_Control_Create;
	DlProcWrap<ADL_EMPTRY_ARGS>						ADL_Main_Control_Destroy;
	DlProcWrap<ADL_EMPTRY_ARGS>						ADL_Main_Control_Refresh;
	DlProcWrap<ADL_ADAPTER_NUMBEROFADAPTERS_GET>	ADL_Adapter_NumberOfAdapters_Get;
	DlProcWrap<ADL_ADAPTER_ACTIVE_GET>				ADL_Adapter_Active_Get;
	DlProcWrap<ADL_ADAPTER_ADAPTERINFO_GET>			ADL_Adapter_AdapterInfo_Get;
	DlProcWrap<ADL_OVERDRIVE5_TEMPERATURE_GET>		ADL_Overdrive5_Temperature_Get;
	DlProcWrap<ADL_ADAPTER_ID_GET>					ADL_Adapter_ID_Get;

	vector<AdapterInfo> m_vAdapterInfo;

	void RefreshAdapterInfo();

	friend class AdlDevice;
};


}} // Ext::Gpu::


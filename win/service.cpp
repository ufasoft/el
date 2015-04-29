/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "service.h"

namespace Ext { namespace ServiceProcess {

void ServiceBase::put_Status(DWORD v) {
	SERVICE_STATUS st = { SERVICE_WIN32_OWN_PROCESS, m_status = v, 0, 0 };
	if (CanStop)
		st.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
	if (CanShutdown)
		st.dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
	if (CanPauseAndContinue)
		st.dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
	if (CanHandlePowerEvent)
		st.dwControlsAccepted |= SERVICE_ACCEPT_POWEREVENT;
	if (CanHandleSessionChangeEvent)
		st.dwControlsAccepted |= SERVICE_ACCEPT_SESSIONCHANGE;

	switch (v) {
	case SERVICE_STOP_PENDING:
	case SERVICE_START_PENDING:
		st.dwCheckPoint = 1;
		st.dwWaitHint = 1000;
		break;
	case SERVICE_STOPPED:
		if (ExitCode) {
			if (HRESULT_FACILITY(ExitCode) == FACILITY_WIN32)
				st.dwWin32ExitCode = HRESULT_CODE(ExitCode);
			else {
				st.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
				st.dwServiceSpecificExitCode = ExitCode;
			}
		}
		break;
	}
	Win32Check(::SetServiceStatus(ServiceHandle, &st));
}

void ServiceBase::OnStart() {
}

void ServiceBase::OnStop() {
	m_evStop.Set();
}

void ServiceBase::OnShutdown() {
	m_evStop.Set();
}

void ServiceBase::OnPause() {
}

void ServiceBase::OnContinue() {
}

void ServiceBase::Stop() {
	Status = SERVICE_STOP_PENDING;
	OnStop();
}

#ifndef SERVICE_CONTROL_USERMODEREBOOT
#	define SERVICE_CONTROL_USERMODEREBOOT 0x00000040
#endif

String ServiceControlToString(DWORD cmd) {
#if UCFG_TRC
	static const struct {
		DWORD Cmd;
		const char *Name;
	} ar[] = {
		{ SERVICE_CONTROL_STOP					, "STOP"			},
		{ SERVICE_CONTROL_PAUSE					, "PAUSE"			},
		{ SERVICE_CONTROL_CONTINUE				, "CONTINUE"		},
		{ SERVICE_CONTROL_INTERROGATE			, "INTERROGATE"		},
		{ SERVICE_CONTROL_SHUTDOWN				, "SHUTDOWN "		},
		{ SERVICE_CONTROL_PARAMCHANGE			, "PARAMCHANGE"		},
		{ SERVICE_CONTROL_NETBINDADD			, "NETBINDADD"		},
		{ SERVICE_CONTROL_NETBINDREMOVE			, "NETBINDREMOVE"	},
		{ SERVICE_CONTROL_NETBINDENABLE			, "NETBINDENABLE"	},
		{ SERVICE_CONTROL_NETBINDDISABLE		, "NETBINDDISABLE"	},
		{ SERVICE_CONTROL_DEVICEEVENT			, "DEVICEEVENT"		},
		{ SERVICE_CONTROL_HARDWAREPROFILECHANGE	, "HARDWAREPROFILECHANGE" },
		{ SERVICE_CONTROL_POWEREVENT			, "POWEREVENT"		},
		{ SERVICE_CONTROL_SESSIONCHANGE			, "SESSIONCHANGE"	},
		{ SERVICE_CONTROL_PRESHUTDOWN			, "PRESHUTDOWN"		},
		{ SERVICE_CONTROL_TIMECHANGE			, "TIMECHANGE "		},
		{ SERVICE_CONTROL_TRIGGEREVENT			, "TRIGGEREVENT"	},
		{ SERVICE_CONTROL_USERMODEREBOOT		, "USERMODEREBOOT"	}
	};
	for (int i=0; size(ar); ++i)
		if (ar[i].Cmd == cmd)
			return String("SERVICE_CONTROL_") + ar[i].Name;
#endif
	return Convert::ToString(cmd);
}

String ReasonToString(SessionChangeReason reason) {
#if UCFG_TRC
	static const char * const table[] = {
		0,
		"ConsoleConnect",
		"ConsoleDisconnect",		
		"RemoteConnect",
		"RemoteDisconnect",
		"SessionLogon",
		"SessionLogoff",			
		"SessionLock",		
		"SessionUnlock",			
		"SessionRemoteControl",
		"SessionCreate",
		"SessionTerminate"
	};
	if (uint32_t(reason) < size(table) && table[int(reason)])
		return table[int(reason)];
#endif
	return Convert::ToString(int(reason));
}

bool ServiceBase::OnCommand(DWORD dwControl, DWORD dwEventType, void *lpEventData) {
	TRC(2, ServiceName << ": " << ServiceControlToString(dwControl));

	switch (dwControl) {
	case SERVICE_CONTROL_STOP:
		Stop();
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		Status = SERVICE_STOP_PENDING;
		OnShutdown();
		break;
	case SERVICE_CONTROL_PAUSE:
		Status = SERVICE_PAUSE_PENDING;
		OnPause();
		break;
	case SERVICE_CONTROL_CONTINUE:
		Status = SERVICE_CONTINUE_PENDING;
		OnContinue();
		break;
	case SERVICE_CONTROL_INTERROGATE:
		Status = m_status;
		break;
	case SERVICE_CONTROL_POWEREVENT:
		OnPowerEvent((PowerBroadcastStatus)dwEventType);
		break;
	case SERVICE_CONTROL_SESSIONCHANGE:
		{
			SessionChangeDescription scd = { (SessionChangeReason)dwEventType, (int)((WTSSESSION_NOTIFICATION*)lpEventData)->dwSessionId };
			TRC(2, ReasonToString(scd.Reason));
			OnSessionChange(scd);
		}
		break;
	case SERVICE_CONTROL_PARAMCHANGE:
		OnParamChange();
		break;
	case SERVICE_CONTROL_TIMECHANGE:
		OnTimeChange();
		break;
	case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
		OnHardwareProfileChange();
		break;
	default:
		if (dwControl >= 128 && dwControl <= 255)
			OnCustomCommand(dwControl);
		else
			return false;
	}
	return true;
}

DWORD WINAPI ServiceBase::HandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
	try {
		 return ((ServiceBase*)lpContext)->OnCommand(dwControl, dwEventType, lpEventData) ? NO_ERROR : ERROR_CALL_NOT_IMPLEMENTED;
	} catch (RCExc ex) {
		return HResultInCatch(ex);
	}
}

void ServiceBase::ServiceMainCallback(const vector<String>& args) {
	TRC(1, ServiceName);

	ServiceHandle = ::RegisterServiceCtrlHandlerEx(ServiceName, &HandlerEx, this);
	
	Win32Check(ServiceHandle != 0);
	Status = SERVICE_START_PENDING;
	
	try {
		signal(SIGINT, SIG_DFL);			// signals hang during logoff
		signal(SIGBREAK, SIG_DFL);

		OnStart();
		Status = SERVICE_RUNNING;
		m_evStop.lock();
	} catch (RCExc DBG_PARAM(ex)) {
		TRC(0, ex.what());
	}
	Status = SERVICE_STOPPED;
}

static ServiceBase *s_pServiceBase;

void WINAPI ServiceBase::ServiceMainFunction(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors) {	
	vector<String> args;
	for (int i=0; i<dwNumServicesArgs; ++i)
		args.push_back(lpServiceArgVectors[i]);
	exchange(s_pServiceBase, nullptr)->ServiceMainCallback(args);		//!!!? RC
}

void ServiceBase::Run() {
	SERVICE_TABLE_ENTRY serviceTable[2] = {
		{ (LPTSTR)(LPCTSTR)ServiceName, &ServiceMainFunction },
		{ 0, 0 }
	};
	s_pServiceBase = this;
	Win32Check(::StartServiceCtrlDispatcher(serviceTable));
}

CServiceObject::~CServiceObject() {
	Close();
}

void CServiceObject::Close() {
	if (m_handle)
		Win32Check(::CloseServiceHandle(exchange(m_handle, (SC_HANDLE)0)));
}

ServiceController::ServiceController(RCString serviceName, RCString machineName) {
	CSCManager sm(machineName);
	DWORD dwDesiredAccess = SC_MANAGER_ALL_ACCESS; //!!!
	Win32Check((m_handle = ::OpenService(sm.m_handle, serviceName, dwDesiredAccess))!=0);
}

void ServiceController::Delete() {
	Win32Check(::DeleteService(m_handle));
}

SERVICE_STATUS ServiceController::GetStatusEx() {
	SERVICE_STATUS r;
	Win32Check(::QueryServiceStatus(m_handle, &r));
	return r;	
}

void ServiceController::Start(const CStringVector& ar) {
	LPCTSTR *p = 0;
	if (!ar.empty()) {
		p = (LPCTSTR*)alloca(ar.size()*sizeof(LPCTSTR));
		for (size_t i=0; i<ar.size(); ++i)
			p[i] = ar[i];
	}
	Win32Check(::StartService(m_handle, (DWORD)ar.size(), p));
}

SERVICE_STATUS ServiceController::ExecuteCommand(DWORD dwControl) {
	SERVICE_STATUS result;
	Win32Check(::ControlService(m_handle, dwControl, &result));
	return result;
}

SERVICE_STATUS ServiceController::Stop() {
	return ExecuteCommand(SERVICE_CONTROL_STOP);
}

SERVICE_STATUS ServiceController::Pause() {
	return ExecuteCommand(SERVICE_CONTROL_PAUSE);
}

SERVICE_STATUS ServiceController::Continue() {
	return ExecuteCommand(SERVICE_CONTROL_CONTINUE);
}

SERVICE_STATUS ServiceController::Interrogate() {
	return ExecuteCommand(SERVICE_CONTROL_INTERROGATE);
}

CSCManager::CSCManager(RCString lpMachineName, RCString lpDatabaseName, DWORD dwDesiredAccess) {
	Open(lpMachineName, lpDatabaseName, dwDesiredAccess);
}

void CSCManager::Open(RCString lpMachineName, RCString lpDatabaseName, DWORD dwDesiredAccess) {
	Close();
	Win32Check((m_handle = ::OpenSCManager(lpMachineName, lpDatabaseName, dwDesiredAccess))!=0);
}

ServiceController CSCManager::CreateService(RCString serviceName, RCString displayName, DWORD dwDesiredAccess, DWORD dwServiceType,
	DWORD dwStartType, DWORD dwErrorControl, LPCTSTR lpBinaryPathName, LPCTSTR lpLoadOrderGroup,
	LPDWORD lpdwTagId, LPCTSTR lpDependencies, LPCTSTR lpServiceStartName, LPCTSTR lpPassword)
{
	ServiceController result;
	Win32Check((result.m_handle = ::CreateService(m_handle, serviceName, displayName, dwDesiredAccess, dwServiceType, dwStartType,
		dwErrorControl, lpBinaryPathName, lpLoadOrderGroup, lpdwTagId, lpDependencies,
		lpServiceStartName, lpPassword)) != 0);
	return result;
}

/*
CServiceHandler::CServiceHandler(RCString serviceName, LPHANDLER_FUNCTION_EX lpHandlerProc, void *ctx) {
	m_handle = ::RegisterServiceCtrlHandlerEx(serviceName, lpHandlerProc, ctx);
	Win32Check(m_handle != 0);
}

void CServiceHandler::SetServiceStatus(SERVICE_STATUS& serviceStatus) {
	Win32Check(::SetServiceStatus(m_handle, &serviceStatus));
}*/


}} // Ext::ServiceProcess::


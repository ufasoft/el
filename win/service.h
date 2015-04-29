/*######   Copyright (c) 2011-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <winsvc.h>

#include <el/libext/win32/ext-full-win.h>

namespace Ext { namespace ServiceProcess {

ENUM_CLASS(ServiceControllerStatus) {
	ContinuePending			= SERVICE_CONTINUE_PENDING,
	Paused					= SERVICE_PAUSED,
	PausePending			= SERVICE_PAUSE_PENDING,
	Running					= SERVICE_RUNNING,
	StartPending			= SERVICE_START_PENDING,
	Stopped					= SERVICE_STOPPED,
	StopPending				= SERVICE_STOP_PENDING
} END_ENUM_CLASS(ServiceControllerStatus);

ENUM_CLASS(SessionChangeReason) {
	ConsoleConnect			= WTS_CONSOLE_CONNECT,
	ConsoleDisconnect		= WTS_CONSOLE_DISCONNECT,
	RemoteConnect			= WTS_REMOTE_CONNECT,
	RemoteDisconnect		= WTS_REMOTE_DISCONNECT,
	SessionLogon			= WTS_SESSION_LOGON,
	SessionLogoff			= WTS_SESSION_LOGOFF,
	SessionLock				= WTS_SESSION_LOCK,
	SessionUnlock			= WTS_SESSION_UNLOCK,
	SessionRemoteControl	= WTS_SESSION_REMOTE_CONTROL,
	SessionCreate			= WTS_SESSION_CREATE,
	SessionTerminate		= WTS_SESSION_TERMINATE
} END_ENUM_CLASS(SessionChangeReason);

struct SessionChangeDescription {
	SessionChangeReason Reason;
	int SessionId;
};

ENUM_CLASS(PowerBroadcastStatus) {
	BatteryLow				= PBT_APMBATTERYLOW,
	OemEvent				= PBT_APMOEMEVENT,
	PowerStatusChange		= PBT_APMPOWERSTATUSCHANGE,
	QuerySuspend			= PBT_APMQUERYSUSPEND,
	QuerySuspendFailed		= PBT_APMQUERYSUSPENDFAILED,
	ResumeAutomatic			= PBT_APMRESUMEAUTOMATIC,
	ResumeCritical			= PBT_APMRESUMECRITICAL,
	ResumeSuspend			= PBT_APMRESUMESUSPEND,
	Suspend					= PBT_APMSUSPEND
} END_ENUM_CLASS(PowerBroadcastStatus);


class ServiceBase {
public:
	static const int MaxNameLength = 256;

	String ServiceName;
	DWORD ExitCode;
	bool CanStop, CanShutdown;
	CBool CanPauseAndContinue, CanHandlePowerEvent, CanHandleSessionChangeEvent;

	ServiceBase()
		:	ExitCode(0)
		,	ServiceHandle(0)
	{
		m_status = SERVICE_STOPPED;

		CanStop = CanShutdown = true;
	}		

	void Run();

	DWORD get_Status() { return m_status; }
	void put_Status(DWORD v);
	DEFPROP(DWORD, Status);

	void Stop();
	void ServiceMainCallback(const std::vector<String>& args);
protected:
	SERVICE_STATUS_HANDLE ServiceHandle;

	virtual void OnStart();
	virtual void OnStop();
	virtual void OnShutdown();
	virtual void OnPause();
	virtual void OnContinue();
	virtual void OnPowerEvent(PowerBroadcastStatus) {}
	virtual void OnSessionChange(const SessionChangeDescription&) {}
	virtual void OnParamChange() {}
	virtual void OnTimeChange() {}
	virtual void OnHardwareProfileChange() {}
	virtual void OnCustomCommand(int command) {}

	virtual bool OnCommand(DWORD dwControl, DWORD dwEventType, void *lpEventData);
private:
	AutoResetEvent m_evStop;
	DWORD m_status;

	static DWORD WINAPI HandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	static void WINAPI ServiceMainFunction(DWORD dwNumServicesArgs, LPTSTR *lpServiceArgVectors);
};

class AFX_CLASS CServiceObject {
public:
	CServiceObject()
		:	m_handle(0)
	{}

	CServiceObject(const CServiceObject& s)
		:	m_handle(s.m_handle)
	{
		((CServiceObject&)s).m_handle = 0;
	}

	void operator=(CServiceObject& s) {
		Close();
		m_handle = s.m_handle;
		s.m_handle = 0;
	}

	virtual ~CServiceObject();
	void Close();

	SC_HANDLE m_handle;
};

class ServiceController : public CServiceObject {		//!!!
public:
	ServiceController() {
	}

	ServiceController(RCString serviceName, RCString machineName = nullptr);

	SERVICE_STATUS GetStatusEx();

	ServiceControllerStatus get_Status() { return (ServiceControllerStatus)GetStatusEx().dwCurrentState; }
	DEFPROP_GET(ServiceControllerStatus, Status);

	void Delete();
	void Start(const CStringVector& ar = CStringVector());
	SERVICE_STATUS ExecuteCommand(DWORD dwControl);
	SERVICE_STATUS Stop();
	SERVICE_STATUS Pause();
	SERVICE_STATUS Continue();
	SERVICE_STATUS Interrogate();
};

class CSCManager : public CServiceObject {	//!!!
	static const DWORD DEFAULT_ACCESS = MAXIMUM_ALLOWED;
	//!!!	const DWORD DEFAULT_ACCESS = SC_MANAGER_ALL_ACCESS;
public:
	CSCManager(RCString lpMachineName = nullptr, RCString lpDatabaseName = nullptr, DWORD dwDesiredAccess = DEFAULT_ACCESS);
	void Open(RCString lpMachineName = nullptr, RCString lpDatabaseName = nullptr, DWORD dwDesiredAccess = DEFAULT_ACCESS);
	ServiceController CreateService(RCString serviceName, RCString displayName, DWORD dwDesiredAccess = DEFAULT_ACCESS,
		DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS, DWORD dwStartType = SERVICE_AUTO_START,
		DWORD dwErrorControl = SERVICE_ERROR_NORMAL, LPCTSTR lpBinaryPathName = 0, LPCTSTR lpLoadOrderGroup = 0,
		LPDWORD lpdwTagId = 0, LPCTSTR lpDependencies = 0, LPCTSTR lpServiceStartName = 0,
		LPCTSTR lpPassword = 0);
};


}} // Ext::ServiceProcess::

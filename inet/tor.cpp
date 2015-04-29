/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <tor/tor_config.h>

#include <el/ext.h>

#include "tor.h"

#include <el/libext/win32/ext-win.h>


extern "C" {

void tor_cleanup();

const char *libor_get_digests() { return ""; }
const char *tor_get_digests() { return ""; }


} // "C"


namespace Ext {
namespace Inet {

static mutex s_mtxSingleton;
static TorProxy *s_singleton;

TorProxy::TorProxy(bool bStart, bool bQuiet)
	:	ControlStream(m_sockControl)
{
	if (!bStart)
		return;
	m_torThread = new TorThread(m_tr);
	m_torThread->Quiet = bQuiet;

	uint16_t portSocks, portControl;
	{
		Socket sock(AddressFamily::InterNetwork);
		sock.Bind();
		portSocks = sock.LocalEndPoint.Port;

		Socket sock2(AddressFamily::InterNetwork);
		sock2.Bind();
		portControl = sock2.LocalEndPoint.Port;
	}
	m_epSocks = IPEndPoint(IPAddress::Loopback, portSocks);
	m_epControl = IPEndPoint(IPAddress::Loopback, portControl);		

	TorParams torParams = { System.ExeFilePath.parent_path() / "tor.rc", Convert::ToString(portSocks), Convert::ToString(portControl) };
	
	Address = Uri("http://127.0.0.1:" + Convert::ToString(portSocks));
	Type = ProxyType::Socks;
	
	m_torThread->m_torParams = torParams;	
	m_torThread->Start();

	TorStatusThread = new class TorStatusThread(m_tr, _self);
	TorStatusThread->Start();

	Sleep(1000);			//!!! delay for TorThread to open SOCKS port
}

TorProxy::~TorProxy() {
	SendSignal("HALT");
	m_tr.StopChilds();
	s_singleton = 0;
}

static void CleanupTorProxy() {
	delete exchange(s_singleton, nullptr);
}

ptr<TorProxy> __stdcall TorProxy::GetSingleton(bool bQuiet) {
	if (s_singleton)
		return s_singleton;
	EXT_LOCK(s_mtxSingleton) {
		if (s_singleton)
			return s_singleton;
		ptr<TorProxy> r = new TorProxy(true, bQuiet);
		++(r->m_aRef);
		RegisterAtExit(CleanupTorProxy);
		s_singleton = r.get();
		return r;
	}
}

regex s_re("^(\\d+)([ +-])(\\w+)=?\\s*(.*)$");

pair<String, vector<String> > TorProxy::ReadValue(bool bShouldBeEvent) {
LAB_BEGIN:
	vector<String> vec;
	String keyword;
	StreamReader r(ControlStream, Encoding::UTF8);
	String s = r.ReadLine();
	if (s == nullptr)
		Throw(E_FAIL);

	TRC(7, "Read: " << s);

	if (s == "250 OK")
		return make_pair("", vec);
	cmatch m;
	if (regex_search(s.c_str(), m, s_re)) {
		int code = atoi(String(m[1]));
		keyword = m[3];
		try {
			if (code>=600 && code<=699) {
				if (code != 650)
					Throw(E_FAIL);
				String rest = m[4];
				if (keyword == "CIRC")
					ProcessCircuitEvent(rest);
				else if (keyword == "STREAM")
					ProcessStreamEvent(rest);
				else if (keyword == "STATUS_GENERAL")
					ProcessStatusGeneralEvent(rest);
				else if (keyword == "STATUS_CLIENT")
					ProcessStatusClientEvent(rest);
				else if (keyword == "ADDRMAP")
					ProcessAddrmapEvent(rest);
				else if (keyword == "NEWDESC")
					ProcessNewdescEvent(rest);
				if (bShouldBeEvent)
					return pair<String, vector<String> >();
				goto LAB_BEGIN;
			}
		} catch (Exception& ex) {
			if (ex.code() != ExtErr::Protocol_Violation)
				throw;
			goto LAB_BEGIN;
		}
		if (bShouldBeEvent)
			Throw(E_FAIL);
		if (code != 250)
			Throw(E_FAIL);
		if (String(m[2]) == "-")
			vec.push_back(m[4]);
		else {
			while (true) {
				s = r.ReadLine();

				TRC(7, "Read: " << s);

				if (s == ".")
					break;
				vec.push_back(s);
			}
		}
	} else
		Throw(E_FAIL);	
	return make_pair(keyword, vec);
}

map<String, vector<String> > TorProxy::ReadValues() {
	map<String, vector<String> > m;
	while (true) {
		pair<String, vector<String> > pp = ReadValue();
		if (pp.first == "")
			break;
		m[pp.first] = pp.second;
	}
	return m;
}

void TorProxy::ReadEvents() {
	ReadValue(true);
}

map<String, vector<String> > TorProxy::Send(RCString s) {
	TRC(0, "Send> " << s);			

	String line = s + "\r\n";
	Blob blob = Encoding::Default().GetBytes(line);
	ControlStream.WriteBuffer(blob.constData(), blob.Size);
	return ReadValues();
}



#	pragma comment(lib, "libevent")
#	pragma comment(lib, "openssl")

extern "C" {

typedef void (*log_callback)(int severity, uint32_t domain, const char *msg);

#define LOG_DEBUG   7
/** Info-level severity: for messages that appear frequently during normal
 * operation. */
#define LOG_INFO    6
/** Notice-level severity: for messages that appear infrequently
 * during normal operation; that the user will probably care about;
 * and that are not errors.
 */
#define LOG_NOTICE  5
/** Warn-level severity: for messages that only appear when something has gone
 * wrong. */
#define LOG_WARN    4
/** Error-level severity: for messages that only appear when something has gone
 * very wrong, and the Tor process can no longer proceed. */
#define LOG_ERR     3

typedef uint32_t log_domain_mask_t;

/** Configures which severities are logged for each logging domain for a given
 * log target. */
typedef struct log_severity_list_t {
  /** For each log severity, a bitmask of which domains a given logger is
   * logging. */
  log_domain_mask_t masks[LOG_DEBUG-LOG_ERR+1];
} log_severity_list_t;

int add_file_log(const log_severity_list_t *severity, const char *filename,	const int truncate);
int _cdecl tor_main(int argc, char *argv[]);
void init_logging(int disable_startup_queue);
void _cdecl switch_logs_debug(void);
int _cdecl add_callback_log(const log_severity_list_t *severity, log_callback cb);

extern int log_global_min_severity_;
struct event_base *tor_libevent_get_base();
int event_base_loopbreak(struct event_base *);

bool s_bStopping;

int __cdecl nt_service_is_stopping() {
	return s_bStopping;
}

} // "C"

void _cdecl MyLogCallback(int severity, uint32_t domain, const char *msg) {
	TRC(0, msg);

#ifdef _DEBUG
	log_global_min_severity_ = 7;
#endif
}

static jmp_buf s_jbAbort;

extern "C" void __cdecl MyExit(int c) {
	longjmp(s_jbAbort, E_ABORT);
}

extern "C" void __cdecl MyAbort() {
	longjmp(s_jbAbort, E_ABORT);
}

TorThread::~TorThread() {
}

void TorThread::Stop() {
	s_bStopping = true;
	event_base_loopbreak(tor_libevent_get_base());
}

#pragma warning(disable: 4611)

static void CallTor(vector<const char *>& args) {
	if (HRESULT hr = setjmp(s_jbAbort)) {
		if (hr != E_ABORT)
			Throw(hr);
	} else {
#ifdef _DEBUG
		init_logging(0);

		log_severity_list_t sevs;
		memset(&sevs, 0xFF, sizeof sevs);
		add_file_log(&sevs, "c:\\var\\log\\tor.log", 1);
#endif
		tor_main(args.size() - 1, (char **)&args[0]);			//!!!?
	}
}

void TorThread::Execute() {
	Name = "TorThread";
#ifdef X_DEBUG//!!!D
	Sleep(100000);
#endif

	DBG_LOCAL_IGNORE(E_ABORT);

	vector<const char *> args;
	args.push_back("tor");
	
	args.push_back("-f");
	args.push_back(m_torParams.torrc.native());
	
	args.push_back("-SocksPort");
	args.push_back(m_torParams.sPortSocks);
	
	args.push_back("-ControlPort");
	args.push_back(m_torParams.sPortControl);

	args.push_back("-HashedControlPassword");
	args.push_back("16:3C3A16A81BFAA43A60211474FB21B34DF35E6A3C6FE194D59160D80CEB");

	args.push_back("--allow-missing-torrc");

	if (Quiet)
		args.push_back("--quiet");
	args.push_back(nullptr);		

	create_directories(Environment::GetFolderPath(SpecialFolder::ApplicationData) / UCFG_MANUFACTURER / "tor");
	CallTor(args);
}

void TorStatusThread::BeforeStart() {
	for (int i=0;; ++i) {
		try {
			DBG_LOCAL_IGNORE_CONDITION(errc::connection_refused);

			TRC(0, "Connecting to Control port: " << TorProxy.m_epControl);

			TorProxy.m_sockControl.Connect(TorProxy.m_epControl);
			break;
		} catch (RCExc) {
			if (i == 3) {
				throw;
			}
			Thread::Sleep(500);
		}
	}

	TorProxy.Authenticate();		
	TorProxy.Send("SETEVENTS EXTENDED CIRC STREAM STATUS_GENERAL STATUS_CLIENT NEWDESC ADDRMAP");
}

void TorStatusThread::Execute() {
	Name = "TorStatusThread";

//	TorProxy.m_sockControl;	//!!!?
	try {
		DBG_LOCAL_IGNORE_CONDITION(errc::connection_reset);	//!!!?


	//!!!	TorProxy.GetInfo("desc/all-recent");

		while (true) {
			TorProxy.m_sockControl.EventSelect((HANDLE)Handle(m_ev), FD_READ|FD_CLOSE);
			Wait((HANDLE)Handle(m_ev));
		
			if (TorProxy.m_sockControl.EnumNetworkEvents().lNetworkEvents & FD_CLOSE)
				break;
	//!!!R		if (TorProxy.m_sockControl.Send(0, 0) == SOCKET_ERROR)		// is still connected?
	//!!!R			break;

			TorProxy.m_sockControl.EventSelect();
			TorProxy.m_sockControl.Blocking = true;

			String cmd;
			while (true) {
				EXT_LOCK (m_cs) {
					if (!Dequeue(m_queue, cmd))
						break;
				}
				TorProxy.Send(cmd);
			}
			for (int avail; avail=TorProxy.m_sockControl.Available;)
				TorProxy.ReadEvents();

	#ifdef X_DEBUG//!!!D
			if (!g_sNode.IsEmpty()) {
	//			TorProxy.GetInfo("desc/all-recent");
	//			TorProxy.GetInfo("desc/name/"+g_sNode+String("\0",1));
			}

	#endif

			if (cmd == "SIGNAL HALT")
				break;

	/*!!!
			vector<String> circs = TorChainer.GetInfo("stream-status");
			CCirMap curCircs;
			if (circs.size()!=1 || circs[0]!="") {
				for (int k=0; k<circs.size(); ++k) {
					String circ = circs[k];
					static Regex s_reCircStatus("^(\\d+)\\s(\\w+)\\s(\\d+)\\s(.*)?$");
					if (Match m = s_reCircStatus.Match(circ)) {
						int sid = atoi(m[1].Value);
						String sstat = m[2].Value;
						int cid = atoi(m[3].Value);
						String target = m[4].Value;
						vector<String> path;
						path.push_back(target);
						curCircs[sid] = path;
						if (m_curCircs.find(sid) == m_curCircs.end()) {
							m_curCircs[sid] = path;
							TorChainer.EventConnectionsChanged->OnConnAdded(sid, path);
						}
					} else
						Throw(E_FAIL);
				}
			}
			for (CCirMap::iterator i=m_curCircs.begin(), e=m_curCircs.end(); i!=e; ) {
				CCirMap::iterator j = i++;
				if (curCircs.find(j->first)==curCircs.end()) {
					TorChainer.EventConnectionsChanged->OnConnClosed(j->first);
					m_curCircs.erase(j);
				}
			}
			*/
			/*!!!R
	#ifdef _DEBUG//!!!D
			Thread::Sleep(5000);
	#else
			Thread::Sleep(1000);
	#endif
			*/
		}
	} catch (RCExc ex) {
		cerr << ex.what() << endl;
		if (!m_bStop)
			throw;
	}
}

}} // Ext::Inet::


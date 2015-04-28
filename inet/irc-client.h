/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/inet/async-text-client.h>

namespace Ext { namespace Inet { namespace Irc {
using namespace Ext;
	

enum IrcResponse {
	RPL_WELCOME		= 1,		RPL_YOURHOST	= 2,		RPL_CREATED		= 3,		RPL_MYINFO		= 4,		RPL_BOUNCE		= 5,
	RPL_AWAY		= 301,		RPL_USERHOST	= 302,		RPL_ISON		= 303,		RPL_UNAWAY		= 305,		RPL_NOWAWAY		= 306,
	RPL_WHOISUSER	= 311,		RPL_WHOISSERVER = 312,		RPL_WHOISOPERATOR = 312,	RPL_WHOWASUSER	= 314,		RPL_ENDOFWHO	= 315,	RPL_WHOISIDLE	= 317,	RPL_ENDOFWHOIS	= 318,	RPL_WHOISCHANNELS = 319,
	RPL_LISTSTART	= 321,		RPL_LIST		= 322,		RPL_LISTEND = 323,			RPL_CHANNELMODEIS = 324,	RPL_UNIQOPIS	= 325,
	RPL_NOTOPIC		= 331,		RPL_TOPIC		= 332,
	RPL_INVITING	= 341,		RPL_SUMMONING	= 342,		RPL_INVITELIST = 346,		RPL_ENDOFINVITELIST = 347,	RPL_EXCEPTLIST = 348,	RPL_ENDOFEXCEPTLIST = 349,
	RPL_VERSION		= 351,		RPL_WHOREPLY	= 352,		RPL_NAMREPLY	= 353,
	RPL_LINKS		= 364,		RPL_ENDOFLINKS	= 365,		RPL_ENDOFNAMES	= 366,		RPL_BANLIST		= 367,		RPL_ENDOFBANLIST = 368, RPL_ENDOFWHOWAS = 369,
	RPL_INFO		= 371,		RPL_MOTD		= 372,		RPL_ENDOFINFO	= 374,		RPL_MOTDSTART	= 375,		RPL_ENDOFMOTD = 376,
	RPL_YOUREOPER	= 381,		RPL_REHASHING	= 382,		RPL_YOURESERVICE = 383,
	RPL_TIME		= 391,		RPL_USERSSTART	= 392,		RPL_USERS = 393		//...

};

struct IrcUserInfo {
	String Name;
	String Host;
	String Server;
	String Nick;
	String RealName;
};

class IrcClient : public AsyncTextClient {
public:
	String Nick;
	String Username;
	String RealName;
	mutex Mtx;

	typedef unordered_map<String, vector<IrcUserInfo>> CWhoList;
	CWhoList m_whoLists;

	typedef unordered_map<String, unordered_set<String>> CNameList;
	CNameList m_nameList;
	
	deque<String> CommandQueue;

	IrcClient();
	void Connect();
	void Disconnect();
	
	void SetNick(RCString nick);	
//!!!	std::vector<IrcUserInfo> Who(RCString channelName);
protected:

	void OnLine(RCString line) override;
	void OnLastLine(RCString line) override {} // usually broken line

	virtual void OnAuth();
	virtual void OnCreatedConnection() {}
	virtual void OnUserHost(RCString host) {}
	virtual void OnNickNamesComplete(RCString channel, const unordered_set<String>& nicks) {}
	virtual void OnUserListComplete(RCString channel, const vector<IrcUserInfo>& userList) {}
	virtual void OnPing(RCString server1, RCString server2);
private:
//	ManualResetEvent m_ev;
	CBool m_bUsernameSent;
};



}}} // Ext::Inet::Irc::


/*######   Copyright (c) 2019      Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/libext/ext-net.h>
#include "http.h"

namespace Ext { namespace Inet {

class HttpServer : public SocketThread {
public:
	IPEndPoint ListeningEndpoint;
protected:
	Socket m_sock;

protected:
	void BeforeStart() override;
	void Execute() override;
};


class HttpConnection : public Thread {
public:
   	HttpServer& Server;
	Socket m_sock;
	NetworkStream m_stm;
   	BinaryReader Reader;
   	BinaryWriter Writer;
	CHttpHeader Header;
	HttpRequest Request;
	HttpResponse Response;
	CBool KeepAlive;

	HttpConnection(HttpServer& server, Socket&& sock);
//   	void Send(const Record& rec);
protected:
	void ReadHeaders();
	void ReadBody();
	void ReadRequest();
	void WriteHeaders();
	void WriteBody();
	void WriteResponse();
	void Execute() override;
};


}} // Ext::Inet::

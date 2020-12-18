/*######   Copyright (c) 2019      Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include <el/ext.h>

#include "http-server.h"

namespace Ext { namespace Inet {

void HttpServer::BeforeStart() {
	m_sock.ReuseAddress = true;
	m_sock.Bind(ListeningEndpoint);
	TRC(2, "Listening on HTTP endpoint  " << ListeningEndpoint);
	m_sock.Listen();
}

void HttpServer::Execute() {
	Name = "FastCgiServer";

	SocketKeeper sk(_self, m_sock);

	DBG_LOCAL_IGNORE_CONDITION(errc::interrupted);

	while (!Ext::this_thread::interruption_requested()) {
		pair<Socket, IPEndPoint> sep = m_sock.Accept();
		TRC(2, "HTTP from " << sep.second);
		(new HttpConnection(_self, move(sep.first)))->Start();
	}
}

HttpConnection::HttpConnection(HttpServer& server, Socket&& sock)
	: Server(server)
	, m_stm(m_sock)
	, Reader(m_stm)
	, Writer(m_stm)
	, m_sock(move(sock))
{

}

void HttpConnection::ReadHeaders() {
	vector<String> headers = ReadHttpHeader(m_stm);
	if (headers.empty())
		Throw(E_FAIL);
	static regex reHttpReq("^(\\S+)\\s+(\\S+)");
	smatch m;
	string sh(headers[0]);
	if (regex_match(sh, m, reHttpReq)) {
		Request.Method = String(m[1]).ToUpper();
		Request.Path = m[2];
	}
	for (size_t i = 1; i < headers.size(); ++i) {
		auto ar = headers[i].Split(":");
		Request.Headers.Set(ar[0].Trim(), ar[1].Trim());
	}
}

void HttpConnection::ReadBody() {
}

void HttpConnection::ReadRequest() {
	ReadHeaders();
	ReadBody();
}

void HttpConnection::WriteHeaders() {
}

void HttpConnection::WriteBody() {
}

void HttpConnection::WriteResponse() {
	WriteHeaders();
	WriteBody();
}



void HttpConnection::Execute() {
	Name = "HttpConnection";

	do {
		ReadRequest();
		WriteResponse();
	} while (KeepAlive);

}


}} // Ext::Inet::

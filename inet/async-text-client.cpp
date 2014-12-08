#include <el/ext.h>

#include "async-text-client.h"

namespace Ext { namespace Inet { 

void AsyncTextClient::Execute() {
	DBG_LOCAL_IGNORE_WIN32(WSAECONNREFUSED);
	DBG_LOCAL_IGNORE_WIN32(WSAECONNABORTED);
	DBG_LOCAL_IGNORE_WIN32(WSAECONNRESET);
	DBG_LOCAL_IGNORE(E_EXT_EndOfStream);

	for (pair<String, bool> pp; !m_bStop && (pp=R.ReadLineEx()).first!=nullptr;) {
		if (FirstByte != -1)
			pp.first = String((char)std::exchange(FirstByte, -1), 1) + pp.first;
		if (pp.second)
			OnLine(pp.first);
		else
			OnLastLine(pp.first);										// last line can be broken
	}
}

void AsyncTextClient::SendPendingData() {
	EXT_LOCK (MtxSend) {
		if (!DataToSend.empty())
			Tcp.Stream.WriteBuf(W.Encoding.GetBytes(exchange(DataToSend, String())));
	}
}

void AsyncTextClient::Send(RCString cmd) {
	EXT_LOCK (MtxSend) {
		if (Tcp.Client.Valid())
			W.WriteLine(cmd);
		else
			DataToSend += cmd+W.NewLine;
	}
}


}} // Ext::Inet::


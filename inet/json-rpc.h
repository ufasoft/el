#pragma once

namespace Ext { namespace Inet {
using namespace Ext;

enum json_rpc_errc {
	ParseError = -32700,
	InvalidParams = -32602,
	MethodNotFound = -32601,
	InvalidRequest = -32600,
};

const error_category& json_rpc_category();

inline error_code make_error_code(json_rpc_errc v) { return error_code((int)v, json_rpc_category()); }

typedef unordered_map<String, VarValue> CJsonNamedParams;

class JsonRpcException : public Exception {
	typedef Exception base;
public:
//	int Code;
	VarValue Data;

	JsonRpcException(int code, RCString msg)
		:	base(error_code(code, json_rpc_category()), msg)		
//		,	Code(code)
	{}

	~JsonRpcException() noexcept {}
};

class JsonRpcRequest : public Object {
public:
	typedef InterlockedPolicy interlocked_policy;

	VarValue Id;
	String Method;
	VarValue Params;
	CBool V20;

	String ToString() const;
};

class JsonResponse {
public:
	ptr<JsonRpcRequest> Request;

	VarValue Id;
	VarValue Result;
	VarValue ErrorVal;
	VarValue Data;

	String JsonMessage;
	int Code;
	bool Success;
	bool V20;

	JsonResponse()
		:	Success(true)
		,	V20(false)
		,	Code(0)
		,	Id(nullptr)
	{}

	VarValue ToVarValue() const;
	String ToString() const;
};

class JsonRpc {
public:
	CBool V20;

	JsonRpc()
		: m_aNextId(1)
	{}

	static bool TryAsRequest(const VarValue& v, JsonRpcRequest& req);
	String Request(RCString method, const vector<VarValue>& params = vector<VarValue>(), ptr<JsonRpcRequest> req = nullptr);
	String Request(RCString method, const CJsonNamedParams& params, ptr<JsonRpcRequest> req = nullptr);
	VarValue ProcessResponse(const VarValue& vjresp);
	VarValue Call(Stream& stm, RCString method, const vector<VarValue>& params = vector<VarValue>());
	VarValue Call(Stream& stm, RCString method, const VarValue& arg0);
	VarValue Call(Stream& stm, RCString method, const VarValue& arg0, const VarValue& arg1);

	String Notification(RCString method, const vector<VarValue>& params = vector<VarValue>(), ptr<JsonRpcRequest> req = nullptr);
	JsonResponse Response(const VarValue& v);
	void ServerLoop(Stream& stm);
	
protected:
	virtual void SendChunk(Stream& stm, const ConstBuf& cbuf);
	virtual VarValue CallMethod(RCString name, const VarValue& params) { Throw(E_EXT_JSON_RPC_MethodNotFound); }
private:
	std::mutex MtxReqs;

	typedef LruMap<int, ptr<JsonRpcRequest>> CRequests;
	CRequests m_reqs;

	atomic<int> m_aNextId;

	void PrepareRequest(JsonRpcRequest *req);
	VarValue ProcessRequest(const VarValue& v);
};


}} // Ext::Inet::


namespace std {

template<> struct is_error_code_enum<Ext::Inet::json_rpc_errc> : true_type {};

}

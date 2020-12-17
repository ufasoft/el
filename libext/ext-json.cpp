/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_JSON == UCFG_JSON_JANSSON
#	include <jansson.h>
#	ifndef json_boolean																	//BC backward compatibility for jansson < 2.x
#		define json_boolean(val)      ((val) ? json_true() : json_false())
#	endif
#elif UCFG_JSON == UCFG_JSON_JSONCPP
#	include <json/reader.h>
#endif

namespace Ext {
using namespace std;

#if UCFG_JSON == UCFG_JSON_JANSSON

static void *MallocWrap(size_t size) {
	return Malloc(size);
}

static void FreeWrap(void *p) {
	return free(p);
}

static int s_initJanssonMalloc = (::json_set_alloc_funcs(&MallocWrap, &FreeWrap), 1);			// Mandatory, because we are using Ext::Free() for Jansson created objects

class JsonHandle : noncopyable {
public:
	JsonHandle(json_t *h = 0)
		: m_h(h)
	{}

	~JsonHandle() {
		if (m_h)
			::json_decref(m_h);
	}

	JsonHandle& operator=(json_t *h) {
		if (m_h)
			::json_decref(m_h);
		m_h = h;
		return _self;
	}

	operator json_t *() { return m_h; }
	operator const json_t *() const { return m_h; }
	const json_t * get() const { return m_h; }
	json_t *Detach() { return exchange(m_h, nullptr); }
private:
	json_t *m_h;
};

class JsonVarValueObj : public VarValueObj {
public:
	JsonVarValueObj(json_t *json)
		: m_json(json)
	{}

	bool HasKey(RCString key) const override {
		return ::json_object_get(m_json, key);
	}

	VarType type() const override {
		switch (json_typeof(m_json.get())) {
		case JSON_NULL:		return VarType::Null;
		case JSON_INTEGER:	return VarType::Int;
		case JSON_REAL:		return VarType::Float;
		case JSON_TRUE:
		case JSON_FALSE:	return VarType::Bool;
		case JSON_STRING:	return VarType::String;
		case JSON_ARRAY:	return VarType::Array;
		case JSON_OBJECT:	return VarType::Map;
		default:
			Throw(E_NOTIMPL);
		}
	}

	size_t size() const override {
		switch (type()) {
		case VarType::Array: return ::json_array_size(m_json);
		case VarType::Map: return ::json_object_size(m_json);
		default:
			Throw(E_NOTIMPL);
		}
	}

	VarValue operator[](int idx) const override {
		return FromJsonT(::json_incref(::json_array_get(m_json, idx)));
	}

	VarValue operator[](RCString key) const override {
		return FromJsonT(::json_incref(::json_object_get(m_json, key)));
	}

	String ToString() const override {
		if (const char *s = ::json_string_value(m_json))
			return Encoding::UTF8.GetChars(Span((const uint8_t*)s, strlen(s)));
		Throw(ExtErr::InvalidCast);
	}

	int64_t ToInt64() const override {
		return ::json_integer_value(m_json);
	}

	bool ToBool() const override {
		if (type() != VarType::Bool)
			Throw(E_INVALIDARG);
		return json_is_true(m_json.get());
	}

	double ToDouble() const override {
		if (type() == VarType::Int)
			return double(::json_integer_value(m_json));
		else
			return ::json_real_value(m_json);
	}

	static VarValue FromJsonT(json_t *json) {
		VarValue r;
		if (json)
			r.m_pimpl = new JsonVarValueObj(json);
		return r;
	}

	void Set(int idx, const VarValue& v) override {
		Throw(E_NOTIMPL);
	}

	void Set(RCString key, const VarValue& v) override {
		Throw(E_NOTIMPL);
	}

	std::vector<String> Keys() const override {
		if (type() != VarType::Map)
			Throw(E_INVALIDARG);
		std::vector<String> r;
		for (void *it=::json_object_iter((json_t*)m_json.get()); it; it=::json_object_iter_next((json_t*)m_json.get(), it))
			r.push_back(::json_object_iter_key(it));
		return r;
	}

	void Print(ostream& os) const override {
		size_t flags = JSON_ENCODE_ANY | ((os.flags() & ios::adjustfield) == ios::left ? JSON_COMPACT : JSON_INDENT(2));
		if (char *s = json_dumps(m_json, flags)) {
			os << s;
			FreeWrap(s);
		} else
			os << "null";
	}
private:
	JsonHandle m_json;
};


class JsonExc : public Exception {
	typedef Exception base;
public:
	json_error_t m_err;

	JsonExc(const json_error_t& err)
		:	base(ExtErr::JSON_Parse)
		,	m_err(err)
	{}

	String get_Message() const override {
		return EXT_STR(m_err.text << " at line " << m_err.line);
	}
};

void JanssonCheck(bool r, const json_error_t& err) {
	if (!r)
		throw JsonExc(err);
}

VarValue AFXAPI ParseJson(RCString s) {
	json_error_t err;
	json_t *json = ::json_loads(s, 0, &err);
	JanssonCheck(json, err);
	return JsonVarValueObj::FromJsonT(json);
}

class JsonParser : public MarkupParser {
public:
	static json_t *CopyToJsonT(const VarValue& v);

	static VarValue CopyToJsonValue(const VarValue& v) {
		return JsonVarValueObj::FromJsonT(CopyToJsonT(v));
	}
protected:
	VarValue Parse(istream& is, Encoding *enc) override {
		String full;
		for (string s; getline(is, s);)
			full += s;
		return ParseJson(full);
	}

	struct CallbackData {
		Stream *m_pStm;
		Blob LastChunk;
		int LastChunkPos;
		bool Eof;

		CallbackData()
			:	Eof(false)
		{}
	};

	static size_t LoadCallback(void *buffer, size_t buflen, void *data) {
		CallbackData& cbd = *(CallbackData*)data;
		size_t r;
		if (cbd.LastChunkPos < 0) {
			int off = int(ssize_t(cbd.LastChunk.size()) + cbd.LastChunkPos);
			memcpy(buffer, cbd.LastChunk.constData() + off, r = std::min(buflen, size_t(cbd.LastChunk.size()) - off));
			cbd.LastChunkPos += int(r);
		} else {
			cbd.LastChunkPos += cbd.LastChunk.size();
			r = cbd.m_pStm->Read(buffer, buflen);
			cbd.Eof |= true;
			cbd.LastChunk = Blob(buffer, r);
		}
		return r;
	}

	pair<VarValue, Blob> ParseStream(Stream& stm, RCSpan preBuf) override {
#if JANSSON_VERSION_HEX >= 0x020400
		CallbackData cbd;
		cbd.LastChunk = preBuf;
		cbd.m_pStm = &stm;
		cbd.LastChunkPos = int(-(ssize_t)preBuf.size());

		json_error_t err;
		if (json_t *json = json_load_callback(&LoadCallback, &cbd, JSON_DISABLE_EOF_CHECK, &err)) {
			int off = err.position - std::max(cbd.LastChunkPos, 0);
			return make_pair(JsonVarValueObj::FromJsonT(json), Blob(cbd.LastChunk.constData() + off, cbd.LastChunk.size() - off));
		} else {
			if (!cbd.Eof)
				JanssonCheck(json, err);
			return make_pair(VarValue(), Blob(nullptr));
		}
#else
		Throw(E_NOTIMPL);
#endif
	}

	void Print(ostream& os, const VarValue& v) override {
		JsonHandle jh(CopyToJsonT(v));
		size_t flags = (Indent ? JSON_INDENT(Indent) : 0) | (Compact ? JSON_COMPACT : 0);
		if ((os.flags() & ios::adjustfield) == ios::left)
			flags |= JSON_COMPACT;
		if (char *s = json_dumps(jh, flags)) {
			os << s;
			FreeWrap(s);
		} else
			os << "null";
	}
};

json_t *JsonParser::CopyToJsonT(const VarValue& v) {
	switch (v.type()) {
	case VarType::Null:
		return ::json_null();
	case VarType::Bool:
		return json_boolean(v.ToBool());
	case VarType::Int:
		return ::json_integer(v.ToInt64());
	case VarType::Float:
		return ::json_real(v.ToDouble());
	case VarType::String:
		return ::json_string(v.ToString());
	case VarType::Array:
		{
			JsonHandle json(::json_array());
//!!!R			VarValue r = JsonVarValueObj::FromJsonT(json);
			for (int i=0; i<v.size(); ++i) {
				CCheck(::json_array_append_new(json, CopyToJsonT(v[i])));
			}
			return json.Detach();
		}
	case VarType::Map:
		{
			JsonHandle json(::json_object());
			vector<String> keys = v.Keys();
			EXT_FOR (const String& key, keys) {
				CCheck(::json_object_set_new(json, key, CopyToJsonT(v[key])));
			}
			return json.Detach();
		}
	default:
		Throw(E_NOTIMPL);
	}
}

#elif UCFG_JSON == UCFG_JSON_JSONCPP

class JsonVarValueObj : public VarValueObj {
public:
	JsonVarValueObj(Encoding& enc, const Json::Value& val)
		:	m_enc(enc)
		,	m_val(val)
	{}

	bool HasKey(RCString key) override {
		return !!m_val[(const char*)key];
	}

	VarType type() const override {
		switch (m_val.type()) {
		case Json::nullValue:		return VarType::Null;
		case Json::intValue:		return VarType::Int;
		case Json::realValue:		return VarType::Float;
		case Json::booleanValue:	return VarType::Bool;
		case Json::stringValue:		return VarType::String;
		case Json::arrayValue:		return VarType::Array;
		case Json::objectValue:		return VarType::Map;
		default:
			Throw(E_NOTIMPL);
		}
	}

	size_t size() const override {
		return m_val.size();
	}

	VarValue operator[](int idx) override {
		const Json::Value& v = m_val[idx];
		VarValue r;
		if (!v.isNull())
			r.m_pimpl = new JsonVarValueObj(m_enc, v);
		return r;
	}

	VarValue operator[](RCString key) override {
		const Json::Value& v = m_val[(const char*)key];
		VarValue r;
		if (!v.isNull())
			r.m_pimpl = new JsonVarValueObj(m_enc, v);
		return r;
	}

	String ToString() const override {
		string s = m_val.asString();
		return m_enc.GetChars(ConstBuf(s.data(), s.length()));
	}

	int64_t ToInt64() const override {
		return m_val.asInt();
	}
private:
	Encoding& m_enc;
	Json::Value m_val;
};


class JsonParser : public MarkupParser {
	VarValue Parse(istream& is, Encoding *enc) {
		Json::Value v;
		try {
			is >> v;
		} catch (const std::exception&) {
			Throw(E_FAIL);
		}
		VarValue r;
		r.m_pimpl = new JsonVarValueObj(*enc, v);
		return r;
	}
};

VarValue AFXAPI ParseJson(RCString s) {
	string ss(s.c_str());
	istringstream is(ss);
	return MarkupParser::CreateJsonParser()->Parse(is);
}

#endif // UCFG_JSON == UCFG_JSON_JSONCPP

ptr<MarkupParser> MarkupParser::CreateJsonParser() {
	return new JsonParser;
}


} // Ext::

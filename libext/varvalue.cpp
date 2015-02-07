#include <el/ext.h>

namespace Ext { 
using namespace std;

class SimpleVarValueObj : public VarValueObj {
	bool HasKey(RCString key) const	override { Throw(E_FAIL); }
	size_t size() const			override { Throw(E_FAIL); }
	VarValue operator[](int idx) const override { Throw(E_FAIL); }
	VarValue operator[](RCString key) const override { Throw(E_FAIL); }
	String ToString() const override { Throw(E_FAIL); }
	int64_t ToInt64() const override { Throw(E_FAIL); }
	bool ToBool() const override { Throw(E_FAIL); }
	double ToDouble() const override { Throw(E_FAIL); }
	void Set(int idx, const VarValue& v) override { Throw(E_FAIL); }
	void Set(RCString key, const VarValue& v) override { Throw(E_FAIL); }	
	std::vector<String> Keys() const override { Throw(E_FAIL); }	
};

class IntVarValueObj : public SimpleVarValueObj {
public:
	int64_t Value;

	IntVarValueObj(int64_t v)
		:	Value(v)
	{}

	VarType type() const override { return VarType::Int; }
	int64_t ToInt64() const override { return Value; }
};

class DoubleVarValueObj : public SimpleVarValueObj {
public:
	double Value;

	DoubleVarValueObj(double v)
		:	Value(v)
	{}

	VarType type() const override { return VarType::Float; }
	double ToDouble() const override { return Value; }
};


class BoolVarValueObj : public SimpleVarValueObj {
public:
	bool Value;

	BoolVarValueObj(bool v)
		:	Value(v)
	{}

	VarType type() const override { return VarType::Bool; }
	bool ToBool() const override { return Value; }
};

class StringVarValueObj : public SimpleVarValueObj {
public:
	String Value;

	StringVarValueObj(RCString v)
		:	Value(v)
	{}

	VarType type() const override { return VarType::String; }
	String ToString() const override { return Value;; }
};

class ArrayVarValueObj : public SimpleVarValueObj {
public:
	vector<VarValue> Array;

	VarType type() const override { return VarType::Array; }
	size_t size() const override { return Array.size(); }

	VarValue operator[](int idx) const override {
		return Array.at(idx);
	}

	void Set(int idx, const VarValue& v) override {
		if (idx < 0)
			Throw(E_INVALIDARG);
		if (idx >= Array.size())
			Array.resize(idx+1);
		Array[idx] = v;
	}
};

class MapVarValueObj : public SimpleVarValueObj {
public:
	map<String, VarValue> Map;

	VarType type() const override { return VarType::Map; }
	size_t size() const override { return Map.size(); }

	VarValue operator[](RCString key) const override {
		map<String, VarValue>::const_iterator it = Map.find(key);
		if (it == Map.end())
			Throw(E_INVALIDARG);
		return it->second;
	}

	void Set(RCString key, const VarValue& v) override {
		Map[key] = v;
	}

	vector<String> Keys() const override {
		vector<String> r;
		r.reserve(Map.size());
		for (map<String, VarValue>::const_iterator it(Map.begin()), e(Map.end()); it!=e; ++it)
			r.push_back(it->first);
		return r;
	}
};


VarValue::VarValue(int64_t v)
	:	m_pimpl(new IntVarValueObj(v))
{
}

VarValue::VarValue(int v)
	:	m_pimpl(new IntVarValueObj(v))
{
}

VarValue::VarValue(double v)
	:	m_pimpl(new DoubleVarValueObj(v))
{
}

VarValue::VarValue(bool v)
	:	m_pimpl(new BoolVarValueObj(v))
{
}

VarValue::VarValue(RCString v)
	:	m_pimpl(new StringVarValueObj(v))
{
}

VarValue::VarValue(const char *p)
	:	m_pimpl(new StringVarValueObj(p))
{
}

VarValue::VarValue(const vector<VarValue>& ar)
	:	m_pimpl(new ArrayVarValueObj)
{
	for (size_t i=0; i<ar.size(); ++i)
		Set(i, ar[i]);
}

bool VarValue::operator==(const VarValue& v) const {
	if (type() != v.type())
		return false;
	switch (type()) {
	case VarType::Null: return true;
	case VarType::Bool: return ToBool() == v.ToBool();
	case VarType::Int: return ToInt64() == v.ToInt64();
	case VarType::Float: return ToDouble() == v.ToDouble();
	case VarType::String: return ToString() == v.ToString();
	case VarType::Array:
		if (size() != v.size())
			return false;
		for (int i=0; i<size(); ++i)
			if (_self[i] != v[i])
				return false;
		return true;
	case VarType::Map:
		{
			if (size() != v.size())
				return false;
			vector<String> keys = Keys();
			for (int i=0; i<keys.size(); ++i) {
				const String& key = keys[i];
				if (!v.HasKey(key) || _self[key] != v[key])
					return false;
			}
			return true;
		}
	default:
		Throw(E_NOTIMPL);
	}
}

void VarValue::Set(int idx, const VarValue& v) {
	if (!m_pimpl)
		m_pimpl = new ArrayVarValueObj;
	m_pimpl->Set(idx, v);
}

void VarValue::Set(RCString key, const VarValue& v) {
	if (!m_pimpl)
		m_pimpl = new MapVarValueObj;
	m_pimpl->Set(key, v);
}

void VarValue::SetType(VarType typ) {
	switch (typ) {
	case VarType::Array:
		m_pimpl = new ArrayVarValueObj;
		break;
	case VarType::Map:
		m_pimpl = new MapVarValueObj;
		break;
	default:
		Throw(E_NOTIMPL);
	}
}

size_t hash_value(const VarValue& v) {
	switch (v.type()) {
	case VarType::Null:		return 0;
	case VarType::Int:		return hash<int64_t>()(v.ToInt64());
	case VarType::Float:	return hash<double>()(v.ToDouble());
	case VarType::Bool:		return hash<bool>()(v.ToBool());
	case VarType::String:	return hash<String>()(v.ToString());
	case VarType::Array:
		{
			size_t r = 0;
			for (int i=0; i<v.size(); ++i)
				r += hash<VarValue>()(v[i]);
			return r;
		}
	case VarType::Map:
		{
			vector<String> keys = v.Keys();
			size_t r = 0;
			for (int i=0; i<keys.size(); ++i)
				r += hash<VarValue>()(v[keys[i]]);
			return r;
		}
	default:
		Throw(E_NOTIMPL);
	}
}


} // Ext::


/*######   Copyright (c) 2019      Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include EXT_HEADER_FILESYSTEM

namespace Ext {
using namespace std;

class Conf : noncopyable {
    class Binder : public NonInterlockedObject {
    public:
    	String Help;

		virtual String ToString() = 0;
		virtual void Parse(RCString s) = 0;
    };

	template <typename T>
	class BinderBase : public Binder {
	public:
		T& m_refVal;

		BinderBase(T& refVal)
			: m_refVal(refVal)
		{
		}

		BinderBase(T& refVal, T def)
			: m_refVal(refVal)
		{
			m_refVal = def;
		}
	};

	class BoolBinder : public BinderBase<bool> {
		typedef BinderBase<bool> base;
	public:
		BoolBinder(bool& refVal, bool def)
			: base(refVal, def)
		{}

		String ToString() override { return Convert::ToString(m_refVal); }

		void Parse(RCString s) override;
	};

	class IntBinder : public BinderBase<int> {
		typedef BinderBase<int> base;
	public:
		IntBinder(int& refVal, int def)
			: base(refVal, def)
		{}

		String ToString() override { return Convert::ToString(m_refVal); }

		void Parse(RCString s) override {
			m_refVal = atoi(s);
		}
	};

	class Int64Binder : public BinderBase<int64_t> {
		typedef BinderBase<int64_t> base;
	public:
		Int64Binder(int64_t& refVal, int64_t def)
			: base(refVal, def)
		{}

		String ToString() override { return Convert::ToString(m_refVal); }

		void Parse(RCString s) override {
			m_refVal = atoll(s);
		}
	};

	class StringBinder : public BinderBase<String> {
		typedef BinderBase<String> base;
	public:
		StringBinder(String& refVal, RCString def)
			: base(refVal, def)
		{}

		String ToString() override { return m_refVal; }

		void Parse(RCString s) override {
			m_refVal = s;
		}
	};

	class StringVectorBinder : public BinderBase<vector<String>> {
		typedef BinderBase<vector<String>> base;
	public:
		StringVectorBinder(vector<String>& refVal)
			: base(refVal)
		{}

		String ToString() override {
			ostringstream os;
			for (size_t i = 0; i < m_refVal.size(); ++i)
				os << (i ? ";" : "") << m_refVal[i];
			return os.str();
		}

		void Parse(RCString s) override {
			m_refVal.push_back(s);
		}
	};

	unordered_map<String, ptr<Binder>> m_nameToBinder;
public:
	virtual ~Conf() {}
	void AssignOption(RCString key, RCString val);
	void Load(istream& istm);
	void SaveSample(ostream& os);
	void Bind(String name, bool& val, bool def = false, String help = nullptr);
	void Bind(String name, int& val, int def = 0, String help = nullptr);
	void Bind(String name, int64_t& val, int64_t def = 0, String help = nullptr);
	void Bind(String name, String& val, RCString def = "", String help = nullptr);
	void Bind(String name, vector<String>& val, String help = nullptr);
};


#define EXT_CONF_OPTION(var, ...) Bind(#var, var, __VA_ARGS__)


} // Ext::

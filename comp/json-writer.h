/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include EXT_HEADER(stack)

namespace Ext {

String JsonEscapeString(RCString s);

ENUM_CLASS (JsonMode) {
	Object,
	Array
} END_ENUM_CLASS(JsonMode);

class JsonTextWriter {
public:
	JsonMode Mode;
	int Indentation;
	char IndentChar;
	bool FirstItem;

	JsonTextWriter(std::ostream& os);

	void Close();
	void WriteIndent();

	void Write(int val);
	void Write(double val);
	void Write(RCString val);
	void Write(bool val);
	void Write(nullptr_t);

	void Write(RCString name, int val);
	void Write(RCString name, double val);
	void Write(RCString name, RCString val);
	void Write(RCString name, bool val);
	void Write(RCString name, nullptr_t);

	template <typename K, typename V>
	void Write(const map<K, V>& m) {
		typedef map<K, V> CMap;
		JsonWriterObject jwo(*this);
		EXT_FOR (const CMap::value_type& kv, m) {
			Write(kv.first, kv.second);
		}
	}

	template <typename K, typename V>
	void Write(const unordered_map<K, V>& m) {
		typedef unordered_map<K, V> CMap;
		JsonWriterObject jwo(*this);
		EXT_FOR (const CMap::value_type& kv, m) {
			Writer(kv.first, kv.second);
		}
	}

	template <typename T>
	void Write(const vector<T>& vec) {
		JsonWriterArray jwa(*this);
		EXT_FOR (const vector<T>::value_type& x, vec) {
			Writer(x);
		}
	}
private:
	std::ostream& m_os;
	CBool m_bOpenedElement,
		  m_bEoled;

	void CommonInit();

	friend class JsonWriterObject;
	friend class JsonWriterArray;
};

class JsonWriterObject {
public:
	JsonTextWriter& Writer;

	JsonWriterObject(JsonTextWriter& writer, RCString name = nullptr);
	~JsonWriterObject();
private:
	JsonMode m_prevMode;
};

class JsonWriterArray {
public:
	JsonTextWriter& Writer;

	JsonWriterArray(JsonTextWriter& writer, RCString name = nullptr);
	~JsonWriterArray();
private:
	JsonMode m_prevMode;
};


} // Ext::


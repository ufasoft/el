/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "json-writer.h"

namespace Ext {

String JsonEscapeString(RCString s) {
	if (s.length() == 0)
		return s;
	vector<String::value_type> v;
	for (int i=0; i<s.length(); ++i) {
		String::value_type ch = s[i];
		switch (ch) {
		case '\\':
			v.push_back('\\');
			v.push_back(ch);
			break;
		case '"':
			v.push_back('\\');
			v.push_back(ch);
			break;
		default:
			if (ch < 20) {
				v.push_back('\\');
				v.push_back('u');
				v.push_back('0');
				v.push_back('0');
				ostringstream os;
				os << "\\u" << setw(4) << setfill('0') << int(ch);
				string str = os.str();
				v.insert(v.end(), str.begin(), str.end());			
			} else
				v.push_back(ch);
		}		
	}
	return String(&v[0], v.size());
}

JsonTextWriter::JsonTextWriter(std::ostream& os)
	:	m_os(os)
	,	Mode(JsonMode::Object)
	,	FirstItem(true)
{
	CommonInit();
}

void JsonTextWriter::CommonInit() {
	IndentChar = '\t';
	Indentation = 1;
}

void JsonTextWriter::WriteIndent() {
	if (!exchange(FirstItem, false))
		m_os << ",\n" << string(Indentation, IndentChar);
}

void JsonTextWriter::Close() {

}

void JsonTextWriter::Write(int val) {
	WriteIndent();
	m_os << val;
}

void JsonTextWriter::Write(double val) {
	WriteIndent();
	m_os << val;
}

void JsonTextWriter::Write(RCString val) {
	WriteIndent();
	m_os << "\"" << JsonEscapeString(val) << "\"";
}

void JsonTextWriter::Write(bool val) {
	WriteIndent();
	m_os << val;
}

void JsonTextWriter::Write(nullptr_t) {
	WriteIndent();
	m_os << "null";
}

void JsonTextWriter::Write(RCString name, int val) {
	WriteIndent();
	m_os << "\"" << JsonEscapeString(name) << "\": " << val;
}

void JsonTextWriter::Write(RCString name, double val) {
	WriteIndent();
	m_os << "\"" << JsonEscapeString(name) << "\": " << val;
}

void JsonTextWriter::Write(RCString name, RCString val) {
	WriteIndent();
	m_os << "\"" << JsonEscapeString(name) << "\": \"" << JsonEscapeString(name) << "\"";
}

void JsonTextWriter::Write(RCString name, bool val) {
	WriteIndent();
	m_os << "\"" << JsonEscapeString(name) << "\": " << val;
}

void JsonTextWriter::Write(RCString name, nullptr_t) {
	m_os << "\"" << JsonEscapeString(name) << "\": null";
}

JsonWriterObject::JsonWriterObject(JsonTextWriter& writer, RCString name)
	:	Writer(writer)
{
	Writer.Indentation++;
	Writer.WriteIndent();
	if (name != nullptr)
		Writer.m_os << "\"" << JsonEscapeString(name) << "\": ";		
	Writer.m_os << "{\n";
	Writer.FirstItem = true;
	m_prevMode = exchange(Writer.Mode, JsonMode::Object);
}

JsonWriterObject::~JsonWriterObject() {
	Writer.Indentation--;
	Writer.m_os << ",\n" << string(Writer.Indentation, Writer.IndentChar) << "}";
	Writer.Mode = m_prevMode;
}

JsonWriterArray::JsonWriterArray(JsonTextWriter& writer, RCString name)
	:	Writer(writer)
{
	Writer.Indentation++;
	Writer.WriteIndent();
	if (name != nullptr)
		Writer.m_os << "\"" << JsonEscapeString(name) << "\": ";		
	Writer.m_os << "[\n";
	Writer.FirstItem = true;
	m_prevMode = exchange(Writer.Mode, JsonMode::Object);
}

JsonWriterArray::~JsonWriterArray() {
	Writer.Indentation--;
	Writer.m_os << ",\n" << string(Writer.Indentation, Writer.IndentChar) << "]";
	Writer.Mode = m_prevMode;
}



} // Ext::


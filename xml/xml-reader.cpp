/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include <el/xml.h>

namespace Ext {

string unquote(const string& input) {
	const char *beg = input.c_str();
	if (const char *str = strchr(beg, '&')) {
		vector<char> vec(beg, str);
		vec.reserve(max(str-beg, ptrdiff_t(100)));

		bool bActive = false;
		bool have_amp = false;

		for (char ch; ch=*str; str++) {
			if (ch == '&') {
				if (!bActive) {
					bActive = true;
					have_amp = true;
				}
				else if (have_amp)
					vec.push_back('&');
			} else if (bActive) {
				if (!strncmp(str, "amp;", 4)) {
					vec.push_back('&');
					str += 3;
				} else if (!strncmp(str, "quot;", 5)) {
					vec.push_back('\"');
					str += 4;
				} else if (!strncmp(str, "lt;", 3)) {
					vec.push_back('<');
					str += 2;
				} else if (!strncmp(str, "gt;", 3)) {
					vec.push_back('>');
					str += 2;
				} else {
					if (have_amp)
						vec.push_back('&');
					vec.push_back(ch);
				}
				bActive = false;
				have_amp = false;
			} else
				vec.push_back(ch);
		}
		if (have_amp)
			vec.push_back('&');
		return string(&vec[0], vec.size()); // vec can't be empty here
	}
	else
		return input;
}


string unquote(const char *str) {
	return unquote(string(str));
}


string quote_normalize(const string& in) {
	return quote(unquote(in));
}


void XmlReader::Init(void *p) {
	if (!p)
		throw XmlException( "can not open from stream");
	m_p = p;
}

XmlNodeType XmlReader::MoveToContent() const {
	do  {
		switch (XmlNodeType nt = NodeType) {
		case XmlNodeType::Text:
		case XmlNodeType::CDATA:
		case XmlNodeType::Element:
		case XmlNodeType::EndElement:
		case XmlNodeType::EntityReference:
		case XmlNodeType::EndEntity:
			return nt;
		}
	} while (Read());
	return XmlNodeType::None;
}

bool XmlReader::IsStartElement() const {
	return MoveToContent() == XmlNodeType::Element;
}

bool XmlReader::IsStartElement(const CStrPtr& name) const {
	return IsStartElement() && Name==name.c_str();
}

void XmlReader::ReadStartElement() const {
	if (!IsStartElement())
		throw XmlException("current node not StartElement");
	Read();
}

void XmlReader::ReadStartElement(const CStrPtr& name) const {
	if (!IsStartElement(name)) {
#ifdef _DEBUG 
		String curname = Name;
#endif
		throw XmlException(String("current node not StartElement ")+name.c_str());
	}
	Read();
}

void XmlReader::ReadEndElement() const {
	if (NodeType != XmlNodeType::EndElement)
		throw XmlException("current node not EndElement");
	 Read();
}

String XmlReader::ReadString() const {
	String r;
	MoveToElement();
	switch (NodeType) {
	case XmlNodeType::Element:
		if (IsEmptyElement())
			return String();
		while (true) {
			Read();
			switch (NodeType) {
			case XmlNodeType::Text:
			case XmlNodeType::CDATA:
			case XmlNodeType::Whitespace:
			case XmlNodeType::SignificantWhitespace:
				r += Value;
				continue;
			}
			break;
		}
		break;
	case XmlNodeType::Text:
	case XmlNodeType::CDATA:
	case XmlNodeType::Whitespace:
	case XmlNodeType::SignificantWhitespace:
		while (true) {
			switch (NodeType) {
			case XmlNodeType::Text:
			case XmlNodeType::CDATA:
			case XmlNodeType::Whitespace:
			case XmlNodeType::SignificantWhitespace:
				r += Value;
				Read();
				continue;
			}
			break;
		}
		break;
	default:
		return String();
	}
	return r;
}

bool XmlReader::ReadToDescendant(RCString name) const {
	if (Ext::ReadState::Initial == get_ReadState()) {
		MoveToContent();
		if (IsStartElement(name))
			return true;
	}
	if (NodeType == XmlNodeType::Element && !IsEmptyElement())
		for (int depth = Depth; Read() && depth < Depth;)
			if (Name == name)
				return true;
	return false;
}

bool XmlReader::get_Eof() const {
	return Ext::ReadState::EndOfFile == get_ReadState();
}

bool XmlReader::ReadToNextSibling(RCString name) const {
/*!!!R	if (Ext::ReadState::Interactive != ReadState)
		return false;*/
	bool b = MoveToElement();
	for (int depth=Depth;;) {
		Skip();
		if (Depth < depth)
			return false;
		if (NodeType == XmlNodeType::Element && Name == name)
			return true;
	}
}

bool XmlReader::ReadToFollowing(RCString name) const {
	while (Read())
		if (NodeType==XmlNodeType::Element && Name==name)
			return true;
	return false;	
}

String XmlReader::ReadElementString() const {
	String r;
	bool bIsEmpty = IsEmptyElement();
	ReadStartElement();
	if (!bIsEmpty) {
		r = ReadString();
		ReadEndElement();
	}
	return r;
}

String XmlReader::ReadElementString(RCString name) const {
	if (!IsStartElement(name))
		throw XmlException(String("current node not StartElement ")+name.c_str());
	return ReadElementString();
}



} // Ext::



#include <el/ext.h>

#include <el/libext/ext-net.h>

namespace Ext {
using namespace std;

uint16_t AFXAPI CalculateWordSum(const ConstBuf& mb, uint32_t sum, bool bComplement) {
	uint16_t *p = (uint16_t*)mb.P;
	for (size_t count=mb.Size>>1; count--;)
		sum += *p++;
	if (mb.Size & 1)
		sum += *(byte*)p;
	for (uint32_t w; w=sum>>16;)
		sum = (sum & 0xFFFF)+w;
	return uint16_t(bComplement ? ~sum : sum);
}

String CHttpHeader::ParseHeader(const vector<String>& ar, bool bIncludeFirstLine, bool bEmailHeader) {
	Headers.clear();
	Data.Size = 0;
	m_bDataSkipped = false;
	int i=0;
	if (!bIncludeFirstLine) {
		if (ar.empty())
			Throw(E_FAIL);
		i = 1;
	}
	String prev;
	for (; i<ar.size(); i++) {
		String line = ar[i];
		if (bEmailHeader && line.length() > 0 && isspace(line.at(0))) {
			if (prev.empty())
				Throw(E_FAIL);
			Headers[prev].back() += " "+line.TrimLeft();
		} else {
			vector<String> vec = line.Split(":", 2);
			if (vec.size() != 2)
				Throw(E_FAIL);
			Headers[prev = vec[0].Trim().ToUpper()].push_back(vec[1].Trim());
		}
	}
	return bIncludeFirstLine ? "" : ar[0];
}

String CHttpHeader::get_Content() {
	Ext::Encoding *enc = GetEncoding();
	return enc->GetChars(Data);
}

ostream& AFXAPI operator<<(ostream& os, const CHttpHeader& header) {
	header.PrintFirstLine(os);
	for (NameValueCollection::const_iterator i=header.Headers.begin(); i!=header.Headers.end(); ++i)
		os << i->first << ": " << i->second << "\r\n";
	return os << "\r\n";
}

void CHttpRequest::Parse(const vector<String>& ar) {
	String s = ParseHeader(ar);
	vector<String> sar = s.Split();
	if (sar.size()<2)
		Throw(E_FAIL);
	Method = sar[0];
	RequestUri = sar[1];
}

void CHttpResponse::Parse(const vector<String>& ar) {
	String s = ParseHeader(ar);
	vector<String> sar = s.Split();
	if (sar.size()<2 || sar[0].Left(5) != "HTTP/")
		Throw(E_FAIL);
	Code = atoi(sar[1]);
}

void CHttpRequest::ParseParams(RCString s) {
	vector<String> pars = s.substr(1).Split("&");
	for (int i=0; i<pars.size(); ++i) {
		String par = pars[i];
		vector<String> pp = par.Split("=");
		if (pp.size() == 2) {
			String key = pp[0],
				 pp1 = pp[1];
			pp1.Replace("+", " ");
			m_params[key.ToUpper()].push_back(Uri::UnescapeDataString(pp1));
		}
	}
}

NameValueCollection& CHttpRequest::get_Params() {
	if (!m_bParams) {
		m_bParams = true;

		String query = Uri("http://host"+RequestUri).Query;
		if (!!query) {
			if (query.length() > 1 && query.at(0)=='?')
				ParseParams(query);
			else if (Method == "POST")
				ParseParams(Encoding::UTF8.GetChars(Data));			
		}
	}
	return m_params;
}





} // Ext::


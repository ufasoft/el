#include <el/ext.h>


#if UCFG_USE_PCRE==2
#	define PCRE2_STATIC 1
#	define PCRE2_CODE_UNIT_WIDTH 8
#	include <pcre2.h>
#else
#	define PCRE_STATIC 1
#	include <pcre.h>
#endif

#if UCFG_STDSTL && defined(_MSC_VER)
#	include "extstl.h"
#endif

using namespace std;
#include "regex"
using ExtSTL::regex_constants::syntax_option_type;

ASSERT_CDECL

namespace ExtSTL  {
using namespace Ext;

static class PCRECategory : public ErrorCategoryBase {			// outside function to eliminate thread-safe static machinery
	typedef ErrorCategoryBase base;
public:
	PCRECategory()
		:	base(
#if UCFG_USE_PCRE==2
		"PCRE2"
#else
		"PCRE"
#endif
	, FACILITY_PCRE)
	{}

	string message(int eval) const override {
#if UCFG_USE_PCRE==2
		char buf[255];
		if (pcre2_get_error_message(eval, (uint8_t *)buf, sizeof buf) < 0)
			Throw(errc::no_buffer_space);
		return buf;
#else
		return EXT_STR("Code " << -eval);
#endif
	}
} s_pcreCategory;

const error_category& pcre_category() {
	return s_pcreCategory;
}

class RegexException : public Exception {
	typedef Exception base;
public:
	int Offset;

	RegexException(const error_code& ec, int offset = 0)
		:	base(ec)
		,	Offset(offset)
	{
	}
protected:
	String get_Message() const override {
		return EXT_STR(base::get_Message() << " at offset " << Offset);
	}
};


int PcreCheck(int r, int offset = 0) {
	if (r < 0)
		throw RegexException(error_code(-r, pcre_category()), offset);
#if UCFG_USE_PCRE==2
	if (r > 0) {
		throw RegexException(error_code(r, pcre_category()), offset);
	}
#endif
	return r;
}

#if UCFG_USE_PCRE==2

class Pcre2MatchData : noncopyable {
public:
	pcre2_match_data * const m_md;

	Pcre2MatchData(pcre2_code *code)
		: m_md(pcre2_match_data_create_from_pattern(code, 0))
	{
		if (!m_md)
			PcreCheck(-1);
	}

	~Pcre2MatchData() {
		pcre2_match_data_free_8(m_md);
	}

	operator pcre2_match_data*() { return m_md; }
};

#endif // UCFG_USE_PCRE==2


class StdRegexObj : public Object {
public:
	String m_pattern;
	int m_pcreOpts;
#if UCFG_USE_PCRE==2
	pcre2_code *m_re;
	pcre2_code *FullRe();
private:
	pcre2_code *m_reFull;
#else
	pcre *m_re;
	pcre *FullRe();
private:
	pcre *m_reFull;
#endif
public:
	StdRegexObj(RCString pattern, int options, bool bBinary);
	~StdRegexObj();
};


const int MAX_MATCHES = 20; //!!!

StdRegexObj::StdRegexObj(RCString pattern, int options, bool bBinary)
	: m_pattern(pattern)
	, m_reFull(0)
{
#if UCFG_USE_PCRE==2
	m_pcreOpts = 0;
	if (!bBinary)
		m_pcreOpts |= PCRE2_UTF;
	if (options & regex_constants::icase)
		m_pcreOpts |= PCRE2_CASELESS;
	if (options & regex_constants::nosubs)		//!!!?
		m_pcreOpts |= PCRE2_NO_AUTO_CAPTURE;
#else
	m_pcreOpts = PCRE_JAVASCRIPT_COMPAT;
	if (!bBinary)
		m_pcreOpts |= PCRE_UTF8;
	if (options & regex_constants::icase)
		m_pcreOpts |= PCRE_CASELESS;
	if (options & regex_constants::nosubs)		//!!!?
		m_pcreOpts |= PCRE_NO_AUTO_CAPTURE;
#endif
	Blob utf8 = Encoding::UTF8.GetBytes(pattern);
	unsigned char *table = 0;
	int errcode;
#if UCFG_USE_PCRE==2
	size_t error_offset;
	if (!(m_re = pcre2_compile((PCRE2_SPTR8)(const char*)utf8.constData(), PCRE2_ZERO_TERMINATED, m_pcreOpts, &errcode, &error_offset, nullptr)))
		PcreCheck(errcode, error_offset);
#else
	const char *error;
	int error_offset;
	if (!(m_re = pcre_compile2((const char*)utf8.constData(), m_pcreOpts, &errcode, &error, &error_offset, table)))
		throw RegexExc(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_PCRE, errcode), EXT_STR(error << " at offset " << error_offset));
#endif
}

StdRegexObj::~StdRegexObj() {
#if UCFG_USE_PCRE==2
	pcre2_code_free(m_reFull);
	pcre2_code_free(m_re);
#else
	if (m_reFull)
		pcre_free(m_reFull);
	pcre_free(m_re);
#endif
}

#if UCFG_USE_PCRE==2
pcre2_code *StdRegexObj::FullRe() {
#else
pcre *StdRegexObj::FullRe() {
#endif
	if (!m_reFull) {
		Blob utf8 = Encoding::UTF8.GetBytes("(?:" + m_pattern + ")\\z");
		unsigned char *table = 0;
		int errcode;
#if UCFG_USE_PCRE==2
		size_t error_offset;
		if (!(m_reFull = pcre2_compile((PCRE2_SPTR8)(const char*)utf8.constData(), PCRE2_ZERO_TERMINATED, m_pcreOpts | PCRE2_ANCHORED, &errcode, &error_offset, nullptr)))
			PcreCheck(errcode);
#else
		const char *error;
		int error_offset;
		if (!(m_reFull = pcre_compile2((const char*)utf8.constData(), m_pcreOpts | PCRE_ANCHORED, &errcode, &error, &error_offset, table)))
			throw RegexExc(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_PCRE, errcode), EXT_STR(error << " at offset " << error_offset));
#endif
	}
	return m_reFull;
}

basic_regexBase::~basic_regexBase() {
}

void *basic_regexBase::Re() const {
	return m_pimpl->m_re;
}

void *basic_regexBase::FullRe() const {
	return m_pimpl->FullRe();
}

basic_regex<char>::basic_regex(RCString pattern, flag_type flags) {
	m_pimpl = new StdRegexObj(pattern, flags, true);
}

basic_regex<wchar_t>::basic_regex(RCString pattern, flag_type flags) {
	m_pimpl = new StdRegexObj(pattern, flags, false);
}

bool AFXAPI regex_searchImpl(const char *b, const char *e, match_results<const char*> *m, const basic_regexBase& re, bool bMatch, regex_constants::match_flag_type flags, const char *org) {
	void *pre = bMatch ? re.FullRe() : re.Re();

	int options = 0;

#if UCFG_USE_PCRE==2
	if (flags & regex_constants::match_not_bol)
		options |= PCRE2_NOTBOL;
	if (flags & regex_constants::match_not_eol)
		options |= PCRE2_NOTEOL;
	Pcre2MatchData md((pcre2_code*)pre);
	int rc = ::pcre2_match((pcre2_code*)pre, (PCRE2_SPTR8)b, int(e-b), 0, options, md, nullptr);
#else
	int ovector[MAX_MATCHES*3];
	if (flags & regex_constants::match_not_bol)
		options |= PCRE_NOTBOL;
	if (flags & regex_constants::match_not_eol)
		options |= PCRE_NOTEOL;
	int rc = ::pcre_exec((pcre*)pre, NULL, b, int(e-b), 0, options, ovector, MAX_MATCHES*3);
#endif

	if (rc < -1)
		PcreCheck(rc);
	if (rc < 1)
		return false;
#if UCFG_USE_PCRE==2
	size_t *ovector = pcre2_get_ovector_pointer(md.m_md);
	size_t count = pcre2_get_ovector_count(md.m_md);
#else
	int count;
	PcreCheck(::pcre_fullinfo((pcre*)pre, 0, PCRE_INFO_CAPTURECOUNT, &count));
#endif
	if (m) {
		m->m_ready = true;
		m->m_org = b;
		m->Resize(count+1);
		for (int i=0; i<rc; i++) {
			sub_match<const char*>& sm = m->GetSubMatch(i);
			sm.first = b + ovector[2*i];
			sm.second = b + ovector[2*i+1];
#if UCFG_USE_PCRE==2
			sm.matched = ovector[2 * i] != PCRE2_UNSET;
#else
			sm.matched = ovector[2*i] >= 0;
#endif
		}
		m->m_prefix.first = b;
		m->m_prefix.second = (*m)[0].first;
		m->m_prefix.matched = true;

		m->m_suffix.first = (*m)[0].second;
		m->m_suffix.second = e;
		m->m_suffix.matched = true;
	}
	return true;
}

bool AFXAPI regex_searchImpl(const wchar_t *bs, const wchar_t *es, match_results<const wchar_t*> *m, const basic_regexBase& re, bool bMatch, regex_constants::match_flag_type flags, const wchar_t *org) {
	Blob utf8 = Encoding::UTF8.GetBytes(String(wstring(bs, es-bs)));
	const char *b = (const char *)utf8.constData(),
	           *e = (const char *)utf8.constData() + utf8.size();
	bool r;
	if (m) {
		match_results<const char*> m8;
		if (r = regex_searchImpl(b, e, &m8, re, bMatch, flags, nullptr)) {
			m->m_ready = true;
			m->m_org = org;
			m->Resize(m8.size());
			for (size_t i=0; i<m8.size(); ++i) {
				const sub_match<const char*>& sm = m8[i];
				match_results<const wchar_t*>::value_type& dm = m->GetSubMatch(i);
				if (dm.matched = sm.matched) {
					dm.first = bs + Encoding::UTF8.GetCharCount(Span((const uint8_t*)b, (const uint8_t*)sm.first));
					dm.second = bs + Encoding::UTF8.GetCharCount(Span((const uint8_t*)b, (const uint8_t*)sm.second));
				}
			}
			m->m_prefix.first = bs;
			m->m_prefix.second = (*m)[0].first;
			m->m_prefix.matched = true;

			m->m_suffix.first = (*m)[0].second;
			m->m_suffix.second = es;
			m->m_suffix.matched = true;
		}
	} else
		r = regex_searchImpl(b, e, 0, re, bMatch, flags, nullptr);
	return r;
}

bool AFXAPI regex_searchImpl(string::const_iterator bi,  string::const_iterator ei, smatch *m, const basic_regexBase& re, bool bMatch, regex_constants::match_flag_type flags, string::const_iterator orgi) {
	const char *b = (const char *)&*bi,
	           *e = (const char *)&*ei;
	bool r;
	if (m) {
		match_results<const char*> m8;
		if (r = regex_searchImpl(b, e, &m8, re, bMatch, flags, nullptr)) {
			m->m_ready = true;
			m->m_org = orgi;
			m->Resize(m8.size());
			for (size_t i=0; i<m8.size(); ++i) {
				const sub_match<const char*>& sm = m8[i];
				smatch::value_type& dm = m->GetSubMatch(i);
				if (dm.matched = sm.matched) {
					dm.first = bi+(sm.first-b);
					dm.second = bi+(sm.second-b);
				}
			}
			m->m_prefix.first = bi;
			m->m_prefix.second = (*m)[0].first;
			m->m_prefix.matched = true;

			m->m_suffix.first = (*m)[0].second;
			m->m_suffix.second = ei;
			m->m_suffix.matched = true;
		}
	} else
		r = regex_searchImpl(b, e, 0, re, bMatch, flags, nullptr);
	return r;
}

struct ExtSTL_RegexTraits {
	EXT_DATA static const syntax_option_type DefaultB;
};

typedef DelayedStatic2<regex, ExtSTL_RegexTraits, String, syntax_option_type> ExtSTL_StaticRegex;
static ExtSTL_StaticRegex s_reRepl("\\${(\\w+)}");

const syntax_option_type ExtSTL_RegexTraits::DefaultB = regex_constants::ECMAScript;

struct IdxLenName {					// can't be local in old GCC
	sub_match<const char *> sm;
	string Name;
};

string AFXAPI regex_replace(const string& s, const regex& re, const string& fmt, regex_constants::match_flag_type flags) {
	const char *bs = s.c_str(), *es = bs+s.size(),
		*bfmt = fmt.c_str(), *efmt = bfmt+fmt.size();

	vector<IdxLenName> vRepl;
	for (cregex_iterator it(bfmt, efmt, s_reRepl), e; it!=e; ++it) {
		IdxLenName iln;
		iln.sm = (*it)[0];
		iln.Name = (*it)[1];
		vRepl.push_back(iln);
	}

	int last = 0;
	string r;
	const char *prev = bs;
	for (cregex_iterator it(bs, es, re, flags), e; it!=e; ++it) {
		r += string(prev, (*it)[0].first);
		const char *prevRepl = bfmt;
		for (size_t j=0; j<vRepl.size(); ++j) {
			r += string(prevRepl, vRepl[j].sm.first);
			String name = vRepl[j].Name;
			int idx = atoi(name);
			if (!idx && name!="0")
				Throw(E_FAIL);
			r += (*it)[idx];
			prevRepl = vRepl[j].sm.second;
		}
		r += string(prevRepl, efmt);
		prev = (*it)[0].second;
	}
	r += string(prev, es);
	return r;
}

}  // ExtSTL::



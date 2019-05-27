#pragma once


//!!!R#if !UCFG_STDSTL || UCFG_WCE

#ifdef _MSC_VER
#	include <xlocinfo.h>
#else
#	define const _XA = 0;
#endif


//namespace _STL_NAMESPACE {
namespace ExtSTL {

class LocaleObjBase : public Ext::Object {
public:
	typedef Ext::InterlockedPolicy interlocked_policy;

};

class locale {
public:
	enum category {
		none = 0,
		collate = _M_COLLATE,
		ctype = _M_CTYPE,
		monetary = _M_MONETARY,
		numeric = _M_NUMERIC,
		time = _M_TIME,
		messages = _M_MESSAGES,
		all = _M_ALL
	};

	class id : Ext::noncopyable {
		typedef id class_type;
	public:
		id(size_t val = 0);
		operator size_t();
	private:
		atomic<int> m_aVal;
	};

	class facet : public Ext::Object {
	public:
		virtual ~facet() {}

		static size_t GetCat(const locale::facet **pp = 0, const locale *loc = 0) {
			return size_t(-1);
		}
	protected:
		explicit facet(size_t refs = 0) {
			m_aRef = refs;
		}
	};

#if UCFG_FRAMEWORK
	EXT_DATA static mutex s_cs;
#endif

	locale();
	explicit locale(const char *locname);

	template <class F>
	locale(const locale& loc, const F *fac) {
		Init(loc, fac, F::GetCat(), F::id);
	}

	static const locale& classic() {
		return s_classic;
	}

	const facet *GetFacet(size_t id) const;
private:
	static atomic<int> s_id;
	static locale s_classic;

	Ext::ptr<LocaleObjBase> m_pimpl;

	static Ext::ptr<LocaleObjBase> Init();
	void Init(const locale& loc, const facet *fac, int category, size_t id);
};

template <class F>
bool has_facet(const locale& _Loc) {
	EXT_LOCK (locale::s_cs) {
		return F::id || F::GetCat() != size_t(-1);
	}
}

template <class F>
struct FacetPtr {
	static const locale::facet *P;
};

template <class F>
const locale::facet *FacetPtr<F>::P = 0;

#if UCFG_FRAMEWORK

template <class F>
const F& use_facet(const locale& loc) {
	EXT_LOCK (locale::s_cs) {
		const locale::facet *save = FacetPtr<F>::P;
		const locale::facet *pf = loc.GetFacet(F::id);
		if (!pf && !(pf = save)) {
			if (F::GetCat(&pf, &loc) == size_t(-1))
				Ext::ThrowImp(Ext::ExtErr::InvalidCast);
			FacetPtr<F>::P = pf;
		}
		return static_cast<const F&>(*pf);
	}
}

#endif // UCFG_FRAMEWORK

template <typename EL>
EL tolower(EL ch, const locale& loc) {
	return use_facet<ctype<EL>>(loc).tolower(ch);
}

template <typename EL>
EL toupper(EL ch, const locale& loc) {
	return use_facet<ctype<EL>>(loc).toupper(ch);
}

EXT_API bool AFXAPI islower(wchar_t ch, const locale& loc = locale(0));
EXT_API bool AFXAPI isupper(wchar_t ch, const locale& loc = locale(0));
EXT_API bool AFXAPI isalpha(wchar_t ch, const locale& loc = locale(0));
EXT_API wchar_t AFXAPI toupper(wchar_t ch, const locale& loc = locale(0));
EXT_API wchar_t AFXAPI tolower(wchar_t ch, const locale& loc = locale(0));

struct ctype_base : public locale::facet {
	typedef short mask;

	enum {
		lower = _LOWER,
		upper = _UPPER,
		alpha = lower | upper | _XA,
		digit = _DIGIT,
		xdigit = _HEX,
		alnum = alpha | digit,
		punct = _PUNCT,
		graph = alnum | punct,
		print = graph | xdigit,
		cntrl = _CONTROL,
		space = _SPACE | _BLANK
	};
protected:
	_Cvtvec m_cvt;

	ctype_base() {
		Ext::ZeroStruct(m_cvt);		//!!!?
	}
};


template <typename EL>
inline EL _Maklocchr(char ch, EL*, const _Cvtvec&) {
	return (EL)(unsigned char)ch;
}


template <typename EL>
class ctypeBase : public ctype_base {
public:
	typedef EL char_type;

	static locale::id id;

	bool is(mask m, char_type ch) const {
		return do_is(m, ch);
	}

	char narrow(char_type ch, char dflt = '\0') const {
		Ext::ThrowImp(E_NOTIMPL);
	}

	char_type tolower(char_type ch) const { return do_tolower(ch); }
	char_type toupper(char_type ch) const { return do_toupper(ch); }
	char_type widen(char ch) const { return do_widen(ch); }
	const char *widen(const char *b, const char *e, EL *dst) const { return do_widen(b, e, dst); }
protected:
	virtual bool do_is(mask m, char_type ch) const {
		Ext::ThrowImp(E_NOTIMPL);
	}

	virtual char_type do_tolower(char_type ch) const {
		Ext::ThrowImp(E_NOTIMPL);
	}

	virtual char_type do_toupper(char_type ch) const {
		Ext::ThrowImp(E_NOTIMPL);
	}

	virtual char_type do_widen(char ch) const {
		return _Maklocchr(ch, (EL*)0, m_cvt);
	}

	virtual const char *do_widen(const char *b, const char *e, EL *dst) const {
		for (; b!=e; ++b, ++dst)
			*dst = _Maklocchr(*b, (EL*)0, m_cvt);
		return b;
	}
};

template <typename EL>
class ctype : public ctypeBase<EL> {
};

template <>
class ctype<char> : public ctypeBase<char> {
public:
	static size_t GetCat(const locale::facet **pp = 0, const locale *loc = 0) {
		if (pp && !*pp)
			*pp = new ctype;
		return LC_CTYPE;
	}
protected:
	bool do_is(mask m, char_type ch) const override {
		return _isctype(ch, m);
	}

	char_type do_tolower(char_type ch) const override {
		return (char_type)::tolower(ch);
	}

	char_type do_widen(char ch) const override {
		return ch;
	}

	const char *do_widen(const char *b, const char *e, char *dst) const override {
		memcpy(dst, b, e-b);
		return e;
	}
};

template <>
class ctype<wchar_t> : public ctypeBase<wchar_t> {
public:
	static size_t GetCat(const locale::facet **pp = 0, const locale *loc = 0) {
		if (pp && !*pp)
			*pp = new ctype;
		return LC_CTYPE;
	}
protected:
	bool do_is(mask m, char_type ch) const override {
		return iswctype(ch, m);
	}

	char_type do_tolower(char_type ch) const override {
		return ::towlower(ch);
	}

	char_type do_widen(char ch) const override {
		return ch;				//!!!TODO
	}

	const char *do_widen(const char *b, const char *e, wchar_t *dst) const override {
		for (; b!=e; ++b, ++dst)
			*dst = *b;				//!!!TODO
		return b;
	}
};


template <class EL>
locale::id ctype<EL>::id;

template <class EL>
class collate : public locale::facet {
public:
	typedef EL char_type;

	static locale::id id;
};





} // ExtSTL::


//#endif // UCFG_STDSTL || UCFG_WCE




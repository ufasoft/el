#pragma once


namespace Ext {

template <typename T>
class explicit_cast {
public:
	explicit_cast(T t)
		:	m_t(t)
	{}

	operator T() const { return m_t; }
private:
	T m_t;
};


class Encoding;

class EXTCLASS String {
public:
	typedef String class_type;

//!!!R	typedef  Char;
	typedef size_t size_type;
	typedef CBlobBufBase::Char value_type;
	typedef std::char_traits<value_type> traits_type;
	typedef ssize_t difference_type;
	typedef const value_type& const_reference;

	static const size_t npos = size_t(-1);

	class const_iterator : public std::iterator<std::random_access_iterator_tag, value_type>, totally_ordered<const_iterator>  {
	public:
		typedef String::value_type value_type;
		typedef std::random_access_iterator_tag iterator_category;
		typedef String::difference_type difference_type;
		typedef const value_type *pointer;
		typedef const value_type& reference;

		const_iterator()
			:	m_p(0)
		{}

		reference operator*() const { return *m_p; }

		const_iterator& operator++() {
			++m_p;
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator r(*this);
			operator++();
			return r;
		}

		bool operator==(const_iterator it) const { return m_p == it.m_p; }
		bool operator<(const_iterator it) const { return m_p < it.m_p; }

		const_iterator operator+(difference_type diff) const { return const_iterator(m_p + diff); }
		const_iterator operator-(difference_type diff) const { return const_iterator(m_p - diff); }
		difference_type operator-(const_iterator it) const { return m_p - it.m_p; }
	private:
		const value_type *m_p;

		explicit const_iterator(const value_type* p)
			:	m_p(p)
		{}

		friend class String;
	};


	String() {}

	String(RCString s)
		:	m_blob(s.m_blob)
	{
	}

	String(const char *lpch, ssize_t nLength);
#if defined(_NATIVE_WCHAR_T_DEFINED) && UCFG_STRING_CHAR/8 == __SIZEOF_WCHAR_T__
	String(const uint16_t *lpch, ssize_t nLength);
	String(const uint16_t *lpsz);
//	void Init(const uint16_t *lpch, ssize_t nLength);
	void Init(const unsigned short *lpch, ssize_t nLength);
#endif

	String(const char *lpch, int start, ssize_t nLength, Encoding *enc)
		:	m_blob(nullptr)
	{
		Init(enc, lpch+start, nLength);
	}

	String(const value_type *lpch, ssize_t nLength)
		:	m_blob(nullptr)
	{
		Init(lpch, nLength);
	}

	String(const_iterator b, const_iterator e)
		:	m_blob(nullptr)
	{
		Init(b.m_p, e-b);
	}

	explicit String(char ch, ssize_t nRepeat = 1);
	explicit String(value_type ch, ssize_t nRepeat = 1);
	String(const char *lpsz);
	String(const value_type *lpsz);
#if UCFG_STRING_CHAR/8 != __SIZEOF_WCHAR_T__
	String(const wchar_t *lpsz);
#endif
	EXT_API String(const std::string& s);
	EXT_API String(const std::wstring& s);

	EXT_API String(const std::vector<value_type>& vec);

	String (std::nullptr_t p)
		:	m_blob(p)
	{
	}


#if UCFG_COM
	String(const _bstr_t& bstr);

	BSTR get_Bstr() const { return m_blob.m_pData ? m_blob.m_pData->GetBSTR() : 0; }
	DEFPROP_GET_CONST(BSTR, Bstr);

//!!!	operator _bstr_t() const { return _bstr_t(Bstr, true); }
#endif

#if UCFG_USE_REGEX
	template <typename I>
	String(const std::sub_match<I>& sb)
		:	m_blob(nullptr)
	{
		operator=(sb.str());
	}
#endif

	void swap(String& x) noexcept {
		m_blob.swap(x.m_blob);
	}

	//!!!	String(const string& s);

	bool operator!() const { return !m_blob.m_pData; }

	const char *c_str() const;
	operator const char *() const { return c_str(); }
	operator const value_type *() const { return m_blob.m_pData ? (const value_type*)m_blob.m_pData->GetBSTR() : 0; }

	const_iterator begin() const { return const_iterator(m_blob.m_pData ? (const value_type*)m_blob.m_pData->GetBSTR() : 0); }
	const_iterator end() const { return const_iterator(m_blob.m_pData ? (const value_type*)m_blob.m_pData->GetBSTR() + length() : 0); }

	const_reference front() const { return *begin(); }
	const_reference back() const { return *(end()-1); }

	EXT_API operator explicit_cast<std::string>() const;

	operator explicit_cast<std::wstring>() const {
		const value_type *p = *this,
			       *e = p + length();
		return std::wstring(p, e);
	}

#ifdef WDM_DRIVER
	String(UNICODE_STRING *pus);
	operator UNICODE_STRING*() const;
	inline operator UNICODE_STRING() const {
		if (m_blob.m_pData)
			return *(operator UNICODE_STRING*());
		UNICODE_STRING us = { 0 };
		return us;
	}
#endif

	value_type operator[](int nIndex) const;
	value_type operator[](size_t nIndex) const { return operator[]((int)nIndex); }
	//!!!D  String ToOem() const;
	value_type GetAt(size_t idx) const { return (*this)[idx]; }
	void SetAt(size_t nIndex, char c);
	void SetAt(size_t nIndex, unsigned char c) { SetAt(nIndex, (char)c); }
	void SetAt(size_t nIndex, value_type ch);

	String& operator=(const String& stringSrc) {
		m_blob = stringSrc.m_blob;
		return *this;
	}

	String& operator=(std::nullptr_t p) {
		return operator=((const value_type*)0);
	}

	String& operator=(const char * lpsz);
	String& operator=(const value_type * lpsz);

	String& operator=(EXT_RV_REF(String) rv) {
		swap(rv);
		return *this;
	}

	String& operator+=(const String& s);
	void CopyTo(char *ar, size_t size) const;
	void CopyTo(value_type *ar, size_t size) const;
	
	int compare(size_type p1, size_type c1, const value_type *s, size_type c2) const;
	int compare(const String& s) const noexcept;
	
	int CompareNoCase(const String& s) const;
	bool empty() const noexcept;
	void clear();
	size_type find(value_type chm, size_type pos = 0) const noexcept;
	size_type find(const value_type *s, size_type pos, size_type count) const;

	size_type find(const String& s, size_type pos = 0) const { return find((const value_type*)s, pos, s.size()); }

	int LastIndexOf(value_type c) const;

	bool Contains(const String& s) const noexcept { return find(s) != npos; }

	int FindOneOf(const String& sCharSet) const;
	String substr(size_type pos, size_type count = npos) const;
	String Right(ssize_t nCount) const;
	String Left(ssize_t nCount) const;
	String TrimStart(RCString trimChars = nullptr) const;
	String TrimEnd(RCString trimChars = nullptr) const;
	String& TrimLeft() { return (*this) = TrimStart(); }
	String& TrimRight() { return (*this) = TrimEnd(); }
	String Trim() const { return TrimStart().TrimEnd(); }
	EXT_API std::vector<String> Split(RCString separator = "", size_t count = INT_MAX) const;
	EXT_API static String AFXAPI Join(RCString separator, const std::vector<String>& value);
	void MakeUpper();
	void MakeLower();

	String ToUpper() const {
		String r(*this);
		r.MakeUpper();
		return r;
	}

	String ToLower() const {
		String r(*this);
		r.MakeLower();
		return r;
	}

	void Replace(int offset, int size, const String& s);
	int Replace(RCString sOld, RCString sNew);

	size_t length() const noexcept { return m_blob.Size / sizeof(value_type); }

	size_t size() const noexcept { return length(); }

	size_t max_size() const noexcept { return m_blob.max_size() / sizeof(value_type); }

#if UCFG_USE_POSIX
	std::string ToOsString() const {
		return operator explicit_cast<std::string>();
	}
#elif UCFG_STDSTL 
	std::wstring ToOsString() const {
		return operator explicit_cast<std::wstring>();
	}
#else
	String ToOsString() const {
		return *this;
	}
#endif

#if UCFG_COM
	static String AFXAPI FromGlobalAtom(ATOM a);
	BSTR AllocSysString() const;
	LPOLESTR AllocOleString() const;
	void Load(UINT nID);
	bool Load(UINT nID, WORD wLanguage);
//!!!R	bool LoadString(UINT nID); //!!!comp
#endif

private:
	Blob m_blob;

	void Init(Encoding *enc, const char *lpch, ssize_t nLength);
	void Init(const value_type *lpch, ssize_t nLength);
	void MakeDirty() noexcept;

	friend AFX_API String AFXAPI operator+(const String& string1, const String& string2);
	friend AFX_API String AFXAPI operator+(const String& string, const char * lpsz);
	friend AFX_API String AFXAPI operator+(const String& string, const value_type * lpsz);
	//!!!friend AFX_API String AFXAPI operator+(const String& string, TCHAR ch);
	friend inline bool operator<(const String& s1, const String& s2) noexcept;
	friend inline bool AFXAPI operator==(const String& s1, const String& s2) noexcept;
};

inline void swap(String& x, String& y) noexcept {
	x.swap(y);
}


#if UCFG_STLSOFT
} namespace stlsoft {
	inline const char *c_str_ptr(Ext::RCString s) { return s.c_str(); }
} namespace Ext {
#endif

#if UCFG_COM
	inline BSTR Bstr(RCString s) { return s.Bstr; }		// Shim
#endif

typedef std::vector<String> CStringVector;

inline bool operator<(const String& s1, const String& s2) noexcept { return s1.compare(s2) < 0; }

inline bool AFXAPI operator==(const String& s1, const String& s2) noexcept { return s1.m_blob == s2.m_blob; }
AFX_API bool AFXAPI operator!=(const String& s1, const String& s2) noexcept;
AFX_API bool AFXAPI operator<=(const String& s1, const String& s2) noexcept;
AFX_API bool AFXAPI operator>=(const String& s1, const String& s2) noexcept;

AFX_API bool AFXAPI operator==(const String& s1, const char * s2);
AFX_API bool AFXAPI operator==(const char * s1, const String& s2);
AFX_API bool AFXAPI operator!=(const String& s1, const char * s2);
AFX_API bool AFXAPI operator!=(const char * s1, const String& s2);
AFX_API bool AFXAPI operator<(const char * s1, const String& s2);
AFX_API bool AFXAPI operator<(const String& s1, const char * s2);
AFX_API bool AFXAPI operator>(const String& s1, const char * s2);
AFX_API bool AFXAPI operator>(const char * s1, const String& s2);
AFX_API bool AFXAPI operator<=(const String& s1, const char * s2);
AFX_API bool AFXAPI operator<=(const char * s1, const String& s2);
AFX_API bool AFXAPI operator>=(const String& s1, const char * s2);
AFX_API bool AFXAPI operator>=(const char * s1, const String& s2);

AFX_API bool AFXAPI operator==(const String& s1, const String::value_type * s2);
AFX_API bool AFXAPI operator==(const String::value_type * s1, const String& s2);
AFX_API bool AFXAPI operator!=(const String& s1, const String::value_type * s2);
AFX_API bool AFXAPI operator!=(const String::value_type * s1, const String& s2);
AFX_API bool AFXAPI operator<(const String::value_type * s1, const String& s2);
AFX_API bool AFXAPI operator<(const String& s1, const String::value_type * s2);
AFX_API bool AFXAPI operator>(const String& s1, const String::value_type * s2);
AFX_API bool AFXAPI operator>(const String::value_type * s1, const String& s2);
AFX_API bool AFXAPI operator<=(const String& s1, const String::value_type * s2);
AFX_API bool AFXAPI operator<=(const String::value_type * s1, const String& s2);
AFX_API bool AFXAPI operator>=(const String& s1, const String::value_type * s2);
AFX_API bool AFXAPI operator>=(const String::value_type * s1, const String& s2);

inline bool AFXAPI operator!=(const String& s1, std::nullptr_t) {
	return s1 != (const String::value_type*)0;
}

inline bool AFXAPI operator==(const String& s1, std::nullptr_t) {
	return s1 == (const String::value_type*)0;
}


template <class A, class B> String Concat(const A& a, const B& b) {
	std::ostringstream os;
	os << a << b;
	return os.str().c_str();
}

template <class A, class B, class C> String Concat(const A& a, const B& b, const C& c) {
	std::ostringstream os;
	os << a << b << c;
	return os.str().c_str();
}



AFX_API String AFXAPI AfxLoadString(uint32_t nIDS);

struct CStringResEntry {
	uint32_t ID;
	const char *Ptr;
};

inline String ToLower(RCString s) { return s.ToLower(); }
inline String ToUpper(RCString s) { return s.ToUpper(); }

inline char ToLowerChar(char ch) { return (char)::tolower(ch); }

inline std::string ToLower(const std::string& s) {
	std::string r = s;
	std::transform(r.begin(), r.end(), r.begin(), ToLowerChar);
	return r;
}


} // Ext::

namespace EXT_HASH_VALUE_NS {
inline size_t hash_value(const Ext::String& s) {
	return Ext::hash_value((const Ext::String::value_type*)s, (s.length() * sizeof(Ext::String::value_type)));
}
}

EXT_DEF_HASH(Ext::String)

namespace std {
	inline int AFXAPI stoi(Ext::RCString s, size_t *idx = 0, int base = 10) {
		return stoi(Ext::explicit_cast<string>(s), idx, base);
	}

	inline long AFXAPI stol(Ext::RCString s, size_t *idx = 0, int base = 10) {
		return stol(Ext::explicit_cast<string>(s), idx, base);
	}

	inline long long AFXAPI stoll(Ext::RCString s, size_t *idx = 0, int base = 10) {
		return stoll(Ext::explicit_cast<string>(s), idx, base);
	}

	inline double AFXAPI stod(Ext::RCString s, size_t *idx = 0) {
		return stod(Ext::explicit_cast<string>(s), idx);
	}
} // std::



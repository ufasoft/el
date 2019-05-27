/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include EXT_HEADER(list)

#include EXT_HEADER_OPTIONAL

namespace Ext {
using std::unordered_map,
	std::optional, std::nullopt;

template <class T> class Array {
public:
	Array(size_t size)
		:	m_p(new T[size])
	{
	}

	~Array() {
		delete[] m_p;
	}

	T *get() const { return m_p; }
	T *release() { return exchange(m_p, nullptr); }
private:
	T *m_p;
};

template<> class Array<char> {
public:
	Array(size_t size)
		:	m_p((char*)Malloc(size))
	{
	}

	~Array() {
		if (m_p)
			free(m_p);
	}

	char *get() const { return m_p; }
	char *release() { return std::exchange(m_p, nullptr); }
private:
	char *m_p;
};



} // Ext::



namespace Ext {


/*!!!
class Istream {
public:
	std::istream& m_is;

	Istream(std::istream& is)
		:	m_is(is)
	{}

	void ReadString(String& s);
	void GetLine(String& s, char delim);
};


class Ostream : public ostream
{
public:
Ostream(streambuf *sb)
: ostream(sb)
{}
};*/

#if !defined(_MSC_VER) || _MSC_VER > 1400 


//!!!R #define AutoPtr std::unique_ptr
/*!!!R
template <class T> class COwnerArray : public std::vector<AutoPtr<T> > {
	typedef std::vector<AutoPtr<T> > base;
public:
	typedef typename base::iterator iterator;

	void Add(T *p) {
		base::push_back(AutoPtr<T>(p));
	}

	void Unlink(const T *p) {
		for (iterator i(this->begin()); i!=this->end(); ++i)
			if (i->get() == p) {
				i->release();
				erase(i);
				return;
			}
	}

	void Remove(const T *p) {
		for (iterator i(this->begin()); i!=this->end(); ++i)
			if (i->get() == p) {
				this->erase(i);
				return;
			}
	}
};
*/

template <class T> class CMTQueue {
	size_t m_cap;
	size_t m_mask;
	std::unique_ptr<uint8_t> m_buf;
	T * volatile m_r, * volatile m_w;
public:
	explicit CMTQueue(size_t cap)
		:	m_cap(cap+1)
		,	m_mask(cap)
	{
		for (size_t n=cap; n; n>>=1)
			if (!(n & 1))
				Throw(E_FAIL);
		m_buf.reset(new uint8_t[sizeof(T) * m_cap]);
		m_r = m_w = (T*)m_buf.get();
	}

	~CMTQueue() {
		clear();
	}

	size_t capacity() const { return m_cap-1; }

	size_t size() const { return (m_w-m_r) & m_mask; }

	bool empty() const { return m_r == m_w; }

	const T& front() const {
		if (m_r == m_w)
			Throw(ExtErr::IndexOutOfRange);
		return *m_r;
	}

	T& front() { return (T&)((const CMTQueue*)this)->front(); }

	T *Inc(T *p) {
		T *r = ++p;
		if (r == (T*)m_buf.get()+m_cap)
			r = (T*)m_buf.get();
		return r;
	}

	void push_back(const T& v) {
		T *n = Inc(m_w);
		if (n == m_r)
			Throw(ExtErr::QueueOverflow);
		new(m_w) T(v);
		m_w = n;
	}

	void pop_front() {
		if (empty())
			Throw(ExtErr::IndexOutOfRange);
		m_r->~T();
		m_r = Inc(m_r);
	}

	void clear() {
		while (!empty())
			pop_front();
	}
};

#endif

template <typename C, typename K>
bool Contains(const C& c, const K& key) {
	return c.find(key) != c.end();
}

template <typename C, class T>
bool ContainsInLinear(const C& c, const T& key) {
	return find(c.begin(), c.end(), key) != c.end();
}

template <class K, class T>
optional<T> Lookup(const unordered_map<K, T>& m, const K& key) {
	typename unordered_map<K, T>::const_iterator i = m.find(key);
	return i == m.end() ? optional<T>(nullopt) : i->second;
}


template <typename S1, typename S2>
typename S1::const_iterator Search(const S1& s1, const S2& s2) {
	return std::search(std::begin(s1), std::end(s1), std::begin(s2), std::end(s2));
}



template <class Q, class T>
bool Dequeue(Q& q, T& val) {
	if (q.empty())
		return false;
	val = q.front();
	q.pop();
	return true;
}

template <class T> class MyList : public std::list<T>
{
	typedef std::list<T> base;
public:
	typedef typename base::const_iterator const_iterator;
	typedef typename base::iterator iterator;
	//!!!	typedef _Nodeptr MyNodeptr;
	typedef typename base::_Node *MyNodeptr;

#if UCFG_STDSTL
#	if defined(__GNUC__)
	static const_iterator ToConstIterator(void *pNode)
	{
		return const_iterator((MyNodeptr)pNode);
	}
#	elif defined(_MSC_VER) && _MSC_VER > 1400 
	static const_iterator ToConstIterator(void *pNode)
	{
		return _Const_iterator<false>((MyNodeptr)pNode);
	}
#	else
	static iterator ToConstIterator(void *pNode)
	{
		return ((MyNodeptr)pNode);
	}
#	endif
#else
	static const_iterator ToConstIterator(void *pNode)
	{
		return (MyNodeptr)pNode;
	}
#endif
	static iterator ToIterator(void *pNode) {
		return iterator((MyNodeptr)pNode);
	}
};

template <typename LI>
void *ListIteratorToPtr(LI li) {
#if defined(_STLP_LIST) || defined(__GNUC__)
    return li._M_node;
#else
    return li._Mynode();
#endif
}

#	define ASSERT_INTRUSIVE

template <typename T>
class IntrusiveList : noncopyable {
	typedef IntrusiveList class_type;
public:
	typedef T value_type;

	class const_iterator {
	public:
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef T value_type;
		typedef ssize_t difference_type;
		typedef T* pointer;
		typedef T& reference;

		explicit const_iterator(const value_type *p = nullptr)
			:	m_p(p) {
		}

		const_iterator& operator=(const const_iterator& it) {
			m_p = it.m_p;
			return _self;
		}

		const value_type& operator*() const { return *m_p; };
		const value_type *operator->() const { return &operator*(); };

		const_iterator& operator++() {
			m_p = m_p->Next;
			return *this;
		}

		const_iterator operator++(int) {
			const_iterator r(_self);
			operator++();
			return r;
		}

		const_iterator& operator--() {
			m_p = m_p->Prev;
			return *this;
		}

		const_iterator operator--(int) {
			const_iterator r(_self);
			operator--();
			return r;
		}

		bool operator==(const const_iterator& it) const { return m_p == it.m_p; }
		bool operator!=(const const_iterator& it) const { return !operator==(it); }

	protected:
		const value_type *m_p;

	};

	class iterator : public const_iterator {
		typedef const_iterator base;
	public:
		explicit iterator(value_type *p = nullptr)
			:	const_iterator(p) {
		}

		explicit iterator(const const_iterator& ci)
			:	const_iterator(ci) {
		}

		iterator& operator=(const iterator& it) {
			base::m_p = it.m_p;
			return _self;
		}

		value_type& operator*() const { return *const_cast<value_type*>(base::m_p); };
		value_type *operator->() const { return &operator*(); };

		iterator& operator++() {
			base::m_p = base::m_p->Next;
			return *this;
		}

		iterator operator++(int) {
			iterator r(_self);
			operator++();
			return r;
		}

		iterator& operator--() {
			base::m_p = base::m_p->Prev;
			return *this;
		}

		iterator operator--(int) {
			iterator r(_self);
			operator--();
			return r;
		}
};


	IntrusiveList()
		:	m_size(0)
	{
		getP()->Prev = getP()->Next = getP();
	}

//!!!R	size_t size() const { return m_size; }
	bool empty() const { return getP()->Next == getP(); }

	const_iterator begin() const {
		return const_iterator(getP()->Next);
	}

	const_iterator end() const {
		return const_iterator(getP());
	}

	iterator begin() {
		return iterator(getP()->Next);
	}

	iterator end() {
		return iterator(getP());
	}

	const value_type& front() const { return *begin(); }
	const value_type& back() const {
		const_iterator it = end();
		--it;
		return *it;
	}

	value_type& front() { return *begin(); }
	value_type& back() {
		iterator it = end();
		--it;
		return *it;
	}

	iterator insert(iterator it, value_type& v) {
		T *p = v.Prev = exchange(it->Prev, &v);
		v.Next = &*it;
		p->Next = &v;
		++m_size;

		ASSERT_INTRUSIVE;

		return iterator(&v);
	}

	void push_back(value_type& v) {
		insert(end(), v);
	}

	void erase(const_iterator it) noexcept {
		T *n = it->Next, *p = it->Prev;
		p->Next = n;
		n->Prev = p;
		m_size--;
//		ASSERT((value_type*)(it.operator->()) != getP());
#ifdef _DEBUG
		value_type *v = (value_type*)(it.operator->());
		v->Next = v->Prev = 0;
#endif
	}

	size_t size() const { return m_size; }

	void clear() {
		m_size = 0;
		getP()->Prev = getP()->Next = getP();
		ASSERT_INTRUSIVE;
	}

	void splice(iterator _Where, class_type& _Right, iterator _First) {
		const_iterator _Last = _First;
		++_Last;
		if (this != &_Right || (_Where != _First && _Where != _Last)) {
			value_type& v = *_First;
			_Right.erase(_First);
			insert(_Where, v);
		}
		ASSERT_INTRUSIVE;
	}
protected:
	uint8_t m_base[sizeof(value_type)];
	size_t m_size;
	
	const T *getP() const { return (const T*)m_base; }
	T *getP() { return (T*)m_base; }
};


template <class I>
class Range {
public:
	I begin,
		end;

	//!!!	Range() {}

	Range(I b, I e)
		:	begin(b)
		,	end(e)
	{}
};

template <class C>
class CRange {
public:
	CRange(const C& c)
		:	b(c.begin())
		,	e(c.end())
	{}

	operator bool() const { return b != e; }
	void operator ++() {
		++b;
	}

	typename C::value_type& operator*() const { return *b; }
	typename C::value_type operator->() const { return &*b; }
private:
	typename C::const_iterator b,
		e;
};

template <class I>
void Reverse(const Range<I>& range) { reverse(range.begin, range.end); }

template <class I, class L>
void Sort(Range<I> range, L cmp) { sort(begin(range), end(range), cmp); }

template <class C, class P>
bool AllOf(C& range, P pred) { return all_of(begin(range), end(range), pred); }

template <class C>
void Sort(C& range) { sort(begin(range), end(range)); }

template <class C, class T>
void AddToUnsortedSet(C& c, const T& v) {
	if (find(c.begin(), c.end(), v) == c.end())
		c.push_back(v);
}

template <class C, class T>
void Remove(C& c, const T& v) {
	c.erase(std::remove(c.begin(), c.end(), v), c.end());
}


/*!!!
#if defined(_MSC_VER) && _MSC_VER >= 1600

template<typename T> inline size_t hash_value(const T& _Keyval) {
	return ((size_t)_Keyval ^ _HASH_SEED);
}

#endif
*/



template <class T>
class InterlockedSingleton {
public:
	InterlockedSingleton() {}

	T *operator->() {
		if (!s_p.load()) {
			T *p = new T;
			for (T *prev=0; !s_p.compare_exchange_weak(prev, p);)
				if (prev) {
					delete p;
					break;
				}
		}
		return s_p;
	}

	T *get() { return operator->(); }
	T& operator*() { return *operator->(); }
private:
	atomic<T*> s_p;

	InterlockedSingleton(const InterlockedSingleton&);
};

template <class T, class TR, class A, class B>
class DelayedStatic2 {
public:
	mutable atomic<T*> m_p;
	A m_a;
	B m_b;

	DelayedStatic2(const A& a, const B& b = TR::DefaultB)
		: m_p(0)
		, m_a(a)
		, m_b(b) {
	}

	~DelayedStatic2() {
		delete m_p.exchange(nullptr);
	}

	const T& operator*() const {
		if (!m_p.load()) {
			T *p = new T(m_a, m_b);
			CAlloc::DbgIgnoreObject(p);
			for (T *prev=0; !m_p.compare_exchange_weak(prev, p);)
				if (prev) {
					delete p;
					break;
				}
		}
		return *m_p;
	}

	operator const T&() const {
		return operator*();
	}
};

template <typename T, class L>
bool between(const T& v, const T& lo, const T& hi, L pred) {
	return !pred(v, lo) && !pred(hi, v);
}

template <typename T>
bool between(const T& v, const T& lo, const T& hi) {
	return between<T, std::less<T>>(v, lo, hi, std::less<T>());
}

template <typename T>
T RoundUpToMultiple(const T& x, const T& mul) {
	return (x + mul - 1) / mul * mul;
}

#if !UCFG_WDM
template <class T>
class CThreadTxRef : noncopyable {
public:
	static EXT_THREAD_PTR(T) t_pTx;

	template <class U>
	CThreadTxRef(U& u) {
		if (!(m_p = t_pTx)) {
			t_pTx = m_p = new(m_placeTx) T(u);
			m_bOwn = true;
		}
	}

	~CThreadTxRef() {
		if (m_bOwn) {
			t_pTx = nullptr;
			reinterpret_cast<T*>(m_placeTx)->~T();
		}
	}

	operator T&() { return *m_p; }
private:
	T *m_p;
	uint8_t m_placeTx[sizeof(T)];
	CBool m_bOwn;
};
#endif // !UCFG_WDM

} // Ext::

namespace EXT_HASH_VALUE_NS {

template <class T>
size_t AFXAPI hash_value(const Ext::Pimpl<T>& v) { return hash_value(v.m_pimpl); }

} // stdext::



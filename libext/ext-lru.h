#pragma once

#include EXT_HEADER_OPTIONAL

namespace Ext {
using std::pair;
using std::optional;
using std::nullopt;

#ifndef UCFG_DEFAULT_CACHE_SIZE
#	define UCFG_DEFAULT_CACHE_SIZE 256
#endif

template <typename C>
struct LruTraits {
	enum { IteratorSize = sizeof(typename C::iterator) };
};

struct UnorderedLruItem {
	UnorderedLruItem *Next, *Prev;
	void *Key;
	//!!!	uint8_t m_iter[LruTraits<std::unordered_map<int, int> >::IteratorSize];
};

template <typename K, typename T, typename C>
class LruBase : public C {
	typedef LruBase class_type;
	typedef C base;
public:
	typedef typename C::iterator iterator;
	typedef typename C::const_iterator const_iterator;

	struct LruItem {
		LruItem *Next, *Prev;
		const K *Key;
	};
	typedef IntrusiveList<LruItem> iterator_list_type;

	size_t m_maxSize;

	void clear() {
		m_list.clear();
		C::clear();
	}

	iterator find(const K& k) {
		iterator it = base::find(k);
		if (it != C::end())
			UpItem(it);
		return it;
	}

	void erase(iterator i) {
		m_list.erase(ToListIterator(i));
		C::erase(i);
	}

	size_t erase(const K& k) {
		iterator i = find(k);
		if (i != C::end()) {
			erase(i);
			return 1;
		}
		return 0;
	}

	class_type& operator=(const class_type& lru) {
		clear();
		m_maxSize = lru.m_maxSize;
		for (const_iterator i=lru.begin(), e=lru.end(); i!=e; ++i)
			PInsert(*i);
		return *this;
	}

	void SetMaxSize(size_t maxSize) { m_maxSize = maxSize; }
protected:
	iterator_list_type m_list;

	LruBase(size_t maxSize)
		:	m_maxSize(maxSize)
	{}

	LruBase(const class_type& lru) {
		operator=(lru);
	}

	template <typename V>
	LruItem& ValueToLruItem(V& v) {
		return (LruItem&)v.second;
	}

	LruItem& ValueToLruItem(UnorderedLruItem& uit) {
		return (LruItem&)uit;
	}

	template <typename V>
	typename iterator_list_type::iterator ValueToListIterator(V& v) {
		return typename iterator_list_type::iterator((LruItem*)&v.second);
	}

	typename iterator_list_type::iterator ValueToListIterator(UnorderedLruItem& uit) {
		return typename iterator_list_type::iterator((LruItem*)&uit);
	}

	typename iterator_list_type::iterator ToListIterator(iterator i) {
		return ValueToListIterator(i->second);
	}

	iterator ListItemToIterator(iterator it) { return it; }
	iterator ListItemToIterator(LruItem& lruItem) { return find(*lruItem.Key); }

	void UpItem(iterator it) {
		m_list.splice(m_list.begin(), m_list, ToListIterator(it));
	}

	std::pair<iterator, bool> PInsert(const typename C::value_type& v) {
		std::pair<iterator, bool> p = C::insert(v);
		iterator it = p.first;
		if (p.second) {
			if (C::size() >= m_maxSize) {
				erase(ListItemToIterator(m_list.back()));
			}
			LruItem& lruItem = ValueToLruItem(it->second);
			lruItem.Key = &it->first;
			m_list.insert(m_list.begin(), lruItem);
		} else
			UpItem(it);			
		return p;
	}

#if !UCFG_STDSTL														// really works only in ExtSTL
	std::observer_ptr<uint8_t> m_pTemporaryFreed;

	~LruBase() {
		if (m_pTemporaryFreed)
			base::VFreeNode(m_pTemporaryFreed);
	}

	void * __fastcall VBuyNode() override {				
		if (m_pTemporaryFreed)
			return m_pTemporaryFreed.release();
		return base::VBuyNode();
	}

	void __fastcall VFreeNode(void *p) override {
		if (!m_pTemporaryFreed)
			m_pTemporaryFreed.reset((uint8_t*)p);
		else
			base::VFreeNode(p);
	}
#endif // !UCFG_STDSTL
};

template <typename T, typename C = std::unordered_map<T, UnorderedLruItem> >		//!!! dont use boost::unordered_map, because rehashing invalidates iterators
class LruCache : public LruBase<T, UnorderedLruItem, C> {
	typedef LruCache class_type;
	typedef LruBase<T, UnorderedLruItem, C> base;
public:
	typedef T value_type;
	typedef typename base::iterator iterator;

	LruCache(size_t maxSize = UCFG_DEFAULT_CACHE_SIZE)
		:	base(maxSize)
	{}

	LruCache(const LruCache& lru)
		:	base(lru)
	{
	}

	pair<iterator, bool> insert(const value_type& v) {
		return base::PInsert(typename C::value_type(v, UnorderedLruItem()));
	}
};

template <class K, class T, class C = std::unordered_map<K, pair<T, UnorderedLruItem> > >
class LruMap : public LruBase<K, pair<T, UnorderedLruItem>, C> {
	typedef LruMap class_type;
	typedef LruBase<K, pair<T, UnorderedLruItem>, C> base;
public:
	typedef pair<K,T> value_type;
	typedef typename base::iterator iterator;

	using base::end;

	LruMap(size_t maxSize = UCFG_DEFAULT_CACHE_SIZE)
		:	base(maxSize)
	{}

	LruMap(const LruMap& lru)
		:	base(lru)
	{
	}

	std::pair<iterator, bool> insert(const value_type& v) {
		return base::PInsert(typename C::value_type(v.first, std::make_pair(v.second, UnorderedLruItem())));
	}

	T& operator[](const K& k) {
		iterator it = base::find(k);
		if (it != end())
			return it->second.first;
		return insert(std::make_pair(k, T())).first->second.first;
	}
private:
};

template <class K, class T, class C>
std::optional<T> Lookup(LruMap<K, T, C>& m, const K& key) {
	typename LruMap<K, T, C>::iterator i = m.find(key);
	return i == m.end() ? std::optional<T>(nullopt) : i->second.first;
}


} // Ext::



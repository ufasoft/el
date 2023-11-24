/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

// Smart Pointers

#include <el/libext/ext-cpp.h>
#include EXT_HEADER_ATOMIC

namespace Ext {

template <typename T> class PtrBase {
protected:
	T* m_p;
public:
	typedef T element_type;
	typedef T *pointer;
	typedef T &reference;

	PtrBase(T *p = 0)
		: m_p(p) {}

	inline T *get() const EXT_FAST_NOEXCEPT { return m_p; }
	inline T *operator->() const EXT_FAST_NOEXCEPT { return m_p; }

	T *release() noexcept {
		pointer r = m_p;
		m_p = nullptr;
		return r;
	}

	void swap(PtrBase &p) noexcept {
	//!!!Not defined yet	std::swap(m_p, p.m_p);
		T *t = m_p;
		m_p = p;
		p.m_p = t;
	}
};

template <typename T> class SelfCountPtr {
	T *m_p;
	std::atomic<int> *m_pRef;

	void Destroy() {
		if (m_pRef && !--m_pRef) {
			delete m_pRef;
			delete m_p;
		}
	}
public:
	SelfCountPtr()
		: m_p(0)
		, m_pRef(0)
	{}

	SelfCountPtr(T *p)
		: m_p(p)
		, m_pRef(new atomic<int>(1))
	{}

	SelfCountPtr(const SelfCountPtr& p)
		: m_p(p.m_p)
		, m_pRef(p.m_pRef)
	{
		m_pRef++;
	}

	~SelfCountPtr() {
		Destroy();
	}

	bool operator<(const SelfCountPtr& p) const { return m_p < p.m_p; }
	bool operator==(const SelfCountPtr& p) const { return m_p == p.m_p; }
	bool operator==(T *p) const { return m_p == p; }

	SelfCountPtr& operator=(const SelfCountPtr& p) {
		if (&p != this) {
			Destroy();
			m_p = p.m_p;
			++*(m_pRef = p.m_pRef);
		}
		return *this;
	}

	operator T*() const { return m_p; }
	T *operator->() const { return m_p; }
};

template <class T, class L> class CCounterIncDec {
public:
	static int __fastcall AddRef(T *p) EXT_FAST_NOEXCEPT {
		return L::Increment(p->m_aRef);
	}

	static int __fastcall Release(T *p) {
		int r = L::Decrement(p->m_aRef);
		if (!r)
			delete p;
		return r;
	}
};

template <class T> class CAddRefIncDec {
public:
	static int AddRef(T *p) {
		return p->AddRef();
	}

	static int Release(T *p) {
		return p->Release();
	}
};

void ThrowNonEmptyPointer();	// Helpers

template <typename T, typename L>
class RefPtr : public PtrBase<T> {
	typedef PtrBase<T> base;
protected:
	using base::m_p;
public:
	using base::get;

	RefPtr(T *p = 0)
		: base(p)
	{
		AddRef();
	}

	RefPtr(const RefPtr& p)
		: base(p)
	{
		AddRef();
	}

	RefPtr(EXT_RV_REF(RefPtr) rv)
		: base(rv.m_p)
	{
		rv.m_p = nullptr;
	}

	~RefPtr() {
		Destroy();
	}

	bool operator<(const RefPtr& p) const noexcept { return m_p < p.m_p; }
	bool operator==(const RefPtr& p) const noexcept { return m_p == p.m_p; }
	bool operator==(T *p) const noexcept { return m_p == p; }

	RefPtr& operator=(const RefPtr& p) {
		if (&p != this) {
			Destroy();
			m_p = p.m_p;
			AddRef();
		}
		return *this;
	}

	RefPtr& operator=(EXT_RV_REF(RefPtr) rv) {
		std::swap(m_p, rv.m_p);
		return *this;
	}

	void Attach(T *p) {
		if (m_p)
			ThrowNonEmptyPointer();
		m_p = p;
	}

	inline operator T*() const EXT_FAST_NOEXCEPT { return get(); }

	T** AddressOf() {
		if (m_p)
			ThrowNonEmptyPointer();
		return &m_p;
	}

	void reset() {
		if (m_p)
			L::Release(exchange(m_p, (T*)0));
	}
protected:
	void Destroy() {
		if (m_p)
			L::Release(m_p);
	}

	void AddRef() EXT_FAST_NOEXCEPT {
		if (m_p)
			L::AddRef(m_p);
	}
};

template <typename T>
struct ptr_traits {
	typedef typename T::interlocked_policy interlocked_policy;
};

template <typename T, typename I = typename ptr_traits<T>::interlocked_policy>
class ptr : public RefPtr<T, CCounterIncDec<T, I> > {
	typedef RefPtr<T, CCounterIncDec<T, I>> base;
protected:
	using base::m_p;
public:
	typedef ptr class_type;

	ptr(T *p = 0)
		: base(p)
	{}

	ptr(const std::nullptr_t&)
		: base(0)
	{}

	ptr(const ptr& p)
		: base(p)
	{}

	ptr(EXT_RV_REF(ptr) rv)
		: base(static_cast<EXT_RV_REF(base)>(rv))
	{}

	template <class U>
	ptr(const ptr<U>& pu)
		: base(pu.get())
	{}

	ptr& operator=(const ptr& p) {
		base::operator=(p);
		return *this;
	}

	ptr& operator=(T *p) {
		base::Destroy();
		m_p = p;
		base::AddRef();
		return *this;
	}

	ptr& operator=(EXT_RV_REF(ptr) rv) {
		base::operator=(static_cast<EXT_RV_REF(base)>(rv));
		return *this;
	}

	template <class U>
	ptr& operator=(const ptr<U>& pu) {
		base::operator=(pu.get());
		return *this;
	}

	ptr& operator=(const std::nullptr_t& np) {
		return operator=(class_type(np));
	}

	ptr *This() { return this; }

	int use_count() const { return m_p ? m_p->m_aRef : 0; }
};

template <typename U, typename T, typename L> U *StaticCast(const ptr<T, L>& p) 	{
	return static_cast<U*>(p.get());
}

template <class T>
class Pimpl {
public:
	ptr<T> m_pimpl;

	Pimpl() {}

	Pimpl(const Pimpl& x)
		: m_pimpl(x.m_pimpl)
	{}

	Pimpl(EXT_RV_REF(Pimpl) rv)
		: m_pimpl(static_cast<EXT_RV_REF(ptr<T>)>(rv.m_pimpl))
	{}

	EXPLICIT_OPERATOR_BOOL() const {
		return m_pimpl ? EXT_CONVERTIBLE_TO_TRUE : 0;
	}

	const T* operator->() const { return m_pimpl.get(); }
	T* operator->() { return m_pimpl.get(); }

	Pimpl& operator=(const Pimpl& v) {
		m_pimpl = v.m_pimpl;
		return *this;
	}

	Pimpl& operator=(EXT_RV_REF(Pimpl) rv) {
		m_pimpl.operator=(static_cast<EXT_RV_REF(ptr<T>)>(rv.m_pimpl));
		return *this;
	}

	bool operator<(const Pimpl& tn) const {
		return m_pimpl < tn.m_pimpl;
	}

	bool operator==(const Pimpl& tn) const {
		return m_pimpl == tn.m_pimpl;
	}

	bool operator!=(const Pimpl& tn) const {
		return !operator==(tn);
	}
protected:
	Pimpl(T *p)
		: m_pimpl(p)
	{}
};

template <typename T>
class addref_ptr : public RefPtr<T, CAddRefIncDec<T> > {
public:
	typedef addref_ptr class_type;
	typedef RefPtr<T, CAddRefIncDec<T> > base;

	addref_ptr(T *p = 0)
		: base(p)
	{}

	addref_ptr(const base& p) //!!! was ptr
		: base(p)
	{}

	addref_ptr& operator=(const addref_ptr& p) {
		base::operator=(p);
		return *this;
	}
};

template <class T>
class AFX_CLASS CRefCounted {
public:
	class AFX_CLASS CRefCountedData : public T {
	public:
		std::atomic<int> m_aRef;

		CRefCountedData()
			: m_aRef(0)
		{}

		void AddRef() noexcept {
			++m_aRef;
		}

		void Release() {
			if (!--m_aRef)
				delete this;
		}
	};

	CRefCountedData *m_pData;

	CRefCounted()
		: m_pData(new CRefCountedData)
	{
		m_pData->m_aRef = 1;
	}

	CRefCounted(const CRefCounted& rc)
		: m_pData(rc.m_pData)
	{
		m_pData->AddRef();
	}

	CRefCounted(CRefCountedData *p)
		: m_pData(p)
	{
		m_pData->m_aRef = 1;
	}

	~CRefCounted() {
		m_pData->Release();
	}

	void Init(CRefCountedData *p) {
		m_pData->Release();
		(m_pData = p)->m_dwRef = 1;
	}

	CRefCounted<T>& operator=(const CRefCounted<T>& val) {
		if (m_pData != val.m_pData) {
			m_pData->Release();
			(m_pData = val.m_pData)->AddRef();
		}
		return *this;
	}
};

} // Ext::


namespace EXT_HASH_VALUE_NS {
	template <typename T> size_t hash_value(const ptr<T>& v) {
	    return reinterpret_cast<size_t>(v.get());
	}
}

namespace std {
#if !UCFG_STD_OBSERVER_PTR

template <class T>
class observer_ptr : public Ext::PtrBase<T> {
	typedef Ext::PtrBase<T> base;

	using base::m_p;
public:
	using typename base::element_type;
	using typename base::pointer;
	using typename base::reference;

	observer_ptr() noexcept
		: base(0) {
	}

	observer_ptr(nullptr_t) noexcept
		: base(0) {
	}

	observer_ptr(const observer_ptr& p) noexcept
		: base(p.m_p) {
	}

	explicit observer_ptr(T *p) noexcept
		: base(p) {
	}

//!!!R	T *get() const noexcept { return m_p; }
//!!!R	T *operator->() const EXT_FAST_NOEXCEPT { return get(); }

	EXT_OPERATOR_BOOL() const noexcept { return m_p ? _CONVERTIBLE_TO_TRUE : 0; }

	T& operator*() const EXT_FAST_NOEXCEPT { return *m_p; }
	operator pointer() const EXT_FAST_NOEXCEPT { return m_p; }
	bool operator==(T *p) const noexcept { return m_p == p; }
	bool operator<(T *p) const noexcept { return m_p < p; }

/*!!!R	pointer release() noexcept {		// std::exchange may be unavailable yet
		pointer r = m_p;
		m_p = nullptr;
		return r;
	}
	*/
	void reset(pointer p) { m_p = p; }										//!!!TODO  = nullptr
};

#endif // !UCFG_STD_OBSERVER_PTR


} // std::

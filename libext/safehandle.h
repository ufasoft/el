/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#if UCFG_WIN32
#	ifdef NTDDI_WIN8
#		include <processthreadsapi.h>
#	else
#		include <windows.h>
#	endif
#endif

#include <el/libext/exthelpers.h>

namespace Ext {

class Exception;

class SafeHandleBase {
public:
	enum StateBits {
		SH_State_Closed		= 1
		, SH_State_Disposed = 2
		, SH_State_RefCount = 0xFFFFFFFC
		, SH_RefCountOne	= 4, // Amount to increment state field to yield a ref count increment of 1
	};

	mutable atomic<int> m_aState;

#if UCFG_USE_IN_EXCEPTION
	CInException InException;
#endif

	SafeHandleBase()
		: m_aState(SH_State_Closed)
	{
	}

    bool AddRef() const noexcept;
	virtual bool Release(bool fDispose) const;
	bool Dispose(bool bFromDtor = false);
	bool Close(bool bFromDtor = false) { return Dispose(bFromDtor); }   //!!!C For compatibility

	void swap(SafeHandleBase& r);
protected:
	virtual void InternalReleaseHandle() const = 0;
};

template <class T>
class CHandleBase : public SafeHandleBase {    //!!!?
public:

template <class U> friend class CHandleKeeper;
};

#ifndef WDM_DRIVER

class EXTAPI CTls : noncopyable {
#if UCFG_USE_PTHREADS
	pthread_key_t m_key;
#else
	unsigned long m_key; //!!! was ULONG
#endif
public:
	typedef CTls class_type;

	CTls();
	~CTls();

	__forceinline void *get_Value() const noexcept {
#if UCFG_USE_PTHREADS
		return ::pthread_getspecific(m_key);
#else
		return ::TlsGetValue(m_key);
#endif
	}

	void put_Value(const void *p);
	DEFPROP(void*, Value);
};

template <class T>
class single_tls_ptr : protected CTls {
	typedef CTls base;
public:
	single_tls_ptr(T *p = 0) {
		base::put_Value(p);
	}

	single_tls_ptr& operator=(const T* p) {
		base::put_Value(p);
		return _self;
	}

	single_tls_ptr& operator=(const single_tls_ptr& tls) {
		base::put_Value(tls);
		return _self;
	}

	operator T*() const noexcept { return (T*)get_Value(); }
	T* operator->() const noexcept { return operator T*(); }
};

/*!!!T
class CDestructibleTls : public CTls {
	typedef CTls base;
public:
	CDestructibleTls *Prev, *Next;

	CDestructibleTls();
	~CDestructibleTls();
	virtual void OnThreadDetach(void *p) {}
};
*/
#if UCFG_CPP11_THREAD_LOCAL || UCFG_USE_DECLSPEC_THREAD
#	define EXT_THREAD_PTR(typ) THREAD_LOCAL typ*
#else
#	define EXT_THREAD_PTR(typ) Ext::single_tls_ptr<typ>
#endif

/*!!!T
template <typename T>
class thread_specific_ptr : CDestructibleTls {
public:
	void reset(T *p = 0) {
		T* prev = (T*)get_Value();
		if (prev != p) {
			put_Value(p);
			delete prev;
		}
	}

	operator T*() const { return (T*)get_Value(); }
	T* operator->() const { return operator T*(); }
protected:
	void OnThreadDetach(void *p) override {
		delete (T*)p;
	}
};

template <typename T>
class tls_ptr : thread_specific_ptr<T> {
	typedef thread_specific_ptr<T> base;
public:
	T& operator*() {
		T *r = *this;
		if (!r)
			reset(r = new T);
		return *r;
	}
};
*/

#endif // !WDM_DRIVER

template <class H> class CHandleKeeper {
protected:
	mutable const H* m_hp;
	mutable CBool m_bIncremented;
public:
	CHandleKeeper(const H& hp)
		: m_hp(&hp)
	{
		AddRef();
	}

	CHandleKeeper()
		: m_hp(0)
	{
	}

	CHandleKeeper(const CHandleKeeper& k) {
		operator=(k);
	}

	CHandleKeeper& operator=(const CHandleKeeper& k) {
		m_hp = k.m_hp;												// std::exchange() may be not defined yet
		k.m_hp = nullptr;
		m_bIncremented = k.m_bIncremented;
		k.m_bIncremented = false;
		return *this;
	}

	struct _Boolean {
		int i;
	};

	operator int _Boolean::*() const {
    	return m_bIncremented ? &_Boolean::i : 0;
	}

//!!!R	operator typename H::handle_type() { return m_hp->DangerousGetHandle(); }

	~CHandleKeeper() {
		Release();
	}

	void AddRef() {
		m_bIncremented = m_hp->AddRef();
	}

	void Release() {
		if (m_bIncremented) {
			m_bIncremented = false;
			m_hp->Release(false);
		}
	}
};


class EXTAPI SafeHandle : public Object, public CHandleBase<SafeHandle> {
public:
#ifndef WDM_DRIVER
	//!!!R static CTls t_pCurrentHandle;
	static EXT_THREAD_PTR(void) t_pCurrentHandle;
#endif
protected:
	const intptr_t m_invalidHandleValue;
private:
	typedef CHandleBase<SafeHandle> base;
	EXT_MOVABLE_BUT_NOT_COPYABLE(SafeHandle);

	mutable atomic<intptr_t> m_aHandle;
	CBool m_bOwn;
public:
	typedef intptr_t handle_type;

	SafeHandle()
		: m_invalidHandleValue(-1)
		, m_aHandle(-1)
		, m_bOwn(true)
	{}

	SafeHandle(intptr_t invalidHandle, bool)
		: m_invalidHandleValue(invalidHandle)
		, m_aHandle(invalidHandle)
		, m_bOwn(true)
#if UCFG_WDM
		, m_pObject(nullptr)
#endif
	{}

	SafeHandle(intptr_t handle);
	virtual ~SafeHandle();

	SafeHandle(EXT_RV_REF(SafeHandle) rv);
	SafeHandle& operator=(EXT_RV_REF(SafeHandle) rv);

//!!!	void Release() const;
	//!!!  void CloseHandle();
	intptr_t DangerousGetHandle() const;

	void Attach(intptr_t handle, bool bOwn = true);
#ifdef _WIN32
	void Attach(HANDLE handle, bool bOwn = true) { Attach((intptr_t)handle, bOwn); }
#endif

	void ThreadSafeAttach(intptr_t handle, bool bOwn = true);

	EXPLICIT_OPERATOR_BOOL() const {
		return Valid() ? EXT_CONVERTIBLE_TO_TRUE : 0;
	}

	void swap(SafeHandle& r);

#if UCFG_WDM
protected:
	mutable void *m_pObject;
	mutable CBool m_CreatedReference;
	void AttachKernelObject(void *pObj, bool bKeepRef);
public:
	void Attach(PKEVENT pObj) { AttachKernelObject(pObj, false); }
	void Attach(PKMUTEX pObj) { AttachKernelObject(pObj, false); }

	NTSTATUS InitFromHandle(HANDLE h, ACCESS_MASK DesiredAccess, POBJECT_TYPE ObjectType, KPROCESSOR_MODE  AccessMode);
#endif

	intptr_t Detach();
	void Duplicate(intptr_t h, uint32_t dwOptions = 0);
	virtual bool Valid() const;

	class HandleAccess : public CHandleKeeper<SafeHandle> {
		typedef CHandleKeeper<SafeHandle> base;
	public:
		HandleAccess()
		{}

		HandleAccess(const HandleAccess& ha)
			: base(ha)
		{}

		HandleAccess(const SafeHandle& h)
			: base(h)
		{}

		~HandleAccess();

		operator SafeHandle::handle_type() {
			if (!m_bIncremented)
				return 0;
			return m_hp->DangerousGetHandle();
		}

#if UCFG_WIN32
		operator HANDLE() { return (HANDLE)operator SafeHandle::handle_type(); }
#endif
	};

	class BlockingHandleAccess : public HandleAccess {
		HandleAccess* m_pPrev;
	public:
		BlockingHandleAccess(const SafeHandle& h);
		~BlockingHandleAccess();
	};

	void InternalReleaseHandle() const override;
	intptr_t DangerousGetHandleEx() const { return m_aHandle.load(); }
protected:
	void ReplaceHandle(intptr_t h) { m_aHandle = h; }
	virtual void ReleaseHandle(intptr_t h) const;
private:
	void AfterAttach(bool bOwn);

	template <class T> friend class CHandleKeeper;
};

template <typename T>
typename T::handle_type Handle(T& x) {
	return T::HandleAccess(x);
}

template <typename T>
typename T::handle_type BlockingHandle(T& x) {
	return T::BlockingHandleAccess(x);
}

ENUM_CLASS(HandleInheritability) {
	None
	, Inheritable
} END_ENUM_CLASS(HandleInheritability);

} // Ext::

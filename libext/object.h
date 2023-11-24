#pragma once

#include EXT_HEADER_ATOMIC

namespace Ext {
using std::atomic;

class CRuntimeClass;

class NonInterlockedPolicy {
public:
	template <class T> static int Increment(T& v) { int r = v + 1; v = r; return r; }
	template <class T> static int Decrement(T& v) { int r = v - 1; v = r; return r; }
};

class InterlockedPolicy {
public:
	template <class T> static T Increment(atomic<T>& a) { return ++a; }
	template <class T> static T Decrement(atomic<T>& a) { return --a; }
};

class Object {
public:
//	typedef NonInterlockedPolicy interlocked_policy;	// Must be explicit for safity

	static const AFX_DATA CRuntimeClass classObject;
	mutable atomic<int> m_aRef;

	Object()
		: m_aRef(0)
	{
	}

	Object(const Object& ob)
		: m_aRef(0)
	{
	}

	Object& operator=(const Object& ob) {
		m_aRef.store(0);
		return *this;
	}

	virtual ~Object() {
	}

	int RefCount() const noexcept { return m_aRef.load(); }

	bool IsHeaped() const { return m_aRef < (10000); }			//!!! should be replaced to some num limits
	void InitInStack() { m_aRef = 20000; }

//!!!R	virtual CRuntimeClass *GetRuntimeClass() const;

#ifdef _AFXDLL
	static CRuntimeClass* PASCAL _GetBaseClass();
	static CRuntimeClass* PASCAL GetThisClass();
#endif

};


} // Ext::

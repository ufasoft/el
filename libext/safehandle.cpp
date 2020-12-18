/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_WIN32
#	include <windows.h>
#endif

#if UCFG_WIN32 && !UCFG_MINISTL
#	include <shlwapi.h>

#	include <el/libext/win32/ext-win.h>
#endif

namespace Ext {
using namespace std;


void SafeHandleBase::swap(SafeHandleBase& r) {
	int t = m_aState.load();
	m_aState.store(r.m_aState.load());
	r.m_aState.store(t);
#if UCFG_USE_IN_EXCEPTION
	InException.swap(r.InException);
#endif
}

bool SafeHandleBase::AddRef() const noexcept {
	for (int oldState; !((oldState = m_aState) & SH_State_Closed);)
		if (m_aState.compare_exchange_weak(oldState, oldState + SH_RefCountOne))
			return true;
	return false;
}

bool SafeHandleBase::Release(bool fDispose) const {
	for (int disposeMask = fDispose ? SH_State_Disposed : 0, oldState; !((oldState = m_aState) & disposeMask);) {
		int newState = (oldState - SH_RefCountOne)
			| disposeMask
			| ((oldState & SH_State_RefCount) == SH_RefCountOne ? SH_State_Closed : 0);
		if (m_aState.compare_exchange_weak(oldState, newState)) {
			if ((oldState & (SH_State_RefCount | SH_State_Closed)) != SH_RefCountOne)
				return false;
			InternalReleaseHandle();
			return true;
		}
	}
	return false;
}

bool SafeHandleBase::Dispose(bool bFromDtor) {
	bool r = false;
#if UCFG_USE_IN_EXCEPTION
	if (bFromDtor && InException) {
		try {
			r = Release(true);
		} catch (RCExc) {
		}
	} else
#endif
		r = Release(true);
	return r;
}


#ifndef WDM_DRIVER
EXT_THREAD_PTR(void) SafeHandle::t_pCurrentHandle;
#endif

SafeHandle::HandleAccess::~HandleAccess() {
#if !defined(_MSC_VER) || defined(_CPPUNWIND)
			try //!!!
#endif
			{
				Release();
			}
#if !defined(_MSC_VER) || defined(_CPPUNWIND)
			catch (RCExc) {
			}
#endif
}

SafeHandle::SafeHandle(intptr_t handle)
	: m_aHandle(handle)
	, m_invalidHandleValue(-1)
#ifdef WDM_DRIVER
	, m_pObject(nullptr)
#endif
{
	m_aState = SH_RefCountOne;

#ifdef WIN32
	Win32Check(Valid());
#endif
}

SafeHandle::~SafeHandle() {
	InternalReleaseHandle();
}

SafeHandle::SafeHandle(EXT_RV_REF(SafeHandle) rv)
	: m_invalidHandleValue(rv.m_invalidHandleValue)
	, m_bOwn(rv.m_bOwn)
{
	m_aHandle = rv.m_aHandle.load();
	m_aState = rv.m_aState.load();

	rv.m_aHandle = rv.m_invalidHandleValue;
	rv.m_bOwn = true;
	rv.m_aState = SH_State_Closed | SH_State_Disposed;
}

SafeHandle& SafeHandle::operator=(EXT_RV_REF(SafeHandle) rv) {
	InternalReleaseHandle();

	m_aHandle = rv.m_aHandle.load();
	m_bOwn = rv.m_bOwn;
	m_aState = rv.m_aState.load();

	rv.m_aHandle = rv.m_invalidHandleValue;
	rv.m_bOwn = true;
	rv.m_aState = SH_State_Closed | SH_State_Disposed;

	return *this;
}

void SafeHandle::swap(SafeHandle& r) {
	base::swap(r);
	std::swap(m_bOwn, r.m_bOwn);

	intptr_t t = m_aHandle;
	m_aHandle = r.m_aHandle.load();
	r.m_aHandle = t;
}

void SafeHandle::InternalReleaseHandle() const {
	intptr_t h = m_aHandle.exchange(m_invalidHandleValue);
	if (h != m_invalidHandleValue) {
		if (m_bOwn) //!!!
			ReleaseHandle(h);
	}
#if UCFG_WDM
	if (m_CreatedReference) {
		if (m_pObject) {
			ObDereferenceObject(m_pObject);
			m_pObject = nullptr;
		}
		m_CreatedReference = false;
	}
#endif
}

#if UCFG_WDM
void SafeHandle::AttachKernelObject(void *pObj, bool bKeepRef) {
	InternalReleaseHandle();
	m_CreatedReference = bKeepRef;
	m_pObject = pObj;
}

NTSTATUS SafeHandle::InitFromHandle(HANDLE h, ACCESS_MASK DesiredAccess, POBJECT_TYPE ObjectType, KPROCESSOR_MODE  AccessMode) {
	KEVENT *p;
	NTSTATUS st = ObReferenceObjectByHandle(h, DesiredAccess, ObjectType, AccessMode, (void**)&p, 0);
	AttachKernelObject(p, true);
	return st;
}

#endif

void SafeHandle::ReleaseHandle(intptr_t h) const {
#if UCFG_USE_POSIX
	CCheck(::close((int)h));
#elif UCFG_WIN32
	Win32Check(::CloseHandle((HANDLE)h));
#else
	NtCheck(::ZwClose((HANDLE)h));
#endif
}

/*!!!
void SafeHandle::CloseHandle() {
if (Valid() && m_bOwn)
Win32Check(::CloseHandle(exchange(m_handle, (HANDLE)0)));
}*/

intptr_t SafeHandle::DangerousGetHandle() const {
	int state = m_aState;
	if (!(state & SH_State_RefCount) || (state & SH_State_Disposed))
		Throw(ExtErr::ObjectDisposed);
	return m_aHandle.load();
}

void SafeHandle::AfterAttach(bool bOwn) {
	if (Valid()) {
		m_aState = SH_RefCountOne;
		m_bOwn = bOwn;
	} else {
		m_aHandle = m_invalidHandleValue;
#if UCFG_USE_POSIX
		CCheck(-1);
#elif UCFG_WIN32
		Win32Check(false);
#endif
	}
}

void SafeHandle::ThreadSafeAttach(intptr_t handle, bool bOwn) {
	intptr_t prev = m_invalidHandleValue;
	if (m_aHandle.compare_exchange_strong(prev, handle))
		AfterAttach(bOwn);
	else
		ReleaseHandle(handle);
}

void SafeHandle::Attach(intptr_t handle, bool bOwn) {
	if (Valid())
		Throw(ExtErr::AlreadyOpened);
	m_aHandle = handle;
	AfterAttach(bOwn);
}

intptr_t SafeHandle::Detach() { //!!!
	m_aState = SH_State_Closed | SH_State_Disposed;
	m_bOwn = false;
	return m_aHandle.exchange(m_invalidHandleValue);
}

void SafeHandle::Duplicate(intptr_t h, uint32_t dwOptions) {
#if UCFG_WIN32
	if (Valid())
		Throw(ExtErr::AlreadyOpened);
	HANDLE hMy;
	Win32Check(::DuplicateHandle(GetCurrentProcess(), (HANDLE)h, GetCurrentProcess(), &hMy, 0, FALSE, dwOptions));
	m_aHandle = intptr_t(hMy);
	m_aState = SH_RefCountOne;
#else
	Throw(E_NOTIMPL);
#endif
}

bool SafeHandle::Valid() const {
	return m_aHandle != 0 && m_aHandle != m_invalidHandleValue;
}

SafeHandle::BlockingHandleAccess::BlockingHandleAccess(const SafeHandle& h)
	: HandleAccess(h)
{
#ifndef WDM_DRIVER
	m_pPrev = (HandleAccess*)(void*)t_pCurrentHandle;
	t_pCurrentHandle = this;
#endif

	/*!!!
	if (m_sock.m_socketThreader)
	{
	if (m_sock.m_socketThreader->m_bClosing)
	m_sock.Close();
	//!!!R			m_sock.m_socketThreader->m_lock.Lock();
	m_prevKeeper = exchange(m_sock.m_socketThreader->m_pCurrentHandleKeeper, this);
	}*/
}

SafeHandle::BlockingHandleAccess::~BlockingHandleAccess() {
#ifndef WDM_DRIVER
	t_pCurrentHandle = m_pPrev;
#endif

	/*!!!
	if (m_sock.m_socketThreader)
	{
	if (m_sock.m_socketThreader->m_bClosing)
	m_sock.Close();
	m_sock.m_socketThreader->m_pCurrentHandleKeeper = m_prevKeeper;
	//!!!R			m_sock.m_socketThreader->m_lock.Unlock();
	}*/
}


} // Ext::



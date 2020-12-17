/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


#ifdef _MSC_VER

#if defined(WIN32) && !UCFG_WCE
//!!!	#include <intrin.h>
/*!!!
#ifndef InterlockedIncrement
extern "C"
{
long  __cdecl _InterlockedIncrement(long volatile *Addend);
long  __cdecl _InterlockedDecrement(long volatile *Addend);
long  __cdecl _InterlockedCompareExchange(long * volatile Dest, long Exchange, long Comp);
long  __cdecl _InterlockedExchange(long * volatile Target, long Value);
long  __cdecl _InterlockedExchangeAdd(long * volatile Addend, long Value);
}

//#include <winbase.h>



#pragma intrinsic (_InterlockedCompareExchange)
#pragma intrinsic (_InterlockedExchange)
#pragma intrinsic (_InterlockedExchangeAdd)
#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedDecrement)
#endif*/
#endif



/*!!!?
#	if _MSC_VER>=1600
#		include <intrin.h>
#	endif
*/


#if UCFG_WCE
	typedef LONG* PVLONG;
	#define INTERLOCKED_FUN(fun) fun
#elif defined(WIN32)
//	typedef volatile LONG* PVLONG;
	#define INTERLOCKED_FUN(fun) fun
#elif defined(WDM_DRIVER)
	typedef volatile LONG* PVLONG;
	#define INTERLOCKED_FUN(fun) fun
#endif

#if UCFG_WCE
#	pragma warning (disable: 4732)	// intrinsic '_Interlocked...' is not supported in this architectur
#endif


#if defined(_MSC_VER) && !defined(__INTRIN_H_)
	extern "C" long __cdecl _InterlockedCompareExchange(long volatile *, long, long);
	extern "C" char  _InterlockedCompareExchange8(char volatile *, char, char);
	extern "C" long __cdecl _InterlockedIncrement(long volatile *Destination);
	extern "C" long __cdecl _InterlockedDecrement(long volatile *Destination);
	extern "C" long _InterlockedAnd(long volatile *Destination, long Value);
	extern "C" long _InterlockedOr(long volatile *Destination, long Value);
	extern "C" long _InterlockedXor(long volatile *Destination, long Value);
	extern "C" long __cdecl _InterlockedExchange(long volatile *Destination, long Value);
	extern "C" char _InterlockedExchange8(char volatile *Destination, char Value);
	extern "C" long __cdecl _InterlockedExchangeAdd(long volatile *, long);
	extern "C" int64_t _InterlockedCompareExchange64(int64_t volatile *, int64_t, int64_t);

#	ifdef _M_ARM
#	define _InterlockedCompareExchange InterlockedCompareExchange
#	define _InterlockedIncrement InterlockedIncrement
#	define _InterlockedDecrement InterlockedDecrement
#	else
#	pragma intrinsic(_InterlockedCompareExchange)
#	pragma intrinsic(_InterlockedCompareExchange8)
#	pragma intrinsic(_InterlockedIncrement)
#	pragma intrinsic(_InterlockedDecrement)
#	endif // !_M_ARM

#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchange8)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange64)

#	if UCFG_PLATFORM_X64
	extern "C" int64_t __cdecl _InterlockedExchange64(int64_t volatile *, int64_t);
#	pragma intrinsic(_InterlockedExchange64)
#	endif // UCFG_PLATFORM_X64



//	extern "C" long _cdecl InterlockedAdd(long volatile *Destination, long Valu );
//!!! IPF only #	pragma intrinsic(_InterlockedAdd)

#	if _MSC_VER >= 1700 /*&& (_MSC_VER < 1900 || UCFG_WDM)*/ || defined(_M_X64)		//!!!?
	extern "C" void * __cdecl _InterlockedCompareExchangePointer(void * volatile * _Destination, void * _Exchange, void * _Comparand);
#		pragma intrinsic(_InterlockedCompareExchangePointer)
#	endif

#endif

#endif // _MSC_VER



namespace Ext {

#ifdef __GNUC__


class Interlocked {
public:
	static int32_t Add(volatile int32_t& v, int32_t add) {
		return __sync_add_and_fetch(&v, add);

/*!!!
		int32_t r = add;
		asm volatile("lock; xaddl %0, %1" : "=r"(r), "+m"(v) : "0"(r) : "memory");
		return r+add; */
	}

	static int32_t Increment(volatile int32_t& v) {
		return Add(v, 1);
	}

	static int32_t Decrement(volatile int32_t& v) {
		return Add(v, -1);
	}


	static int32_t Exchange(volatile int32_t& v, int32_t n) {
		return __sync_lock_test_and_set(&v, n);
/*		int32_t r = n;
		asm volatile("xchg %0, %1" : "=r"(r), "+m"(v) : "0"(r) : "memory");
		return r;*/
	}

	static int32_t CompareExchange(volatile int32_t& d, int32_t e, int32_t c) {
		return __sync_val_compare_and_swap(&d, c, e);
/*!!!		int32_t prev = c;
		asm volatile( "lock; cmpxchg %3,%1"
			: "=a" (prev), "=m" (d)
			: "0" (prev), "r" (e)
			: "memory", "cc");
		return prev; */
	}

	template <typename T>
	static void *CompareExchange(T * volatile & d, T *e, T *c) {
		return __sync_val_compare_and_swap(&d, c, e);
/*!!!		T *prev = c;
		asm volatile( "lock; cmpxchg %3,%1"
			: "=a" (prev), "=m" (d)
			: "0" (prev), "r" (e)
			: "memory", "cc");
		return prev;
*/
	}               
};

#else

class Interlocked {
public:
#	if UCFG_WCE
	template <class T> static T Increment(volatile T& v) noexcept { return T(_InterlockedIncrement((LONG*)(&v))); }
	template <class T> static T Decrement(volatile T& v) noexcept { return T(_InterlockedDecrement((LONG*)(&v))); }
#	else
//	template <class T> __forceinline static T Increment(volatile T& v) noexcept { return T(_InterlockedIncrement((volatile long*)(&v))); }
//	template <class T> __forceinline static T Decrement(volatile T& v) noexcept { return T(_InterlockedDecrement((volatile long*)(&v))); }

	__forceinline static int32_t Increment(volatile int32_t& v) noexcept { return _InterlockedIncrement((volatile long*)(&v)); }
	__forceinline static int32_t Decrement(volatile int32_t& v) noexcept { return _InterlockedDecrement((volatile long*)(&v)); }
#	endif // !UCFG_WCE

/*!!!R	template <class T> static T Exchange(T &d, T e)
	{
		return (T)INTERLOCKED_FUN(InterlockedExchange)((PVLONG)(&d),e);
	}*/

	template <class T> static T *Exchange(T * volatile &d, void *e) noexcept {
#ifdef WDM_DRIVER
		return (T*)InterlockedExchangePointer((void * volatile *)(&d), e);
#elif defined(_M_IX86)
		return (T*)_InterlockedExchange((volatile long *)(&d), (long)e);
#else
		return (T*)_InterlockedExchangePointer((void * volatile *)(&d), e);
#endif
	}

	static int32_t Exchange(volatile int32_t& d, int32_t e) noexcept { return _InterlockedExchange(reinterpret_cast<long*>(const_cast<int32_t*>(&d)), e); }
	static char Exchange(volatile char& d, char v) noexcept { return _InterlockedExchange8(&d, v); }

	static int32_t CompareExchange(volatile int32_t& d, int32_t e, int32_t c) noexcept { return _InterlockedCompareExchange(reinterpret_cast<long*>(const_cast<int32_t*>(&d)), e, c); }
	static char CompareExchange(volatile char& d, char e, char c) noexcept { return _InterlockedCompareExchange8(&d, e, c); }

	template <typename T>
	static T *CompareExchange(T *volatile & d, T *e, T *c) noexcept {
#	if _MSC_VER >= 1700 || defined(_M_X64)
		return static_cast<T*>(_InterlockedCompareExchangePointer((void* volatile *)&d, e, c));
#else
		return (T*)_InterlockedCompareExchange(reinterpret_cast<long volatile*>(&d), (long)e, (long)c);
#endif
	}

#	if defined(_M_X64)
	static intptr_t CompareExchange(volatile intptr_t& d, intptr_t e, intptr_t c) noexcept {
		return (intptr_t)CompareExchange(reinterpret_cast<int * volatile&>(d), (int*)e, (int*)c);
	}
#	endif

	static int32_t ExchangeAdd(volatile int32_t& d, int32_t v) noexcept {
		return _InterlockedExchangeAdd(reinterpret_cast<long*>(const_cast<int32_t*>(&d)), v);
	}

#if LONG_MAX == 2147483647
	static int32_t And(volatile int32_t& d, int32_t v) {
		return _InterlockedAnd(reinterpret_cast<volatile long*>(&d), v);
	}

	static int32_t Or(volatile int32_t& d, int32_t v) {
		return _InterlockedOr(reinterpret_cast<volatile long*>(&d), v);
	}

	static int32_t Xor(volatile int32_t& d, int32_t v) {
		return _InterlockedXor(reinterpret_cast<volatile long*>(&d), v);
	}

	static int32_t Add(volatile int32_t& d, int32_t v) {
		return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&d), v) + v;
	}
#endif
};

#ifdef WIN32

	inline void atomic_add_int(volatile int *dst, int n) {
		_InterlockedExchangeAdd((long*)dst, n);
	}

	inline void atomic_add_int(volatile unsigned int *dst, int n) {
		_InterlockedExchangeAdd((long*)dst, n);
	}

	inline int atomic_cmpset_int(volatile int *dst, int old, int n) {
		return Interlocked::CompareExchange(*(volatile int32_t *)dst,n,old) == old;
	}

	inline int atomic_cmpset_int(volatile unsigned int *dst, int old, int n) {
		return Interlocked::CompareExchange(*(volatile int32_t*)dst,n,old) == old;
	}

#endif


#endif // __GNUC__

} // Ext::






/*######   Copyright (c) 1997-2023 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
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

bool AFXAPI Equal(RCSpan x, RCSpan y) {
    const uint8_t *px = x.data(), *py = y.data();
	return !px || !py
        ? px == py
        : x.size() == y.size() && (px == py || !memcmp(px, py, x.size()));
}

void CPrintable::Print(ostream& os) const {
	os << ToString();
}

String CPrintable::ToString() const {
	ostringstream os;
	Print(os);
	return os.str();
}

const class CRuntimeClass Object::classObject =
	{ "Object", sizeof(Object), 0xffff, NULL, 0, NULL };

/*!!!R CRuntimeClass *Object::GetRuntimeClass() const {
	return RUNTIME_CLASS(Object);
}*/

#ifdef _AFXDLL
	CRuntimeClass* PASCAL Object::_GetBaseClass() {
		return NULL;
	}
	CRuntimeClass* PASCAL Object::GetThisClass() {
		return _RUNTIME_CLASS(Object);
	}
#endif


typedef map<int, CExceptionFabric*> CExceptionFabrics;
static InterlockedSingleton<CExceptionFabrics> g_exceptionFabrics;







String AssertFailedExc::get_Message() const {
	return EXT_STR(FileName << "(" << LineNumber << "): Assertion Failed: " << Exp);
}

AFX_API bool AFXAPI AfxAssertFailedLine(const char* sexp, const char*fileName, int nLine) {
/*!!!R #	if defined(WIN32) && !defined(_AFX_NO_DEBUG_CRT)
		// we remove WM_QUIT because if it is in the queue then the message box
		// won't display
		MSG msg;
		BOOL bQuit = PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
		BOOL bResult = _CrtDbgReport(_CRT_ASSERT, lpszFileName, nLine, NULL, NULL);
		if (bQuit)
			PostQuitMessage((int)msg.wParam);
		if (bResult)
			Ext::AfxDebugBreak();
#	else */
#if UCFG_WDM
		KeBugCheck(HRESULT_FROM_WIN32(ERROR_ASSERTION_FAILURE));
#else
		throw AssertFailedExc(sexp, fileName, nLine);
#endif
//!!!#	endif
	return false;
}

#if UCFG_WCE

extern "C" {

#if !UCFG_STDSTL
int _daylight;
#endif

void __cdecl API_wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line) {
		throw AssertFailedExc(_Message, _File, _Line);
}

void _cdecl abort() {
	exit(3);
}

int _cdecl remove(const char *path) {
	bool b = ::DeleteFile(String(path));
	if (!b) {
		errno = EFAULT;		//!!!TODO
		return -1;
	}
	return 0;
}



} // "C"

#endif  // UCFG_WCE


#if 0 && (UCFG_WCE || UCFG_USE_OLD_MSVCRTDLL) //!!!?

extern "C" {

#define IS_2_POW_N(X)   (((X)&(X-1)) == 0)
#define PTR_SZ          sizeof(void *)


__declspec(dllexport) void * __cdecl my_aligned_malloc(size_t size, size_t align)
{
	uintptr_t ptr, retptr, gap, offset;
	offset = 0;

/* validation section */
	ASSERT(IS_2_POW_N(align));
	ASSERT(offset == 0 || offset < size);

align = (align > PTR_SZ ? align : PTR_SZ) -1;

/* gap = number of bytes needed to round up offset to align with PTR_SZ*/
gap = (0 - offset)&(PTR_SZ -1);

if ( (ptr =(uintptr_t)Malloc(PTR_SZ +gap +align +size)) == (uintptr_t)NULL)
    return NULL;

retptr =((ptr +PTR_SZ +gap +align +offset)&~align)- offset;
((uintptr_t *)(retptr - gap))[-1] = ptr;

return (void *)retptr;
}

__declspec(dllexport) void  __cdecl my_aligned_free(void *memblock)
{
		uintptr_t ptr;

		if (memblock == NULL)
				return;

		ptr = (uintptr_t)memblock;

		/* ptr points to the pointer to starting of the memory block */
		ptr = (ptr & ~(PTR_SZ -1)) - PTR_SZ;

		/* ptr is the pointer to the start of memory block*/
		ptr = *((uintptr_t *)ptr);
		Free((void *)ptr);
}


} // "C"

#endif // UCFG_WCE || UCFG_USE_OLD_MSVCRTDLL



extern "C" void _cdecl AfxTestEHsStub(void *prevFrame) {
	if (((uint8_t*)prevFrame - (uint8_t*)&prevFrame) < 5 * sizeof(void *)) {
#if UCFG_WDM
		KeBugCheck(E_FAIL);
#else
		std::cerr << "Should be compiled with /EHs /EHc-" << endl;
		abort();
#endif
	}
	//!!!ThrowImp(1);
}

void AFXAPI CCheckErrcode(int en) {
	if (en) {
#if UCFG_HAVE_STRERROR
		ThrowS(HRESULT_FROM_C(en), strerror(en));
#else
		Throw(E_NOTIMPL); //!!!
#endif
	}
}

} // Ext::


extern "C" {

PFN_memcpy g_fastMemcpy = &memcpy;

#if UCFG_USE_MASM && UCFG_WIN32 && defined(_M_IX86)		//!!! UCFG_CPU_X86_X64

void *MemcpySse(void *dest, const void *src, size_t count);

static int InitFastMemcpy() {
	if (Ext::CpuInfo().get_Features().SSE)
		g_fastMemcpy = &MemcpySse;
	return 1;
}

static int s_initFastMemcpy = InitFastMemcpy();

#endif

#if UCFG_CPU_X86_X64
uint8_t g_bHasSse2, g_bHasAvx, g_bHasAvx2;

static int InitBignumFuns() {
	const Ext::CpuInfo::FeatureInfo& f = Ext::CpuInfo().get_Features();
	g_bHasSse2 = f.SSE2;
	g_bHasAvx = f.AVX;
	g_bHasAvx2 = f.AVX2;
	return 1;
}

static int s_initBignumFuns = InitBignumFuns();
#endif // UCFG_CPU_X86_X64


} // "C"

namespace ExtSTL {
#if !UCFG_STDSTL || !UCFG_STD_MUTEX || UCFG_SPECIAL_CRT
	const adopt_lock_t adopt_lock = {};
	const defer_lock_t defer_lock = {};
	const try_to_lock_t try_to_lock = {};
#endif // !UCFG_STDSTL || !UCFG_STD_MUTEX

} // ExtSTL::

#if UCFG_DEFINE_NEW
void __cdecl operator delete(void* p) {
	free(p);
}

void * __cdecl operator new(size_t sz) {
	return Ext::Malloc(sz);
}

void __cdecl operator delete[](void* p) {
	free(p);
}

void * __cdecl operator new[](size_t sz) {
	return Ext::Malloc(sz);
}

#endif // UCFG_DEFINE_NEW

namespace std {

static const char
	s_upperHexDigits[] = "0123456789ABCDEF",
	s_lowerHexDigits[] = "0123456789abcdef";

ostream& __stdcall operator<<(ostream& os, Ext::RCSpan cbuf) {
	if (!cbuf.data())
		return os << EXT_NULLPTR_TRACE_LITERAL;
	const char *digits = os.flags() & ios::uppercase ? s_upperHexDigits : s_lowerHexDigits;
	for (size_t i = 0, size = cbuf.size(); i < size; ++i) {
		uint8_t n = cbuf[i];
		os.put(digits[n >> 4]).put(digits[n & 15]);
	}
	return os;
}

wostream& __stdcall operator<<(wostream& os, Ext::RCSpan cbuf) {
	if (!cbuf.data())
		return os << EXT_NULLPTR_TRACE_LITERAL;
	const char *digits = os.flags() & ios::uppercase ? s_upperHexDigits : s_lowerHexDigits;
	for (size_t i = 0, size = cbuf.size(); i < size; ++i) {
		uint8_t n = cbuf[i];
		os.put(digits[n >> 4]).put(digits[n & 15]);
	}
	return os;
}

} // std::

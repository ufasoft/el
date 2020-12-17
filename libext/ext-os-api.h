/*######   Copyright (c) 1997-2018 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

//!!! #include "../eh/ext_eh.h"
#include <setjmp.h>

#if defined(__cplusplus)
#	include <eh.h>

#	define NTAPI __stdcall

#	if UCFG_WDM

typedef long(__stdcall *PTOP_LEVEL_EXCEPTION_FILTER)(struct _EXCEPTION_POINTERS *ExceptionInfo);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;

typedef int BOOL;

#	endif

struct EHExceptionRecord;

#	if UCFG_MSC_VERSION >= 1900
#		pragma warning(disable : 5025)
#	endif

namespace Ext {

struct ExcRefs {
	EHExceptionRecord *&ExceptionRecord;
	void *&Context;
	void *&_pFrameInfoChain;
	void *&_curexcspec;
#	if !UCFG_WCE
	int &_cxxReThrow;
	_se_translator_function &_translator;
#	endif
#	ifdef _M_X64
	void *&_curexception; /* current exception */
	void *&_curcontext;   /* current exception context */
	uint64_t &_ImageBase;
	uint64_t &_ThrowImageBase;
#	endif
#	if defined _M_X64 || defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID
	EHExceptionRecord *&_pForeignException;
	int &_HandlerSearchState;
	ptrdiff_t &_UnwindStateorOffset;
	bool &_IsOffset;
#	endif

	ExcRefs(EHExceptionRecord *&er, void *&ctx, void *&frameInfoChain, void *&curexcspec
#	if !UCFG_WCE
			, int &cxxReThrow, _se_translator_function &tr
#	endif
#	ifdef _M_X64
			, void *&curexception, void *&curcontext, uint64_t &imageBase, uint64_t &throwImageBase
#	endif
#	if defined _M_X64 || defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID
			, EHExceptionRecord *&foreignException
			, int& handlerSearchState
			, ptrdiff_t& unwindStateorOffset
			, bool& isOffset
#	endif
			)
		: ExceptionRecord(er)
		, Context(ctx)
		, _pFrameInfoChain(frameInfoChain)
		, _curexcspec(curexcspec)
#	if !UCFG_WCE
		, _cxxReThrow(cxxReThrow)
		, _translator(tr)
#	endif
#	ifdef _M_X64
		, _curexception(curexception)
		, _curcontext(curcontext)
		, _ImageBase(imageBase)
		, _ThrowImageBase(throwImageBase)
#	endif
#	if defined _M_X64 || defined _M_ARM || defined _M_ARM64 || defined _M_HYBRID
		, _pForeignException(foreignException)
		, _HandlerSearchState(handlerSearchState)
		, _UnwindStateorOffset(unwindStateorOffset)
		, _IsOffset(isOffset)
#	endif
	{
	}

	ExcRefs *operator->() { return this; }

	static ExcRefs __stdcall GetForCurThread();
	static int &__stdcall ProcessingThrowRef(); // for uncaught_exception
};

bool __stdcall InitTls(void *hInst);

} // namespace Ext
#endif // defined(__cplusplus) && !UCFG_WCE

#if defined(__cplusplus)

namespace Ext {
typedef bool(__stdcall *PFN_HOOK_SelectFastRaiseException)(void *pExceptionObject, void *_pThrowInfo, void *imageBase);
PFN_HOOK_SelectFastRaiseException __cdecl SetExceptionHookHandler(PFN_HOOK_SelectFastRaiseException h);

#	if defined _WIN32 && defined _WINNT_
typedef BOOLEAN(NTAPI *PFN_RtlDispatchException)(IN PEXCEPTION_RECORD ExceptionRecord, IN PCONTEXT ContextRecord);
PFN_RtlDispatchException __stdcall GetProcAddress_RtlDispatchException();
#	endif

} // namespace Ext

#endif

__BEGIN_DECLS

#ifdef _WINNT_
void _stdcall API_RtlRaiseException(EXCEPTION_RECORD *e);
void _stdcall API_TryMyRtlRaiseException(EXCEPTION_RECORD *e);
void _stdcall API_RaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments, const ULONG_PTR *lpArguments);

void _stdcall IMP_RtlUnwind(void *TargetFrame, void *TargetIp, PEXCEPTION_RECORD ExceptionRecord, void *ReturnValue);
void _stdcall API_RtlUnwind(void *TargetFrame, void *TargetIp, PEXCEPTION_RECORD ExceptionRecord, void *ReturnValue);
//!!!LPTOP_LEVEL_EXCEPTION_FILTER
void *_stdcall API_SetUnhandledExceptionFilter(void * /*!!!	LPTOP_LEVEL_EXCEPTION_FILTER*/ lpTopLevelExceptionFilter);

#	ifdef _M_X64
void *_stdcall API_RtlPcToFileHeader(const void *PcValue, void **pBaseOfImage);
void _stdcall API_RtlUnwindEx(void *TargetFrame, PVOID TargetIp, PEXCEPTION_RECORD ExceptionRecord, PVOID ReturnValue, PCONTEXT OriginalContext, void *HistoryTable);
void *_stdcall API_RtlLookupFunctionEntry(DWORDLONG ControlPC, DWORDLONG *ImageBase, DWORDLONG *TargetGp);

#	endif

PVOID _stdcall API_EncodePointer(PVOID Ptr);

BOOL _stdcall API_IsBadReadPtr(CONST VOID *lp, UINT_PTR ucb);
BOOL _stdcall API_IsBadWritePtr(VOID *lp, UINT_PTR ucb);
BOOL _stdcall API_IsBadCodePtr(void *lpfn);
BOOLEAN _stdcall API_RtlpGetStackLimits(void **stackLimit, void **stackBase);

typedef ULONG(_cdecl *PFN_printf)(const char *format, ...);
extern PFN_printf g_API_printf;

#endif // _WINNT_

#	if UCFG_WCE

struct TEB {
	void *DriverDirectCaller;
	DWORD OwnerProcessVersion;
	void *ThreadInfo;
	size_t ProcStackSize;
	int CurFiber;
	void *StackBound;
	void *StackBase;
};

HRESULT API_SafeArrayGetVartype(SAFEARRAY *psa, VARTYPE *pvt);

#	endif // UCFG_WCE

#	if UCFG_EXT_C
#		ifdef _M_ARM
#			define _JBLEN 11
#			define _JBTYPE int
#		endif
#	endif UCFG_EXT_C

#	ifndef _JMP_BUF_DEFINED
#		define _JMP_BUF_DEFINED
typedef _JBTYPE jmp_buf[_JBLEN];
#	endif

__declspec(noreturn) void __cdecl API_longjmp(jmp_buf env, int val);
__declspec(noreturn) void _cdecl API_terminate();
__declspec(noreturn) void _cdecl API_unexpected();

__END_DECLS

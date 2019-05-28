#define _CRTIMP2
#define _ACRTIMP
#define UCFG_DEFINE_NEW 0
//#define DBG 0

//#include <el/libext.h>

#include <el/inc/ext_config.h>
#include <el/libext/vc.h>


#include <Windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winternl.h>

//#include <el/win/ntdll.h>
#include <el/win/nt.h>


#pragma comment(lib, "psapi")  // for GetModuleInformation
#pragma comment(lib, "ntdll")

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace Ext {

LIST_ENTRY *g_pLdrpTlsList;
ULONG *g_pLdrpNumberOfTlsEntries;

#ifdef _M_IX86			// unreferenced in x64
static void *static_memchr(const void * buf, int chr, size_t cnt)
{
        while ( cnt && (*(unsigned char *)buf != (unsigned char)chr) ) {
                buf = (unsigned char *)buf + 1;
                cnt--;
        }

        return(cnt ? (void *)buf : NULL);
}

static const unsigned char *ConstBufFind(const unsigned char *P, size_t Size, const unsigned char *aP, size_t aSize) {			// copy of ConstBuf::Find()
	for (const unsigned char *p=P, *e=p+(Size-aSize); p <= e;) {
		if (const unsigned char *q = (const unsigned char*)static_memchr(p, aP[0], e-p+1)) {
			if (aSize==1 || !memcmp(q+1, aP+1, aSize-1))
				return q;
			else
				p = q+1;
		} else
			break;
	}
	return 0;
}
#endif // _M_IX86

bool DetermineNtVars() {
	MODULEINFO miNtDll;
	if (!::GetModuleInformation(::GetCurrentProcess(), GetModuleHandleW(L"NTDLL.dll"), &miNtDll, sizeof miNtDll))
		return false;

	for (byte *p=(byte*)miNtDll.lpBaseOfDll, *e=(byte*)miNtDll.lpBaseOfDll+miNtDll.SizeOfImage-20; p<e; ++p) {
#ifdef _M_IX86																	//!!!H Find LdrpInitializeTls
		if (!memcmp(p, "\x6A\x06\x8D\x78\x08\x59\xF3\xA5\x8B\x0D", 10)) {
			g_pLdrpTlsList = (LIST_ENTRY*)(*(DWORD_PTR*)(p+10)-sizeof(DWORD_PTR));

			if (const byte *q = ConstBufFind(p+10+sizeof(DWORD_PTR), 40, (const unsigned char*)"\x48\x1C\xFF\x05", 4)) {
				g_pLdrpNumberOfTlsEntries = (ULONG*)*(DWORD_PTR*)(q+4);
				return true;
			}
		}
#endif
	}
	return false;
}

struct IEnumThread {
	virtual void OnThread(DWORD threadId) =0;
};

void EnumThreads(IEnumThread& enumThread) {
	DWORD pid = ::GetCurrentProcessId();
		
	HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	THREADENTRY32 entry = { sizeof entry };
	for (BOOL b=::Thread32First(hSnap, &entry); b; b=::Thread32Next(hSnap, &entry)) {
		if (entry.th32OwnerProcessID == pid)
			enumThread.OnThread(entry.th32ThreadID);
	}
	::CloseHandle(hSnap);
}

NT::RTL_BITMAP LdrpTlsBitmap;
ULONG LdrpStaticTlsBitmapVector[4];

bool __stdcall InitTls(void * hInst) {
	if ((byte)GetVersion() >= 6)
		return true;

	void *baseAddr = hInst;

	ULONG DirectorySize;
	PIMAGE_TLS_DIRECTORY tlsDirectory = (PIMAGE_TLS_DIRECTORY)NT::RtlImageDirectoryEntryToData(baseAddr, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &DirectorySize);
	if (!tlsDirectory) //!!! || *(ULONG*)tlsDirectory->AddressOfIndex != UNINITIALIZED_TLS_INDEX)
		return true;

	NT::TEB *teb = (NT::TEB*)NtCurrentTeb();
	NT::PEB *peb = (NT::PEB*)teb->ProcessEnvironmentBlock;
    
	NT::PLDR_DATA_TABLE_ENTRY dataTableEntry = 0;

	PLIST_ENTRY Head,Next;
    Head = &peb->Ldr->InMemoryOrderModuleList;
	Next = Head->Flink;

	for (; Next!=Head; Next = Next->Flink) {
		NT::PLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(Next, NT::LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
		if (entry->DllBase == baseAddr) {
			dataTableEntry = entry;
			break;
		}
	}
	if (!dataTableEntry)
		return false;
	if (dataTableEntry->TlsIndex)
		return true;

	if (!DetermineNtVars())
		return false;


	
	HANDLE hHeap = GetProcessHeap();


	int nThread = 0;

	struct CountEnumThread : IEnumThread {
		int *m_p;

		void OnThread(DWORD threadId) {
			++ *m_p;
		}
	} countThreads;
	countThreads.m_p = &nThread;
	EnumThreads(countThreads);

	
/*
	PPROCESS_TLS_INFORMATION *tlsInfo = (PPROCESS_TLS_INFORMATION *)::HeapAlloc(hHeap, (ULONG_PTR)((PUCHAR)NtdllBaseTag + 0x000C0000), 
		nThread * sizeof( THREAD_TLS_INFORMATION ) +
		sizeof( PROCESS_TLS_INFORMATION ) - sizeof( THREAD_TLS_INFORMATION )
		);
	if (tlsInfo)
		return false;*/


	dataTableEntry->TlsIndex = 0xFFFF;			//!!! value should be != 0
    

	//!!!if (teb->ThreadLocalStoragePointer) 
	{

		/*
		PROCESS_HEAP_ENTRY he = { 0 };
		void *pInHeap;
		while (::HeapWalk(hHeap, &he)) {
			if (he.lpData <= teb->ThreadLocalStoragePointer && (byte*)he.lpData+he.cbData>teb->ThreadLocalStoragePointer) {
				pInHeap = he.lpData;
				goto LAB_FOUND_IN_HEAP;
			}
		}
		return false;
LAB_FOUND_IN_HEAP:
		DWORD cbTlsVec;
		int nIndexed;
		if (pInHeap == teb->ThreadLocalStoragePointer) {
			cbTlsVec = HeapSize(hHeap, 0, pInHeap);
			nIndexed = cbTlsVec/sizeof(DWORD_PTR);
		} else {
			cbTlsVec = HeapSize(hHeap, 0, (DWORD_PTR*)teb->ThreadLocalStoragePointer-2);
			nIndexed = *((DWORD_PTR*)teb->ThreadLocalStoragePointer-2)/sizeof(DWORD_PTR);
		}*/

		if (!LdrpTlsBitmap.SizeOfBitMap)
			RtlInitializeBitMap(&LdrpTlsBitmap, (PULONG)&LdrpStaticTlsBitmapVector, sizeof(LdrpStaticTlsBitmapVector));
		RtlSetBits(&LdrpTlsBitmap, 0, *g_pLdrpNumberOfTlsEntries);

		ULONG index = RtlFindClearBitsAndSet(&LdrpTlsBitmap, 1, 0);
		if (index == ULONG(-1))
			return false;

		++(*g_pLdrpNumberOfTlsEntries);								//!!!  if DLL unloaded this counter can be incorrect
		*(ULONG*)tlsDirectory->AddressOfIndex = index;

		NT::LDRP_TLS_ENTRY *tlsEntry = (NT::LDRP_TLS_ENTRY*)::HeapAlloc(hHeap, 0, sizeof(NT::LDRP_TLS_ENTRY));
		tlsEntry->Tls = *tlsDirectory;
//!!!			tlsEntry->Tls.TlsIndex = -1;
		NT::InsertTailList((NT::PLIST_ENTRY)g_pLdrpTlsList, (NT::PLIST_ENTRY)&tlsEntry->Links);
		
		tlsEntry->Tls.Characteristics = index;

		struct SwapThreadLocalStoragePointer : IEnumThread {
			HANDLE m_hHeap;
			int m_index;
			PIMAGE_TLS_DIRECTORY m_pTlsDir;

			void OnThread(DWORD threadId) {
				HANDLE hThread = ::OpenThread(MAXIMUM_ALLOWED, FALSE, threadId);
//!!!				ASSERT(hThread);

				NT::THREAD_BASIC_INFORMATION info;
				DWORD dw;
				NTSTATUS st = NtQueryInformationThread(hThread, (THREADINFOCLASS)NT::ThreadBasicInformation, &info, sizeof info, &dw);
//!!!				ASSERT(st >= 0);
				CloseHandle(hThread);
				NT::TEB *teb1 = (NT::TEB*)info.TebBaseAddress;
				DWORD_PTR *p = (DWORD_PTR*)HeapAlloc(m_hHeap, 0, (m_index+1)*sizeof(DWORD_PTR));
				if (teb1->ThreadLocalStoragePointer)
					memcpy(p, teb1->ThreadLocalStoragePointer, m_index*sizeof(DWORD_PTR));
				else
					memset(p, 0, m_index*sizeof(DWORD_PTR));
				teb1->ThreadLocalStoragePointer = p;
				size_t size = m_pTlsDir->EndAddressOfRawData-m_pTlsDir->StartAddressOfRawData;
				void *tdata = ::HeapAlloc(m_hHeap, 0, size);
				memcpy(tdata, (void*)m_pTlsDir->StartAddressOfRawData, size);						
				p[m_index] = (DWORD_PTR)tdata;
				
			}

		} swapTlsStorage;
		swapTlsStorage.m_pTlsDir = tlsDirectory;
		swapTlsStorage.m_hHeap = hHeap;
		swapTlsStorage.m_index = index;
		EnumThreads(swapTlsStorage);
	}
	return true;
}



} // Ext::







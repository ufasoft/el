/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_USE_POSIX
#	include <sys/mman.h>
#endif

#if UCFG_WIN32
#	include <windows.h>
#endif

namespace Ext {
using namespace std;

static int MemoryMappedFileAccessToInt(MemoryMappedFileAccess access) {
#if UCFG_USE_POSIX
	switch (access) {
	case MemoryMappedFileAccess::None: return PROT_NONE;
	case MemoryMappedFileAccess::ReadWrite:			return PROT_READ | PROT_WRITE;
	case MemoryMappedFileAccess::Read:				return PROT_READ;
	case MemoryMappedFileAccess::Write:				return PROT_WRITE;
	case MemoryMappedFileAccess::CopyOnWrite:		return PROT_READ | PROT_WRITE;
	case MemoryMappedFileAccess::ReadExecute:		return PROT_READ | PROT_EXEC;
	case MemoryMappedFileAccess::ReadWriteExecute:	return PROT_READ | PROT_WRITE | PROT_EXEC;
	default:
		Throw(E_INVALIDARG);	
	}
#else
	switch (access) {
	case MemoryMappedFileAccess::None:				return PAGE_NOACCESS;
	case MemoryMappedFileAccess::ReadWrite:			return PAGE_READWRITE;
	case MemoryMappedFileAccess::Read:				return PAGE_READONLY;
	case MemoryMappedFileAccess::Write:				return PAGE_READWRITE;
	case MemoryMappedFileAccess::CopyOnWrite:		return PAGE_EXECUTE_READ;
	case MemoryMappedFileAccess::ReadExecute:		return PAGE_EXECUTE_READ;
	case MemoryMappedFileAccess::ReadWriteExecute:	return PAGE_EXECUTE_READWRITE;
	default:
		Throw(E_INVALIDARG);	
	}
#endif
}


MemoryMappedView::MemoryMappedView(EXT_RV_REF(MemoryMappedView) rv)
	:	Offset(exchange(rv.Offset, 0))
	,	Address(exchange(rv.Address, nullptr))
	,	Size(exchange(rv.Size, 0))
	,	Access(rv.Access)
	,	AddressFixed(rv.AddressFixed)
{
}

MemoryMappedView& MemoryMappedView::operator=(EXT_RV_REF(MemoryMappedView) rv) {
	Unmap();
	Address = exchange(rv.Address, nullptr);
	Offset = exchange(rv.Offset, 0);
	Size = exchange(rv.Size, 0);
	Access = rv.Access;
	AddressFixed = rv.AddressFixed;
	return *this;
}

void MemoryMappedView::Map(MemoryMappedFile& file, uint64_t offset, size_t size, void *desiredAddress) {
	if (Address)
		Throw(ExtErr::AlreadyOpened);
	Offset = offset;
	Size = size;
#if UCFG_USE_POSIX
	int prot = MemoryMappedFileAccessToInt(Access);
	int flags = Access==MemoryMappedFileAccess::CopyOnWrite ? 0 : MAP_SHARED;
	if (AddressFixed)
		flags |= MAP_FIXED;
	void *a = ::mmap(desiredAddress, Size, prot, flags, file.GetHandle(), offset);
	CCheck(a != MAP_FAILED);
	Address = a;
#else
	DWORD dwAccess = 0;
	switch (Access) {
	case MemoryMappedFileAccess::ReadWrite:			dwAccess = FILE_MAP_READ | FILE_MAP_WRITE;	break;
	case MemoryMappedFileAccess::Read:				dwAccess = FILE_MAP_READ;					break;
	case MemoryMappedFileAccess::Write:				dwAccess = FILE_MAP_WRITE;					break;
	case MemoryMappedFileAccess::CopyOnWrite:		dwAccess = FILE_MAP_COPY;					break;
	case MemoryMappedFileAccess::ReadExecute:		dwAccess = FILE_MAP_READ;					break;
	case MemoryMappedFileAccess::ReadWriteExecute:	dwAccess = FILE_MAP_ALL_ACCESS;				break;
	}
#	if UCFG_WCE
	Win32Check((Address = ::MapViewOfFile((HANDLE)file.GetHandle(), dwAccess, DWORD(Offset>>32), DWORD(Offset), Size)) != 0);
#	else
	Win32Check((Address = ::MapViewOfFileEx((HANDLE)file.GetHandle(), dwAccess, DWORD(Offset>>32), DWORD(Offset), Size, desiredAddress)) != 0);
#	endif
#endif
}

void MemoryMappedView::Unmap() {
	if (void *a = exchange(Address, nullptr)) {
#if UCFG_USE_POSIX
		CCheck(::munmap(a, Size));
#else
		Win32Check(::UnmapViewOfFile(a));
#endif
	}
}

void MemoryMappedView::Flush() {
#if UCFG_USE_POSIX
	CCheck(::msync(Address, Size, MS_SYNC));
#elif UCFG_WIN32
	Win32Check(::FlushViewOfFile(Address, Size));
#else
	Throw(E_NOTIMPL);
#endif
}

void AFXAPI MemoryMappedView::Protect(void *p, size_t size, MemoryMappedFileAccess access) {
	int prot = MemoryMappedFileAccessToInt(access);
#if UCFG_USE_POSIX
	CCheck(::mprotect(p, size, prot));
#else
	DWORD prev;
	Win32Check(::VirtualProtect(p, size, prot, &prev));
#endif
}

MemoryMappedView MemoryMappedFile::CreateView(uint64_t offset, size_t size, MemoryMappedFileAccess access) {
	MemoryMappedView r;
	r.Access = access;
	r.Map(_self, offset, size);
	return std::move(r);
}

MemoryMappedFile MemoryMappedFile::CreateFromFile(File& file, RCString mapName, uint64_t capacity, MemoryMappedFileAccess access) {
	int prot = MemoryMappedFileAccessToInt(access);
	MemoryMappedFile r;
	r.Access = access;
#if UCFG_WIN32	
	r.m_hMapFile.Attach((intptr_t)::CreateFileMapping((HANDLE)file.DangerousGetHandle(), 0, prot, uint32_t(capacity>>32), uint32_t(capacity), mapName));	
#else
	Throw(E_NOTIMPL);
#endif
	return std::move(r);
}

MemoryMappedFile MemoryMappedFile::CreateFromFile(RCString path, FileMode mode, RCString mapName, uint64_t capacity, MemoryMappedFileAccess access) {
	File file;
	file.Open(path, mode);
	return CreateFromFile(file, mapName, capacity, access);
};


MemoryMappedFile MemoryMappedFile::OpenExisting(RCString mapName, MemoryMappedFileRights rights, HandleInheritability inheritability) {
	MemoryMappedFile r;
#if UCFG_WIN32
	DWORD dwAccess = 0;
	if (int(rights) & int(MemoryMappedFileRights::Read))
		dwAccess |= FILE_MAP_READ;
	if (int(rights) & int(MemoryMappedFileRights::Write))
		dwAccess |= FILE_MAP_WRITE;
	if (int(rights) & int(MemoryMappedFileRights::Execute))
		dwAccess |= FILE_MAP_EXECUTE;
	r.m_hMapFile.Attach((intptr_t)::OpenFileMapping(dwAccess, inheritability==HandleInheritability::Inheritable, mapName));
#else
	Throw(E_NOTIMPL);
#endif
	return std::move(r);
}

void VirtualMem::Alloc(size_t size, MemoryMappedFileAccess access, bool bLargePage) {
	if (m_address)
		Throw(E_FAIL);
	m_size = size;
#if UCFG_USE_POSIX
	int flags = 0 
		| (bLargePage ? MAP_HUGETLB : 0;
	void *a = ::mmap(0, size, MemoryMappedFileAccessToInt(access), flags, 0, 0);
	CCheck(a != MAP_FAILED);
	m_address = a;
#else
	Win32Check(bool(m_address = (byte*)::VirtualAlloc(0, size, MEM_COMMIT | (bLargePage ? MEM_LARGE_PAGES : 0), MemoryMappedFileAccessToInt(access))));
#endif
}

void VirtualMem::Free() {
	if (void *a = exchange(m_address, nullptr)) {
#if UCFG_USE_POSIX
		CCheck(::munmap(a, m_size));
#else
		if (a)
			Win32Check(::VirtualFree(a, 0, MEM_RELEASE));
#endif
	}
}


} // Ext::


/*######   Copyright (c) 1997-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


#define CREATE_NEW          1 //!!!  windows
typedef struct tagEXCEPINFO EXCEPINFO;

#if UCFG_USE_POSIX
#	include <nl_types.h>
#endif

#include EXT_HEADER_SHARED_MUTEX

#include EXT_HEADER_FILESYSTEM

namespace Ext {

inline path operator/(const path& a, const String& s) { return path(a) /= path(s.c_str()); }
inline path operator/(const path& a, const char s[]) { return path(a) /= path(s); }


using std::wstring;
using std::istringstream;

class Stream;
class DirectoryInfo;

typedef uint32_t (*AFX_THREADPROC)(void *);


bool AFXAPI GetSilentUI();
void AFXAPI SetSilentUI(bool b);

#if !UCFG_WDM
class CResID : public CPersistent, public CPrintable {
public:
	AFX_API CResID(UINT nResId = 0);
	AFX_API CResID(const char *lpName);
	AFX_API CResID(const String::value_type *lpName);
	AFX_API CResID& operator=(const char *resId);
	AFX_API CResID& operator=(const String::value_type *resId);
	AFX_API CResID& operator=(RCString resId);
	AFX_API operator const char *() const;
	AFX_API operator const String::value_type *() const;
	AFX_API operator UINT() const;

	bool operator==(const CResID& resId) const {
		return m_resId==resId.m_resId && m_name==resId.m_name;
	}

	String ToString() const override;
//!!!private:
	uintptr_t m_resId;
	String m_name;

	void Read(const BinaryReader& rd) override;
	void Write(BinaryWriter& wr) const override;
};

}
namespace EXT_HASH_VALUE_NS {
	inline size_t hash_value(const Ext::CResID& resId) { return std::hash<uint64_t>()((uint64_t)resId.m_resId) + std::hash<Ext::String>()(resId.m_name); }
}
EXT_DEF_HASH(Ext::CResID) namespace Ext {


#endif

const int NOTIFY_PROBABILITY = 1000;

#if UCFG_USE_POSIX

class DlException : public Exception {
	typedef Exception base;
public:
	DlException();
};

inline void DlCheck(int rc) {
	if (rc)
		throw DlException();
}


#endif

class EXTAPI CDynamicLibrary {
	typedef CDynamicLibrary class_type;
public:
	mutable CInt<HMODULE> m_hModule;
	path Path;

	CDynamicLibrary() {
	}

	CDynamicLibrary(const path& path, bool bDelay = false) {
		Path = path;
		if (!bDelay)
			Load(path);
	}

	~CDynamicLibrary();

	operator HMODULE() const {
		if (!m_hModule)
			Load(Path);
		return m_hModule;
	}

	void Load(const path& path) const;
	void Free();
	FARPROC GetProcAddress(const CResID& resID);
private:
	CDynamicLibrary(const class_type&);
	CDynamicLibrary& operator=(const CDynamicLibrary&);
};

enum _NoInit {
	E_NoInit
};

#if UCFG_WIN32

class DlProcWrapBase {
protected:
	void* m_p;
public:
	void Init(HMODULE hModule, RCString funname);
protected:
	DlProcWrapBase()
		: m_p(0)
	{}

	DlProcWrapBase(RCString dll, RCString funname);
};

template <typename F>
class DlProcWrap : public DlProcWrapBase {
	typedef DlProcWrapBase base;
public:
	DlProcWrap() {
	}

	DlProcWrap(HMODULE hModule, RCString funname) {
		Init(hModule, funname);
	}

	DlProcWrap(RCString dll, RCString funname)
		: base(dll, funname)
	{
	}

	operator F() const { return (F)m_p; }
};

template <typename F> bool GetProcAddress(F& pfn, HMODULE hModule, RCString funname) {
	return pfn = (F)::GetProcAddress(hModule, funname);
}

template <typename F> bool GetProcAddress(F& pfn, RCString dll, RCString funname) {
	return GetProcAddress(pfn, ::GetModuleHandle(dll), funname);
}

class ResourceObj : public InterlockedObject {
	HMODULE m_hModule;
	HRSRC m_hRsrc;
	HGLOBAL m_hglbResource;
	void* m_p;
public:
	ResourceObj(HMODULE hModule, HRSRC hRsrc);
	~ResourceObj();
private:

	friend class Resource;
};
#endif // UCFG_WIN32

class Resource {
	typedef Resource class_type;

#if UCFG_WIN32
	ptr<ResourceObj> m_pimpl;
#else
	Blob m_blob;
#endif
public:
	Resource(const CResID& resID, const CResID& resType, HMODULE hModule = 0);

	const uint8_t *data() const;
	size_t size() const;
private:
};



#if UCFG_COM

const int COINIT_APARTMENTTHREADED  = 0x2;

class AFX_CLASS CUsingCOM {
	CDynamicLibrary m_dllOle;
	bool m_bInitialized;
public:
	CUsingCOM(DWORD dwCoInit = COINIT_APARTMENTTHREADED);
	CUsingCOM(_NoInit);
	~CUsingCOM();
	void Initialize(DWORD dwCoInit = COINIT_APARTMENTTHREADED);
	void Uninitialize();
};

#endif



#if !UCFG_USE_PTHREADS

struct CTimesInfo {
	FILETIME m_tmCreation,
		m_tmExit,
		m_tmKernel,
		m_tmUser;
};

#endif


#ifdef WIN32

//!!!O
struct CFileStatus {
	path AbsolutePath; // absolute path name
	DateTime m_ctime;          // creation date/time of file
	DateTime m_mtime;          // last modification date/time of file
	DateTime m_atime;          // last access date/time of file
	int64_t m_size;            // logical size of file in bytes
	uint8_t m_attribute;	   // logical OR of File::Attribute enum values
	uint8_t _m_padding;		   // pad the structure to a WORD
};
#endif

ENUM_CLASS(FileMode) {
	CreateNew
	, Create
	, Open
	, OpenOrCreate
	, Truncate
	, Append
} END_ENUM_CLASS(FileMode);

ENUM_CLASS(FileAccess) {
	Read		= 1
	, Write		= 2
	, ReadWrite = 3,
} END_ENUM_CLASS(FileAccess);

ENUM_CLASS(FileShare) {
	None		= 0
	, Read		= 1
	, Write		= 2
	, ReadWrite = 3
	, Delete	= 4
	, Inheritable	= 8
} END_ENUM_CLASS(FileShare);

ENUM_CLASS(FileOptions) {
	None = 0
	, Asynchronous	= 1
	, DeleteOnClose = 2
	, Encrypted		= 4
	, RandomAccess	= 8
	, SequentialScan = 16
	, WriteThrough	= 32
} END_ENUM_CLASS(FileOptions);

class AFX_CLASS File : public SafeHandle {
public:
	typedef File class_type;

	enum Attribute {
		normal =    0x00,
		readOnly =  0x01,
		hidden =    0x02,
		system =    0x04,
		volume =    0x08,
		directory = 0x10,
		archive =   0x20
	};


	EXT_DATA static bool s_bCreateFileWorksWithMMF;
	CBool m_bFileForMapping;

	File();

	File(intptr_t h, bool bOwn) {
		Attach(h, bOwn);
	}

	File(const path& p, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::Read, FileOptions options = FileOptions::None) {
		Open(p, mode, access, share, options);
	}

	~File();

	static Blob AFXAPI ReadAllBytes(const path& p);
	static void AFXAPI WriteAllBytes(const path& p, RCSpan mb);

	static String AFXAPI ReadAllText(const path& p, Encoding *enc = &Encoding::UTF8);
	static void AFXAPI WriteAllText(const path& p, RCString contents, Encoding *enc = &Encoding::UTF8);

	struct OpenInfo {
		path Path;
		FileMode Mode;
		FileAccess Access;
		FileShare Share;
		FileOptions Options;
		bool BufferingEnabled;

		OpenInfo(const path& p = path())
			: Path(p)
			, Mode(FileMode::Open)
			, Access(FileAccess::ReadWrite)
			, Share(FileShare::None)
			, Options(FileOptions::None)
			, BufferingEnabled(true)
		{}
	};

	void Open(const OpenInfo& oi);
	EXT_API virtual void Open(const path& p, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::None, FileOptions options = FileOptions::None);
#ifdef WIN32
	void Create(const path& fileName, DWORD dwDesiredAccess = GENERIC_READ|GENERIC_WRITE, DWORD dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE, DWORD dwCreationDisposition = CREATE_NEW, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL, HANDLE hTemplateFile = 0, LPSECURITY_ATTRIBUTES lpsa = 0);
	void CreateForMapping(LPCTSTR lpFileName, DWORD dwDesiredAccess = GENERIC_READ|GENERIC_WRITE, DWORD dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE, DWORD dwCreationDisposition = CREATE_NEW, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL, HANDLE hTemplateFile = 0, LPSECURITY_ATTRIBUTES lpsa = 0);
	DWORD GetOverlappedResult(OVERLAPPED& ov, bool bWait = true);
//!!!R	static bool AFXAPI GetStatus(RCString lpszFileName, CFileStatus& rStatus);
	bool Read(void *buf, uint32_t len, uint32_t *read, OVERLAPPED *pov);			// no default args to eleminate ambiguosness
	bool Write(const void *buf, uint32_t len, uint32_t *written, OVERLAPPED *pov);	// no default args to eleminate ambiguosness
	bool DeviceIoControl(int code, LPCVOID bufIn, size_t nIn, LPVOID bufOut, size_t nOut, LPDWORD pdw, LPOVERLAPPED pov = 0);
	DWORD DeviceIoControlAndWait(int code, LPCVOID bufIn = 0, size_t nIn = 0, LPVOID bufOut = 0, size_t nOut = 0);
	void CancelIo();
#endif

	bool get_CanSeek();
	DEFPROP_GET(bool, CanSeek);

	EXT_API int64_t Seek(const int64_t& off, SeekOrigin origin = SeekOrigin::Begin);
	int64_t SeekToEnd() { return Seek(0, SeekOrigin::End); }
	void Flush();
	void SetEndOfFile();

	uint64_t get_Length() const;
	void put_Length(uint64_t len);
	DEFPROP_CONST(uint64_t, Length);

	static const int64_t CURRENT_OFFSET = -2;

	virtual void Write(const void *buf, size_t size, int64_t offset = CURRENT_OFFSET);
	virtual uint32_t Read(void *buf, size_t size, int64_t offset = CURRENT_OFFSET);
	void Lock(uint64_t pos, uint64_t len, bool bExclusive = true, bool bFailImmediately = false);
	void Unlock(uint64_t pos, uint64_t len);
	size_t PhysicalSectorSize() const;			// return 0 if not detected

	//!!!  static void Remove(LPCTSTR lpszFileName);
	//!!!  static void Rename(LPCTSTR oldName, LPCTSTR newName);
	//!!!D  static void Copy(LPCTSTR existingFile, LPCTSTR newFile, bool bFailIfExists = false);
	//!!!D  static void RemoveDirectory(RCString dir, bool bRecurse = false);
private:
#ifdef WIN32
	bool CheckPending(BOOL b);
	OVERLAPPED *SetOffsetForFileOp(OVERLAPPED& ov, int64_t offset);
#endif
};

class AFX_CLASS CStdioFile : public File {
public:
//!!!R	bool ReadString(String& rString);
};

ENUM_CLASS(MemoryMappedFileAccess) {
	None 					= 0
	, CopyOnWrite 			= 1
	, Write 				= 2
	, Read 					= 4
	, ReadWrite 			= Read|Write
	, Execute 				= 8
	, ReadExecute 			= Read | Execute
	, ReadWriteExecute 		= Read | Write | Execute
} END_ENUM_CLASS(MemoryMappedFileAccess);

ENUM_CLASS(MemoryMappedFileRights) {
	CopyOnWrite 			= 1
	, Write 				= 2
	, Read 					= 4
	, ReadWrite 			= Read|Write
	, Execute 				= 8
	, ReadExecute 			= Read | Execute
	, ReadWriteExecute 		= Read | Write | Execute
	, Delete 				= 0x10000
	, ReadPermissions 		= 0x20000
	, ChangePermissions 	= 0x40000
	, TakeOwnership 		= 0x80000
	, FullControl 			= CopyOnWrite | ReadWriteExecute | Delete | ReadPermissions | ChangePermissions | TakeOwnership
	, AccessSystemSecurity 	= 0x1000000
} END_ENUM_CLASS(MemoryMappedFileRights);

class VirtualMem : noncopyable {
	void* m_address;
	size_t m_size;
public:
	VirtualMem(size_t size = 0, MemoryMappedFileAccess access = MemoryMappedFileAccess::ReadWrite, bool bLargePage = false)
		: m_address(0)
		, m_size(0) {
		if (size)
			Alloc(size, access, bLargePage);
	}

	~VirtualMem() {
		Free();
	}

	void Alloc(size_t size, MemoryMappedFileAccess access = MemoryMappedFileAccess::ReadWrite, bool bLargePage = false);
	void Free();

	void *get() { return m_address; }
	size_t size() { return m_size; }
	void *Detach() { return exchange(m_address, nullptr); }

	void Attach(void *a, size_t size) {
		if (m_address)
			Throw(E_FAIL);
		m_address = a;
		m_size = size;
	}
};

class MemoryMappedFile ;

class MemoryMappedView {
public:
	uint64_t Offset;
	void *Address;
	size_t Size;
	MemoryMappedFileAccess Access;
	bool AddressFixed, LargePages;

	MemoryMappedView()
		: Offset(0)
		, Address(0)
		, Size(0)
		, Access(MemoryMappedFileAccess::ReadWrite)
		, AddressFixed(false)
		, LargePages(false)
	{
	}

	~MemoryMappedView() {
		Unmap();
	}

	MemoryMappedView(const MemoryMappedView& v)
		: Offset(0)
		, Address(0)
		, Size(0)
		, Access(MemoryMappedFileAccess::ReadWrite)
		, AddressFixed(false)
	{
		if (v.Address)
			Throw(E_FAIL);
	}

	MemoryMappedView(EXT_RV_REF(MemoryMappedView) rv);

	MemoryMappedView& operator=(const MemoryMappedView& v) {
		Throw(E_FAIL);
	}

	MemoryMappedView& operator=(EXT_RV_REF(MemoryMappedView) rv);
	void Map(MemoryMappedFile& file, uint64_t offset, size_t size, void *desiredAddress = 0);
	void Unmap();
	void Flush();

	static void AFXAPI Protect(void *p, size_t size, MemoryMappedFileAccess access);
};

class MemoryMappedFile {
	typedef MemoryMappedFile class_type;

public:
	SafeHandle m_hMapFile;
	MemoryMappedFileAccess Access;

	MemoryMappedFile()
		: Access(MemoryMappedFileAccess::None)
	{
	}

	MemoryMappedFile(EXT_RV_REF(MemoryMappedFile) rv)
		: m_hMapFile(static_cast<EXT_RV_REF(SafeHandle)>(rv.m_hMapFile))
		, Access(rv.Access)
	{}

	void Close() {
		m_hMapFile.Close();
	}

	MemoryMappedFile& operator=(EXT_RV_REF(MemoryMappedFile) rv) {
		m_hMapFile.Close();
		m_hMapFile = static_cast<EXT_RV_REF(SafeHandle)>(rv.m_hMapFile);
		Access = rv.Access;
		return *this;
	}

#if UCFG_USE_POSIX
	Ext::File *m_pFile;

	intptr_t GetHandle() { return (intptr_t)m_pFile->DangerousGetHandle(); }
#else
	intptr_t GetHandle() { return m_hMapFile.DangerousGetHandle(); }
#endif

	static MemoryMappedFile AFXAPI CreateFromFile(Ext::File& file, RCString mapName = nullptr, uint64_t capacity = 0, MemoryMappedFileAccess access = MemoryMappedFileAccess::ReadWrite, bool bLargePages = false);
	static MemoryMappedFile AFXAPI CreateFromFile(const path& p, FileMode mode = FileMode::Open, RCString mapName = nullptr, uint64_t capacity = 0, MemoryMappedFileAccess access = MemoryMappedFileAccess::ReadWrite);
	static MemoryMappedFile AFXAPI OpenExisting(RCString mapName, MemoryMappedFileRights rights = MemoryMappedFileRights::ReadWrite, HandleInheritability inheritability = HandleInheritability::None);

	MemoryMappedView CreateView(uint64_t offset, size_t size, MemoryMappedFileAccess access, bool bLargePages = false);
	MemoryMappedView CreateView(uint64_t offset, size_t size = 0) { return CreateView(offset, size, Access); }
};

class FileStream : public Stream {
	typedef FileStream class_type;
public:
//!!!	mutable File m_ownFile;
	observer_ptr<File> m_pFile;
	mutable FILE *m_fstm;

#if UCFG_WIN32_FULL
	observer_ptr<OVERLAPPED> m_ovl;
#endif
	CBool TextMode;

	FileStream()
		: m_fstm(0)
	{
	}

	FileStream(Ext::File& file
#if UCFG_WIN32_FULL
, OVERLAPPED *ovl = nullptr
#endif
)
		: m_pFile(&file)
		, m_fstm(0)
#if UCFG_WIN32_FULL
		, m_ovl(ovl)
#endif
	{
	}

	FileStream(const path& p, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::Read, size_t bufferSize = 4096, FileOptions options = FileOptions::None)
		: m_fstm(0)
	{
		Open(p, mode, access, share, bufferSize, options);
	}

	~FileStream() {
		if (m_fstm)
			Close();
	}

	EXT_API void Open(const path& p, FileMode mode, FileAccess access = FileAccess::ReadWrite, FileShare share = FileShare::None, size_t bufferSize = 4096, FileOptions options = FileOptions::None);
	size_t Read(void *buf, size_t count) const override;
	void ReadBuffer(void *buf, size_t count) const override;
	void WriteBuffer(const void *buf, size_t count) override;
	void Close() const override;
	void Flush() override;

	intptr_t get_Handle() const;
	DEFPROP_GET(intptr_t, Handle);

	uint64_t get_Position() const override {
		if (m_fstm) {
			fpos_t r;
			CFileCheck(fgetpos(m_fstm, &r));
#if defined(_WIN32) || defined(__FreeBSD__)
			return r;
#else
			return r.__pos;
#endif
		} else if (m_pFile)
			return m_pFile->Seek(0, SeekOrigin::Current);
		else
			Throw(E_FAIL);
	}

	void put_Position(uint64_t pos) const override {
		if (m_fstm) {
			fpos_t fpos;
#if defined(_WIN32) || defined(__FreeBSD__)
			fpos = pos;
#else
			CFileCheck(fgetpos(m_fstm, &fpos));
			fpos.__pos = pos;
#endif
			CFileCheck(fsetpos(m_fstm, &fpos));
		} else if (m_pFile)
			m_pFile->Seek(pos, SeekOrigin::Begin);
		else
			Throw(E_FAIL);
	}

	int64_t Seek(int64_t offset, SeekOrigin origin) const override {
		if (m_fstm) {
			CCheck(fseek(m_fstm, (long)offset, (int)origin));	//!!!
			return Position;
		} else if (m_pFile)
			return m_pFile->Seek(offset, origin);
		else
			Throw(E_FAIL);
	}

	uint64_t get_Length() const override;
	bool Eof() const override;
};

class PositionOwningFileStream : public FileStream {
	typedef FileStream base;
protected:
	mutable uint64_t m_pos;
	uint64_t m_maxPos;
public:
	PositionOwningFileStream(Ext::File& file, uint64_t pos = 0, uint64_t maxLen = _UI64_MAX);
	uint64_t get_Position() const override { return m_pos; }
	void put_Position(uint64_t pos) const override { m_pos = pos; }
	bool Eof() const override {	return m_pos == m_pFile->Length; }

	int64_t Seek(int64_t offset, SeekOrigin origin) const override {
		switch (origin) {
		case SeekOrigin::Begin: m_pos = offset; break;
		case SeekOrigin::End: m_pos = m_pFile->Length; break;
		case SeekOrigin::Current: m_pos += offset; break;
		}
		return m_pos;
	}

	size_t Read(void *buf, size_t count) const override;
	void ReadBuffer(void *buf, size_t count) const override;
	void WriteBuffer(const void *buf, size_t count) override;
};

class TraceStream : public FileStream {
	typedef FileStream base;
protected:
	File m_file;
public:
	TraceStream(const path& p, bool bAppend = false);
};

class CycledTraceStream : public TraceStream {
	typedef TraceStream base;

	path m_path;
	std::shared_mutex m_mtx;
	size_t m_maxSize, m_threshold;
public:
	CycledTraceStream(const path& p, bool bAppend = false, size_t maxSize = 10000000)
		: base(p, bAppend)
		, m_path(p)
		, m_maxSize(maxSize)
		, m_threshold(maxSize * 11 / 10)
	{}

	void WriteBuffer(const void *buf, size_t count) override;
};

class Guid : public GUID {
public:
	Guid() { ZeroStruct(*this); }
	Guid(const GUID& guid) { *this = guid; }
	explicit Guid(RCString s);

	static Guid AFXAPI NewGuid();

	Guid& operator=(const GUID& guid) {
		*(GUID*)this = guid;
		return *this;
	}

	bool operator==(const GUID& guid) const { return !memcmp(this, &guid, sizeof(GUID)); }

	String ToString(RCString format = nullptr) const;
};

inline BinaryWriter& operator<<(BinaryWriter& wr, const Guid& guid) {
	return wr.WriteStruct(guid);
}

inline const BinaryReader& operator>>(const BinaryReader& rd, Guid& guid) {
	return rd.ReadStruct(guid);
}


inline bool operator<(const GUID& guid1, const GUID& guid2) {
	return memcmp(&guid1, &guid2, sizeof(GUID)) < 0;
}


} // Ext::

namespace EXT_HASH_VALUE_NS {
inline size_t hash_value(const Ext::Guid& guid) {
	return Ext::hash_value((const uint8_t*)&guid, sizeof(GUID));
}
}

EXT_DEF_HASH(Ext::Guid)

//!!!#if !UCFG_STDSTL
//!!!using namespace _STL_NAMESPACE;
//!!!#endif

#	include "ext-thread.h"

namespace Ext {

class Version : public CPrintable, totally_ordered<Version> {
public:
	int Major, Minor, Build, Revision;

	explicit Version(int major = 0, int minor = 0, int build = -1, int revision = -1)
		: Major(major)
		, Minor(minor)
		, Build(build)
		, Revision(revision)
	{}

	explicit Version(RCString s);

#ifdef WIN32
	static Version AFXAPI FromFileInfo(int ms, int ls, int fieldCount = 4);
#endif

	bool operator==(const Version& x) const {
		return Major == x.Major && Minor == x.Minor && Build == x.Build && Revision == x.Revision;
	}

	bool operator<(const Version& x) const {
		return Major < x.Major
			|| Major == x.Major
				&& (Minor < x.Minor || Minor == x.Minor && (Build < x.Build || Build == x.Build && Revision < x.Revision));
	}

	String ToString(int fieldCount) const;
	String ToString() const override;
};


class VersionException : public Exception {
	typedef Exception base;
public:
	Ext::Version Version;

	VersionException(const Ext::Version& ver = Ext::Version()) : base(ExtErr::DB_Version), Version(ver) {}

	~VersionException() noexcept {} //!!! GCC 4.6
protected:
	String get_Message() const override { return base::get_Message() + " " + Version.ToString(2); }
};

class UnsupportedOldVersionException : public VersionException {
	typedef VersionException base;
public:
	UnsupportedOldVersionException(const Ext::Version& ver = Ext::Version()) : base(ver) {}
};

class UnsupportedNewVersionException : public VersionException {
	typedef VersionException base;
public:
	UnsupportedNewVersionException(const Ext::Version& ver = Ext::Version()) : base(ver) {}
};



} // Ext::

#ifdef WIN32

//#	include "win32/ext-win.h"
//#	include "extwin32.h"
#else
#	define AFX_MANAGE_STATE(p)
#endif

namespace Ext {

AFX_API int AFXAPI Rand();

class EXTAPI Random : noncopyable {
public:
	void *m_prngeng;

	Random(int seed = Rand());
	~Random();

	virtual void NextBytes(const span<uint8_t>& mb);
	int Next();
	int Next(int maxValue);
	double NextDouble();
private:
	uint16_t NextWord();
};


BinaryReader& AFXAPI GetSystemURandomReader();


struct IAnnoy {
	virtual void OnAnnoy() =0;
};

class EXTAPI CAnnoyer {
	observer_ptr<IAnnoy> m_iAnnoy;
	DateTime m_prev;
	TimeSpan m_period;
public:
	CAnnoyer(IAnnoy *iAnnoy = 0)
		: m_iAnnoy(iAnnoy)
		, m_period(TimeSpan::FromSeconds(1))
	{
	}

	void Request();
protected:
	virtual void OnAnnoy();
};

#if !UCFG_WCE
template <class T> class CSubscriber {
public:
	typedef std::unordered_set<T*> CSet;
	CSet m_set;

	void operator+=(T *v) {
		m_set.insert(v);
	}

	void operator-=(T *v) {
		m_set.erase(v);
	}
};
#endif


#if !UCFG_WCE

class CDirectoryKeeper {													//!!! Non Thread-safe
	path m_prevCurPath;
public:
	CDirectoryKeeper(const path& p)
		: m_prevCurPath(current_path())
	{
		current_path(p);
	}

	~CDirectoryKeeper() {
		current_path(m_prevCurPath);
	}
};

#endif // !UCFG_WCE

inline path ToPath(RCString s) {
	return wstring(explicit_cast<wstring>(s));
}

class Path {
public:
	struct CSplitPath {
		String m_drive,
			m_dir,
			m_fname,
			m_ext;
	};

#ifdef _WIN32				//	for WDM too
	static const char AltDirectorySeparatorChar = '/';
	static const char VolumeSeparatorChar = ':';
	static const char PathSeparator = ';';
#else
	static const char AltDirectorySeparatorChar = '/';
	static const char VolummeSeparatorChar = '/';
	static const char PathSeparator = ':';
#endif

	EXT_API static std::pair<path, UINT> AFXAPI GetTempFileName(const path& p, RCString prefix, UINT uUnique = 0);
	EXT_API static path AFXAPI GetTempFileName() { return GetTempFileName(temp_directory_path(), "tmp").first; }

	static CSplitPath AFXAPI SplitPath(const path&p );

	static path AFXAPI GetPhysicalPath(const path& p);
	static path AFXAPI GetTruePath(const path& p);
};

path AFXAPI AddDirSeparator(const path& p);
//!!!RAFX_API String AFXAPI RemoveDirSeparator(RCString s, bool bOnEndOnly = false);


class FileSystemInfo {
	typedef FileSystemInfo class_type;
public:
	path FullPath;
	bool m_bDir;

	FileSystemInfo(const path& name, bool bDir)
		: FullPath(name)
		, m_bDir(bDir)
	{}

	DWORD get_Attributes() const;
	DEFPROP_GET(DWORD, Attributes);

	DateTime get_CreationTime() const;
	void put_CreationTime(const DateTime& dt);
	DEFPROP_CONST(DateTime, CreationTime);

	DateTime get_LastAccessTime() const;
	void put_LastAccessTime(const DateTime& dt);
	DEFPROP_CONST(DateTime, LastAccessTime);

	DateTime get_LastWriteTime() const;
	void put_LastWriteTime(const DateTime& dt);
	DEFPROP_CONST(DateTime, LastWriteTime);
protected:
#ifdef WIN32
	EXT_API WIN32_FIND_DATA GetData() const;
#endif
};

#if UCFG_WIN32_FULL

class SerialPort {
public:
	EXT_API static std::vector<String> AFXAPI GetPortNames();
};

#endif // UCFG_WIN32_FULL

ENUM_CLASS(Architecture) {
	X86
	, X64
	, Arm
	, Arm64
	, MIPS
	, MIPS64
	, IA64
	, SHX
	, Unknown = 255
} END_ENUM_CLASS(Architecture);

class OSPlatform {
public:
	static const OSPlatform Windows, Linux, OSX, Unix;

	String Name;

	OSPlatform(RCString name)
		: Name(name)
	{}

	bool operator==(const OSPlatform& o) const { return Name == o.Name; }
};

class RuntimeInformation {
public:
	static bool IsOSPlatform(const OSPlatform& platform);
	static Architecture OSArchitecture();
};

ENUM_CLASS(PlatformID) {	//!!!Obsolete
	Win32S
	, Win32Windows
	, Win32NT
	, WinCE
	, Unix
	, XBox
} END_ENUM_CLASS(PlatformID);

class OperatingSystem : public CPrintable {
	typedef OperatingSystem class_type;
public:
	PlatformID Platform;
	Ext::Version Version;
	String ServicePack;

	OperatingSystem();

	String get_PlatformName() const;
	DEFPROP_GET(String, PlatformName);

	String get_VersionName() const;
	DEFPROP_GET(String, VersionName);

	String get_VersionString() const;
	DEFPROP_GET(String, VersionString);

	String ToString() const override {
		return VersionString;
	}
};

#define CSIDL_DESKTOP                   0x0000        // <desktop>
#define CSIDL_INTERNET                  0x0001        // Internet Explorer (icon on desktop)
#define CSIDL_PROGRAMS                  0x0002        // Start Menu\Programs
#define CSIDL_CONTROLS                  0x0003        // My Computer\Control Panel
#define CSIDL_PRINTERS                  0x0004        // My Computer\Printers
#define CSIDL_PERSONAL                  0x0005        // My Documents
#define CSIDL_FAVORITES                 0x0006        // <user name>\Favorites
#define CSIDL_STARTUP                   0x0007        // Start Menu\Programs\Startup
#define CSIDL_RECENT                    0x0008        // <user name>\Recent
#define CSIDL_SENDTO                    0x0009        // <user name>\SendTo
#define CSIDL_BITBUCKET                 0x000a        // <desktop>\Recycle Bin
#define CSIDL_STARTMENU                 0x000b        // <user name>\Start Menu
#define CSIDL_MYDOCUMENTS               CSIDL_PERSONAL //  Personal was just a silly name for My Documents
#define CSIDL_MYMUSIC                   0x000d        // "My Music" folder
#define CSIDL_MYVIDEO                   0x000e        // "My Videos" folder
#define CSIDL_DESKTOPDIRECTORY          0x0010        // <user name>\Desktop
#define CSIDL_DRIVES                    0x0011        // My Computer
#define CSIDL_NETWORK                   0x0012        // Network Neighborhood (My Network Places)
#define CSIDL_NETHOOD                   0x0013        // <user name>\nethood
#define CSIDL_FONTS                     0x0014        // windows\fonts
#define CSIDL_TEMPLATES                 0x0015
#define CSIDL_COMMON_STARTMENU          0x0016        // All Users\Start Menu
#define CSIDL_COMMON_PROGRAMS           0X0017        // All Users\Start Menu\Programs
#define CSIDL_COMMON_STARTUP            0x0018        // All Users\Startup
#define CSIDL_COMMON_DESKTOPDIRECTORY   0x0019        // All Users\Desktop
#define CSIDL_APPDATA                   0x001a        // <user name>\Application Data
#define CSIDL_PRINTHOOD                 0x001b        // <user name>\PrintHood

#define CSIDL_PROGRAM_FILES             0x0026        // C:\Program Files
#define CSIDL_MYPICTURES                0x0027        // C:\Program Files\My Pictures
#define CSIDL_PROFILE                   0x0028

ENUM_CLASS(SpecialFolder) {
#ifdef CSIDL_DESKTOP
#ifdef CSIDL_APPDATA
	ApplicationData			= CSIDL_APPDATA,
#endif
#ifdef CSIDL_COMMON_APPDATA
	CommonApplicationData	= CSIDL_COMMON_APPDATA,
#endif
#ifdef CSIDL_PROGRAM_FILES_COMMON
	CommonProgramFiles		= CSIDL_PROGRAM_FILES_COMMON,
#endif
#ifdef CSIDL_COOKIES
	Cookies					= CSIDL_COOKIES,
#endif
	Desktop					= CSIDL_DESKTOP,
	DesktopDirectory		= CSIDL_DESKTOPDIRECTORY,
	Favorites				= CSIDL_FAVORITES,
#ifdef CSIDL_HISTORY
	History					= CSIDL_HISTORY,
#endif
#ifdef CSIDL_INTERNET_CACHE
	InternetCache			= CSIDL_INTERNET_CACHE,
#endif
#ifdef CSIDL_LOCAL_APPDATA
	LocalApplicationData	= CSIDL_LOCAL_APPDATA,
#endif
	MyComputer				= CSIDL_DRIVES,
#ifdef CSIDL_MYDOCUMENTS
	MyDocuments				= CSIDL_MYDOCUMENTS,
#endif
	MyMusic					= CSIDL_MYMUSIC,
	MyPictures				= CSIDL_MYPICTURES,
	Personal				= CSIDL_PERSONAL,
	ProgramFiles			= CSIDL_PROGRAM_FILES,
	Programs				= CSIDL_PROGRAMS,
	Recent					= CSIDL_RECENT,
	SendTo					= CSIDL_SENDTO,
	StartMenu				= CSIDL_STARTMENU,
	Startup					= CSIDL_STARTUP,
#ifdef CSIDL_SYSTEM
	System					= CSIDL_SYSTEM,
#endif
	Templates				= CSIDL_TEMPLATES,
	UserProfile				= CSIDL_PROFILE,

#endif // CSIDL_DESKTOP

	Downloads				= 257,			// Non-standard
} END_ENUM_CLASS(SpecialFolder);


std::vector<String> ParseCommandLine(RCString s);

class AFX_CLASS Environment {
	typedef Environment class_type;
public:
	EXT_DATA static int ExitCode;
	EXT_DATA static const OperatingSystem OSVersion;

#if UCFG_WIN32
	static int32_t AFXAPI TickCount();
#endif // UCFG_WIN32

#if UCFG_WIN32_FULL
	class CStringsKeeper {
	public:
		LPTSTR m_p;

		CStringsKeeper();
		~CStringsKeeper();
	};

	static uint32_t AFXAPI GetLastInputInfo();
#endif

	static String AFXAPI CommandLine();
#if !UCFG_WCE
	EXT_API static vector<String> AFXAPI GetCommandLineArgs();
	EXT_API static String AFXAPI GetEnvironmentVariable(RCString s);
	EXT_API static void AFXAPI SetEnvironmentVariable(RCString name, RCString val);
	AFX_API static String AFXAPI ExpandEnvironmentVariables(RCString name);
	EXT_API static map<String, String> AFXAPI GetEnvironmentVariables();
#endif

	static path AFXAPI SystemDirectory();
	EXT_API static path AFXAPI GetFolderPath(SpecialFolder folder);
	static String AFXAPI GetMachineType();
	static String AFXAPI GetMachineVersion();
	static bool AFXAPI Is64BitProcess() { return sizeof(void*) == 8; }
	static bool AFXAPI Is64BitOperatingSystem();

	int get_ProcessorCount();
	DEFPROP_GET(int, ProcessorCount);

	int get_ProcessorCoreCount();
	DEFPROP_GET(int, ProcessorCoreCount);
};

extern EXT_DATA Ext::Environment Environment;

bool AFXAPI IsConsole();

struct CaseInsensitiveStringLess : std::less<String> {
	bool operator()(RCString a, RCString b) const {
		return a.CompareNoCase(b) < 0;
	}
};

struct CaseInsensitiveHash {
	size_t operator()(RCString s) const {
		return std::hash<String>()(s.ToUpper());
	}
};

struct CaseInsensitiveEqual {
	size_t operator()(RCString a, RCString b) const {
		return a.CompareNoCase(b) == 0;
	}
};

class NameValueCollection : public std::map<String, CStringVector, CaseInsensitiveStringLess>, public CPrintable {
	typedef std::map<String, CStringVector, CaseInsensitiveStringLess> base;
public:
	CStringVector GetValues(RCString key) const {
		CStringVector r;
		base::const_iterator i = find(key);
		return i != end() ? i->second : CStringVector();
	}

	CStringVector& GetRef(RCString key) {
		return base::operator[](key);
	}

	String Get(RCString key) const {
		return find(key) != end() ? String::Join(",", GetValues(key)) : nullptr;
	}

	String operator[](RCString key) const { return Get(key); }

	void Set(RCString name, RCString v) {
		CStringVector ar;
		ar.push_back(v);
		base::operator[](name) = ar;
	}

	String ToString() const;
};

class WebHeaderCollection : public NameValueCollection {
};

class HttpUtility {
public:
	static String AFXAPI UrlEncode(RCString s, Encoding& enc = Encoding::UTF8);
	static String AFXAPI UrlDecode(RCString s, Encoding& enc = Encoding::UTF8);

	static NameValueCollection AFXAPI ParseQueryString(RCString query);
};


typedef std::unordered_map<UINT, const char*> CMapStringRes;
CMapStringRes& MapStringRes();

typedef vararray<uint8_t, 64> hashval;

class HashAlgorithm {
public:
	static const unsigned WordCount = 16;

	size_t BlockSize,
		HashSize;
	bool IsHaifa, IsBigEndian, IsLenBigEndian, IsBlockCounted;
protected:
	bool Is64Bit;
public:
	HashAlgorithm();
	virtual ~HashAlgorithm() {}
	virtual hashval ComputeHash(Stream& stm);				//!!!TODO should be const
	virtual hashval ComputeHash(RCSpan mb);

	virtual void InitHash(void *dst) noexcept {}
	void PrepareEndianness(void *dst, int count) noexcept;
	virtual void HashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept {}
	virtual void PrepareEndiannessAndHashBlock(void* dst, uint8_t src[256], uint64_t counter) noexcept;
	virtual void OutTransform(void *dst) noexcept {}
protected:
	hashval Finalize(void *hash, Stream& stm, uint64_t processedLen);
};

hashval HMAC(HashAlgorithm& halgo, RCSpan key, RCSpan text);

class Crc32 : public HashAlgorithm {
	typedef HashAlgorithm base;
public:
	using base::ComputeHash;
	hashval ComputeHash(Stream& stm) override;
};

extern EXT_DATA std::mutex g_mfcCS;

#if UCFG_USE_POSIX

class MessageCatalog {
	nl_catd m_catd;
public:
	MessageCatalog()
		: m_catd(nl_catd(-1))
	{
	}

	~MessageCatalog() {
		if (_self)
			CCheck(::catclose(m_catd));
	}

	EXPLICIT_OPERATOR_BOOL() const {
		return m_catd != nl_catd(-1) ? EXT_CONVERTIBLE_TO_TRUE : 0;
	}

	void Open(const char *name, int oflag = 0) {
		CCheck((m_catd = ::catopen(name, oflag))==(nl_catd)-1 ? -1 : 0);
	}

	String GetMessage(int set_id, int msg_id, const char *s = 0) {
		return ::catgets(m_catd, set_id, msg_id, s);
	}
};


#endif // UCFG_USE_POSIX


class AFX_CLASS CMessageRange {
public:
	CMessageRange();
	virtual ~CMessageRange();
	virtual String CheckMessage(DWORD code) { return ""; }
};

class EXTAPI CMessageProcessor : noncopyable {
	struct CModuleInfo {
		uint32_t m_lowerCode, m_upperCode;
		path m_moduleName;
#if UCFG_USE_POSIX
		MessageCatalog m_mcat;
		CBool m_bCheckedOpen;
#endif
		void Init(uint32_t lowerCode, uint32_t upperCode, const path& moduleName);
		String GetMessage(HRESULT hr);
	};

	std::vector<CModuleInfo> m_vec;
	CModuleInfo m_default;

	void CheckStandard();
public:
	std::vector<CMessageRange*> m_ranges;

	CMessageProcessor();
	static void AFXAPI RegisterModule(DWORD lowerCode, DWORD upperCode, RCString moduleName);
#if UCFG_COM
	static HRESULT AFXAPI Process(HRESULT hr, EXCEPINFO *pexcepinfo);
#endif
	String ProcessInst(HRESULT hr, bool bWithErrorCode = true);

//!!!R	thread_specific_ptr<String> m_param;

//!!!R	static void AFXAPI SetParam(RCString s);
};

extern CMessageProcessor g_messageProcessor;

AFX_API String AFXAPI HResultToMessage(HRESULT hr, bool bWithErrorCode = true);
#if UCFG_COM
AFX_API HRESULT AFXAPI AfxProcessError(HRESULT hr, EXCEPINFO *pexcepinfo);
#endif

ENUM_CLASS(VarType) {
	Null
	, Int
	, Float
	, Bool
	, String
	, Array
	, Map
} END_ENUM_CLASS(VarType);

class VarValue;

class VarValueObj : public InterlockedObject {
public:
	virtual bool HasKey(RCString key) const =0;
	virtual VarType type() const =0;
	virtual size_t size() const =0;
	virtual VarValue operator[](int idx) const =0;
	virtual VarValue operator[](RCString key) const =0;
	virtual String ToString() const =0;
	virtual int64_t ToInt64() const =0;
	virtual bool ToBool() const =0;
	virtual double ToDouble() const =0;
	virtual void Set(int idx, const VarValue& v) =0;
	virtual void Set(RCString key, const VarValue& v) =0;
	virtual std::vector<String> Keys() const =0;

	virtual void Print(std::ostream& os) const { Throw(E_NOTIMPL); }
};

class VarValue {
public:
	ptr<VarValueObj> m_pimpl;

	VarValue() {}

	VarValue(std::nullptr_t) {}

	VarValue(int64_t v);
	VarValue(int v);
	VarValue(double v);
	explicit VarValue(bool v);
	VarValue(RCString v);
	VarValue(const char *p);
	VarValue(const std::vector<VarValue>& ar);

	bool operator==(const VarValue& v) const;
	bool operator!=(const VarValue& v) const { return !operator==(v); }

	EXPLICIT_OPERATOR_BOOL() const {
		return m_pimpl ? EXT_CONVERTIBLE_TO_TRUE : 0;
	}

	bool HasKey(RCString key) const { return Impl().HasKey(key); }
	VarType type() const {
		return m_pimpl ? Impl().type() : VarType::Null;
	}
	size_t size() const { return Impl().size(); }
	VarValue operator[](int idx)  const { return Impl().operator[](idx); }
	VarValue operator[](RCString key) const { return Impl().operator[](key); }
	String ToString() const { return Impl().ToString(); }
	int64_t ToInt64() const { return Impl().ToInt64(); }
	bool ToBool() const { return Impl().ToBool(); }
	double ToDouble() const { return Impl().ToDouble(); }
	void Set(int idx, const VarValue& v);
	void Set(RCString key, const VarValue& v);
	void SetType(VarType typ);
	std::vector<String> Keys() const { return Impl().Keys(); }
	void Print(std::ostream& os) const { Impl().Print(os); }
private:
	template <typename T>
	VarValue(const T*);							// to prevent VarValue(bool) ctor for pointers

	VarValueObj& Impl() const {
		if (!m_pimpl)
			Throw(errc::invalid_argument);
		return *m_pimpl;
	}
};

inline std::ostream& operator<<(std::ostream& os, const VarValue& v) {
	v.Print(os);
	return os;
}

} // Ext::
namespace EXT_HASH_VALUE_NS {
	size_t hash_value(const Ext::VarValue& v);
}
EXT_DEF_HASH(Ext::VarValue)
namespace Ext {


class MarkupParser : public InterlockedObject {
public:
	int Indent;
	CBool Compact;

	MarkupParser()
		: Indent(0)
	{}

	virtual ~MarkupParser() {
	}

	virtual VarValue Parse(std::istream& is, Encoding *enc = &Encoding::UTF8) =0;
	virtual void Print(std::ostream& os, const VarValue& v) =0;

	virtual std::pair<VarValue, Blob> ParseStream(Stream& stm, RCSpan preBuf = Span()) { Throw(E_NOTIMPL); }

	VarValue Parse(RCString s) {
		istringstream iss(s.c_str());
		return Parse(iss);
	}

	static ptr<MarkupParser> AFXAPI CreateJsonParser();
};

VarValue AFXAPI ParseJson(RCString s);

template <class T>
struct PersistentTraits {
	static void Read(const BinaryReader& rd, T& v) {
		rd >> v;
	}

	static void Write(BinaryWriter& wr, const T& v) {
		wr << v;
	}
};

#ifndef UCFG_USE_PERSISTENT_CACHE
#	define UCFG_USE_PERSISTENT_CACHE 1
#endif

class PersistentCache {
public:
	template <class T>
	static bool TryGet(RCString key, T& val) {
#if UCFG_USE_PERSISTENT_CACHE
		Blob blob;
		if (!Lookup(key, blob))
			return false;
		CMemReadStream ms(blob);
		PersistentTraits<T>::Read(BinaryReader(ms), val);
		return true;
#else
		return false;
#endif
	}

	template <class T>
	static void Set(RCString key, const T& val) {
#if UCFG_USE_PERSISTENT_CACHE
		MemoryStream ms;
		BinaryWriter wr(ms);
		PersistentTraits<T>::Write(wr, val);
		Set(key, Span(ms.get_Blob()));
#endif
	}
private:
	static bool AFXAPI Lookup(RCString key, Blob& blob);
	static void AFXAPI Set(RCString key, RCSpan mb);
};

class Temperature : public CPrintable {
	float m_kelvin;				// float to be atomic
public:
	EXT_DATA static const Temperature Zero;

	Temperature()
		: m_kelvin(0)
	{}

	static Temperature FromKelvin(float v) { return Temperature(v); };
	static Temperature FromCelsius(float v) { return Temperature(float(v+273.15)); };
	static Temperature FromFahrenheit(float v) { return Temperature(float((v-32)*5/9+273.15)); };

	operator float() const { return m_kelvin; }
	float ToCelsius() const { return float(m_kelvin-273.15); }
	float ToFahrenheit() const { return float((m_kelvin-273.15)*1.8+32); }

	bool operator==(const Temperature& v) const { return m_kelvin == v.m_kelvin; }
	bool operator<(const Temperature& v) const { return m_kelvin < v.m_kelvin; }

	String ToString() const;
private:
	explicit Temperature(float kelvin)
		: m_kelvin(kelvin)
	{}
};

std::ostream& AFXAPI GetNullStream();
std::wostream& AFXAPI GetWNullStream();


extern EXT_API std::mutex g_mtxObjectCounter;
typedef std::unordered_map<const std::type_info*, int> CTiToCounter;
extern EXT_API CTiToCounter g_tiToCounter;

void AFXAPI LogObjectCounters(bool fFull = true);

template <class T>
class DbgObjectCounter {
public:
	DbgObjectCounter() {
		EXT_LOCK (g_mtxObjectCounter) {
			++g_tiToCounter[&typeid(T)];
		}
	}

	DbgObjectCounter(const DbgObjectCounter&) {
		EXT_LOCK (g_mtxObjectCounter) {
			++g_tiToCounter[&typeid(T)];
		}
	}

	~DbgObjectCounter() {
		EXT_LOCK (g_mtxObjectCounter) {
			--g_tiToCounter[&typeid(T)];
		}
	}
};

#ifdef _DEBUG
#	define DBG_OBJECT_COUNTER(typ) Ext::DbgObjectCounter<typ> _dbgObjectCounter;
#else
#	define DBG_OBJECT_COUNTER(typ)
#endif


struct ProcessStartInfo {
	map<String, String> EnvironmentVariables;
	path FileName,
		WorkingDirectory;
	String Arguments;
	uint32_t Flags;
	CBool CreateNewWindow,
		CreateNoWindow,
		RedirectStandardInput,
		RedirectStandardOutput,
		RedirectStandardError;

	ProcessStartInfo(const path& fileName = path(), RCString arguments = String());
};

class ProcessObj : public SafeHandle { //!!!
	typedef SafeHandle base;
public:
	typedef InterlockedPolicy interlocked_policy;

	ProcessStartInfo StartInfo;

#if !UCFG_WCE
protected:
	File m_fileIn, m_fileOut, m_fileErr;
public:
	FileStream StandardInput,
		StandardOutput,
		StandardError;
#endif
protected:
	int m_stat_loc;
	mutable CInt<pid_t> m_pid;
public:
	ProcessObj();
#if UCFG_WIN32
	ProcessObj(HANDLE handle, bool bOwn = false);
#endif

	DWORD get_ID() const;
	DWORD get_ExitCode() const;
	bool get_HasExited();
	bool Start();
	void Kill();
	void WaitForExit(DWORD ms = INFINITE);

#if UCFG_WIN32
	ProcessObj(pid_t pid, DWORD dwAccess = MAXIMUM_ALLOWED, bool bInherit = false);

	EXT_API std::unique_ptr<CWinThread> Create(RCString commandLine, DWORD dwFlags = 0, const char *dir = 0, bool bInherit = false, STARTUPINFO *psi = 0);

	void Terminate(DWORD dwExitCode)  {	Win32Check(::TerminateProcess((HANDLE)(intptr_t)HandleAccess(*this), dwExitCode)); }

	AFX_API CTimesInfo get_Times() const;
	DEFPROP_GET(CTimesInfo, Times);

	HWND get_MainWindowHandle() const;
	DEFPROP_GET(HWND, MainWindowHandle);

#if !UCFG_WCE
	AFX_API DWORD get_Version() const;
	DEFPROP_GET(DWORD, Version);

	bool get_IsWow64();
	DEFPROP_GET(bool, IsWow64);

	DWORD get_PriorityClass() { return Win32Check(::GetPriorityClass((HANDLE)(intptr_t)HandleAccess(*this))); }
	void put_PriorityClass(DWORD pc) { Win32Check(::SetPriorityClass((HANDLE)(intptr_t)HandleAccess(*this), pc)); }

	String get_MainModuleFileName();
#endif

	void FlushInstructionCache(LPCVOID base = 0, SIZE_T size = 0) { Win32Check(::FlushInstructionCache((HANDLE)(intptr_t)HandleAccess(*this), base, size)); }

	DWORD VirtualProtect(void *addr, size_t size, DWORD flNewProtect);
	MEMORY_BASIC_INFORMATION VirtualQuery(const void *addr);
	SIZE_T ReadMemory(LPCVOID base, LPVOID buf, SIZE_T size);
	SIZE_T WriteMemory(LPVOID base, LPCVOID buf, SIZE_T size);
#endif // UCFG_WIN32
protected:
	bool Valid() const override {
		return DangerousGetHandleEx() != 0;
	}

	void CommonInit();
};


class Process : public Pimpl<ProcessObj> {
	typedef Process class_type;
public:
	static Process AFXAPI GetProcessById(pid_t pid);
	static Process AFXAPI GetCurrentProcess();

	ProcessStartInfo& StartInfo() { return m_pimpl->StartInfo; }

	static Process AFXAPI Start(const ProcessStartInfo& psi);

#if !UCFG_WCE
	Stream& StandardInput() { return m_pimpl->StandardInput; }
	Stream& StandardOutput() { return m_pimpl->StandardOutput; }
	Stream& StandardError() { return m_pimpl->StandardError; }
#endif

	DWORD get_ID() const { return m_pimpl->get_ID(); }
	DEFPROP_GET(DWORD, ID);

	DWORD get_ExitCode() const { return m_pimpl->get_ExitCode(); }
	DEFPROP_GET(DWORD, ExitCode);

	void Kill() { m_pimpl->Kill(); }
	void WaitForExit(DWORD ms = INFINITE) { m_pimpl->WaitForExit(ms); }

	bool get_HasExited() { return m_pimpl->get_HasExited(); }
	DEFPROP_GET(bool, HasExited);

	String get_ProcessName();
	DEFPROP_GET(String, ProcessName);

	static std::vector<Process> AFXAPI GetProcesses();
	static std::vector<Process> AFXAPI GetProcessesByName(RCString name);

#if UCFG_WIN32
	static Process AFXAPI FromHandle(HANDLE h, bool bOwn);
	void FlushInstructionCache(LPCVOID base = 0, SIZE_T size = 0) { m_pimpl->FlushInstructionCache(base, size); }
	DWORD VirtualProtect(void *addr, size_t size, DWORD flNewProtect) { return m_pimpl->VirtualProtect(addr, size, flNewProtect); }
	MEMORY_BASIC_INFORMATION VirtualQuery(const void *addr) { return m_pimpl->VirtualQuery(addr); }
	SIZE_T ReadMemory(LPCVOID base, LPVOID buf, SIZE_T size) { return m_pimpl->ReadMemory(base, buf, size); }
	SIZE_T WriteMemory(LPVOID base, LPCVOID buf, SIZE_T size) { return m_pimpl->WriteMemory(base, buf, size); }
#endif

#if UCFG_WIN32_FULL
	DWORD get_PriorityClass() { return m_pimpl->get_PriorityClass(); }
	void put_PriorityClass(DWORD pc) { m_pimpl->put_PriorityClass(pc); }
	DEFPROP(DWORD, PriorityClass);

	void GenerateConsoleCtrlEvent(DWORD dwCtrlEvent = 1);		// CTRL_BREAK_EVENT==1

	String get_MainModuleFileName() { return m_pimpl->get_MainModuleFileName(); }
	DEFPROP_GET(String, MainModuleFileName);
#endif
};

class POpen : noncopyable {
	FILE* m_stream;
public:
	POpen(RCString command, RCString mode)
		: m_stream(::popen(command, mode))
	{
		CCheck(m_stream ? 1 : -1);
	}

	~POpen() {
		if (m_stream)
			Wait();
	}

	void Wait();
	operator FILE*() { return m_stream; }
};


} // Ext::


#if UCFG_USE_REGEX
#	include "ext-regex.h"
#endif

/*######   Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#if UCFG_USE_POSIX
#	include <utime.h>
#	include <glob.h>
#endif

#if UCFG_NTAPI && UCFG_WIN32
#	include <el/win/nt.h>
#	pragma comment(lib, "ntdll")
using namespace NT;
#endif // UCFG_WIN32_FULL

#if UCFG_WIN32
#	include <shlwapi.h>

#	include <el/libext/win32/ext-win.h>
#endif

#if UCFG_WIN32_FULL
#	pragma comment(lib, "shlwapi")

#	include <el/libext/win32/ext-full-win.h>
#	include <el/win/nt.h>
#endif

namespace Ext {
using namespace std;

path AFXAPI AddDirSeparator(const path& p) {
	path fn = p.filename();
	return fn.native() == "/" || fn.native() == "." ? p : p / "/";
}

/*!!!R

static bool WithDirSeparator(RCString s) {
if (s.IsEmpty())
return false;
wchar_t ch = s[s.Length-1];
return ch==path::preferred_separator || ch==Path::AltDirectorySeparatorChar;
}



String AFXAPI RemoveDirSeparator(RCString s, bool bOnEndOnly) {
	return WithDirSeparator(s) && (!bOnEndOnly || s.Length!=1) ? s.Left(int(s.Length-1)) : s;
}*/

Path::CSplitPath Path::SplitPath(RCString path) {
	CSplitPath sp;
#ifdef _WIN32
	TCHAR drive[_MAX_DRIVE],
		dir[_MAX_DIR],
		fname[_MAX_FNAME],
		ext[_MAX_EXT];
#	if UCFG_WCE
	_tsplitpath_s(path, drive, sizeof drive,
		dir, sizeof dir,
		fname, sizeof fname,
		ext, sizeof ext);
#	else
	_tsplitpath(path, drive, dir, fname, ext);
#	endif
	sp.m_drive = drive;
	sp.m_dir = dir;
	sp.m_fname = fname;
	sp.m_ext = ext;
#else
	size_t fn = path.rfind(path::preferred_separator);
	String sSnExt = path.substr(fn==String::npos ? 0 : fn+1);

	size_t ext = sSnExt.rfind('.');
	if (ext != String::npos)
		sp.m_ext = sSnExt.substr(ext);
	else
		ext = sSnExt.length();
	sp.m_fname = sSnExt.Left(ext);
	sp.m_dir = path.Left(fn+1);
#endif
	return sp;
}

pair<path, UINT> Path::GetTempFileName(const path& p, RCString prefix, UINT uUnique) {
#if UCFG_USE_POSIX
	char buf[PATH_MAX+1];
	ZeroStruct(buf);
	int fd = CCheck(::mkstemp(strncpy(buf, (p / (prefix+"XXXXXX")).c_str(), _countof(buf)-1)));
	close(fd);
	return pair<path, UINT>(buf, 1);
#else
	TCHAR buf[MAX_PATH];
	UINT r = Win32Check(::GetTempFileName(String(p.native()), prefix, uUnique, buf));
	return pair<path, UINT>(ToPath(buf), r);
#endif
}

path Path::GetPhysicalPath(const path& p) {
	String path = p;
#if UCFG_WIN32_FULL
	while (true) {
		Path::CSplitPath sp = SplitPath(path);
		vector<String> vec = System.QueryDosDevice(sp.m_drive);				// expand SUBST-ed drives
		if (vec.empty() || vec[0].Left(4) != "\\??\\")
			break;
		path = vec[0].substr(4) + sp.m_dir + sp.m_fname + sp.m_ext;
	}
#endif
	return path.c_str();
}

#if UCFG_WIN32_FULL && UCFG_USE_REGEX
static StaticWRegex s_reDosName("^\\\\\\\\\\?\\\\([A-Za-z]:.*)");   //  \\?\x:
#endif

path Path::GetTruePath(const path& p) {
#if UCFG_USE_POSIX	
	char buf[PATH_MAX];
	for (const char *psz = p.native();; psz = buf) {
		int len = ::readlink(psz, buf, sizeof(buf)-1);
		if (len == -1) {
			if (errno == EINVAL)
				return psz;
			CCheck(-1);
		}
		buf[len] = 0;
	}
#elif UCFG_WIN32_FULL
	TCHAR buf[_MAX_PATH];
	DWORD len = ::GetLongPathName(String(p.native()), buf, size(buf)-1);
	Win32Check(len != 0);
	buf[len] = 0;

	typedef DWORD (WINAPI *PFN_GetFinalPathNameByHandle)(HANDLE hFile, LPTSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

	DlProcWrap<PFN_GetFinalPathNameByHandle> pfn("KERNEL32.DLL", EXT_WINAPI_WA_NAME(GetFinalPathNameByHandle));
	if (!pfn)
		return buf;
	TCHAR buf2[_MAX_PATH];
	File file;
	file.Attach(::CreateFile(buf, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0));
	len = pfn((HANDLE)(intptr_t)Handle(file), buf2, size(buf2)-1, 0);
	Win32Check(len != 0);
	buf2[len] = 0;	
#if UCFG_USE_REGEX
	wcmatch m;
	if (regex_search(buf2, m, s_reDosName))
		return String(m[1]).c_str();
#else
	String sbuf(buf2);				//!!! incoplete check, better to use Regex
	int idx = sbuf.Find(':');
	if (idx != -1)
		return sbuf.Mid(idx-1);
#endif
	return buf2;
#else
	return p;
#endif
}


#if UCFG_WIN32
bool File::s_bCreateFileWorksWithMMF = (System.Version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
	(Environment.OSVersion.Platform==PlatformID::Win32NT) ||
	(Environment.OSVersion.Version.Major >= 5); // CE
#endif

File::File() {
}

File::~File() {
}

void File::Open(const File::OpenInfo& oi) {
#if UCFG_USE_POSIX
	if (is_directory(oi.Path))							// open() opens dirs, but we need not it
		Throw(E_ACCESSDENIED);

	int oflag = 0;
	switch (oi.Access) {
	case FileAccess::Read:	oflag = O_RDONLY;	break;
	case FileAccess::Write:	oflag = O_WRONLY;	break;
	case FileAccess::ReadWrite:	oflag = O_RDWR;	break;
	default:
		Throw(E_INVALIDARG);
	}
	switch (oi.Mode) {
	case FileMode::CreateNew:
		oflag |= O_CREAT | O_EXCL;
		break;
	case FileMode::Create:
		oflag |= O_CREAT | O_TRUNC;
		break;
	case FileMode::Open:
		break;
	case FileMode::OpenOrCreate:
		oflag |= O_CREAT;
		break;
	case FileMode::Append:
		oflag |= O_APPEND;
		break;
	case FileMode::Truncate:
		oflag |= O_TRUNC;
		break;
	default:
		Throw(E_INVALIDARG);
	}
	int fd = CCheck(::open(oi.Path.c_str(), oflag, 0644));
	Attach((intptr_t)fd);
#else
	DWORD dwAccess = 0;
	switch (oi.Access) {
	case FileAccess::Read:		dwAccess = GENERIC_READ;	break;
	case FileAccess::Write:		dwAccess = GENERIC_WRITE;	break;
	case FileAccess::ReadWrite:	dwAccess = GENERIC_READ|GENERIC_WRITE;	break;
	default:
		Throw(E_INVALIDARG);
	}
	DWORD dwShareMode = 0;
	if ((int)oi.Share & (int)FileShare::Read)
		dwShareMode |= FILE_SHARE_READ;
	if ((int)oi.Share & (int)FileShare::Write)
		dwShareMode |= FILE_SHARE_WRITE;
	if ((int)oi.Share & (int)FileShare::Delete)
		dwShareMode |= FILE_SHARE_DELETE;
	DWORD dwCreateFlag;
	switch (oi.Mode) {
	case FileMode::CreateNew:
		dwCreateFlag = CREATE_NEW;
		break;
	case FileMode::Create:
		dwCreateFlag = CREATE_ALWAYS;
		break;
	case FileMode::Open:
		dwCreateFlag = OPEN_EXISTING;
		break;
	case FileMode::Append:
		dwAccess = FILE_APPEND_DATA;
	case FileMode::OpenOrCreate:
		dwCreateFlag = OPEN_ALWAYS;
		break;
	case FileMode::Truncate:
		dwCreateFlag = TRUNCATE_EXISTING;
		break;
	default:
		Throw(E_INVALIDARG);
	}

	SECURITY_ATTRIBUTES sa = { sizeof sa };
	sa.bInheritHandle = bool(oi.Share & FileShare::Inheritable);
	
	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL
		| (oi.BufferingEnabled ? 0 : FILE_FLAG_NO_BUFFERING)
		| (bool(oi.Options & FileOptions::Asynchronous) ? FILE_FLAG_OVERLAPPED : 0)
		| (bool(oi.Options & FileOptions::RandomAccess) ? FILE_FLAG_RANDOM_ACCESS : 0)
		| (bool(oi.Options & FileOptions::SequentialScan) ? FILE_FLAG_SEQUENTIAL_SCAN : 0)
		| (bool(oi.Options & FileOptions::DeleteOnClose) ? FILE_FLAG_DELETE_ON_CLOSE : 0)
		| (bool(oi.Options & FileOptions::WriteThrough) ? FILE_FLAG_WRITE_THROUGH : 0);		

	Attach(::CreateFile(ExcLastStringArgKeeper(oi.Path.native()), dwAccess, dwShareMode, &sa, dwCreateFlag, dwFlagsAndAttributes, NULL));
	if (oi.Mode == FileMode::Append)
		SeekToEnd(); 
#endif	// UCFG_USE_POSIX
}

void File::Open(const path& p, FileMode mode, FileAccess access, FileShare share) {
	OpenInfo oi;
	oi.Path = p;
	oi.Mode = mode;
	oi.Access = access;
	oi.Share = share;
	Open(oi);
}

Blob File::ReadAllBytes(const path& p) {
	FileStream stm(p, FileMode::Open, FileAccess::Read);
	uint64_t len = stm.Length;
	if (len > numeric_limits<size_t>::max())
		Throw(E_OUTOFMEMORY);
	Blob blob(0, (size_t)len);
	stm.ReadBuffer(blob.data(), blob.Size);
	return blob;
}

void File::WriteAllBytes(const path& p, const ConstBuf& mb) {
	FileStream(p, FileMode::Create, FileAccess::Write).WriteBuf(mb);
}

String File::ReadAllText(const path& p, Encoding *enc) {
	return enc->GetChars(ReadAllBytes(p));
}

void File::WriteAllText(const path& p, RCString contents, Encoding *enc) {
	WriteAllBytes(p, enc->GetBytes(contents));
}

#if !UCFG_USE_POSIX

void File::Create(RCString fileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, LPSECURITY_ATTRIBUTES lpsa) {
#if UCFG_USE_POSIX
	int oflag = 0;
	if ((dwDesiredAccess & (GENERIC_READ|GENERIC_WRITE)) == (GENERIC_READ|GENERIC_WRITE))
		oflag = O_RDWR;
	else if (dwDesiredAccess & GENERIC_READ)
		oflag = O_RDONLY;
	else if (dwDesiredAccess & GENERIC_WRITE)
		oflag = O_WRONLY;
	Attach(CCheck(::open(fileName, oflag)));
#else
	Attach(::CreateFile(fileName, dwDesiredAccess, dwShareMode, lpsa, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile));
#endif
}

void File::CreateForMapping(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, LPSECURITY_ATTRIBUTES lpsa) {
#if UCFG_WCE
	if (!s_bCreateFileWorksWithMMF) {
		Attach(::CreateFileForMapping(lpFileName, dwDesiredAccess, dwShareMode, lpsa, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile));
		m_bFileForMapping = true;
		return;
	}
#endif
	Create(lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile, lpsa);
}

bool File::Read(void *buf, uint32_t len, uint32_t *read, OVERLAPPED *pov) {
	bool b = ::ReadFile((HANDLE)(intptr_t)HandleAccess(_self), buf, len, (DWORD*)read, pov);
	if (!b && ::GetLastError()==ERROR_BROKEN_PIPE) {
		*read = 0;
		return true;
	}
	return CheckPending(b);
}

bool File::Write(const void *buf, uint32_t len, uint32_t *written, OVERLAPPED *pov) {
	return CheckPending(::WriteFile((HANDLE)(intptr_t)HandleAccess(_self), buf, len, (DWORD*)written, pov));
}

bool File::DeviceIoControl(int code, LPCVOID bufIn, size_t nIn, LPVOID bufOut, size_t nOut, LPDWORD pdw, LPOVERLAPPED pov) {
	return CheckPending(::DeviceIoControl((HANDLE)(intptr_t)HandleAccess(_self), code, (void*)bufIn, (DWORD)nIn, bufOut, (DWORD)nOut, pdw, pov));
}

void File::CancelIo() {
	static CDynamicLibrary dll;
	if (!dll)
		dll.Load("kernel32.dll");
	typedef BOOL(__stdcall *C_CancelIo)(HANDLE);
	Win32Check(((C_CancelIo)dll.GetProcAddress((LPCTSTR)String("CancelIo")))((HANDLE)(intptr_t)HandleAccess(_self)));
}

bool File::CheckPending(BOOL b) {
	if (b)
		return true;
	Win32Check(GetLastError() == ERROR_IO_PENDING);
	return false;
}

#endif // !UCFG_USE_POSIX

void File::SetEndOfFile() {
#if UCFG_USE_POSIX
	CCheck(::ftruncate((int)(intptr_t)HandleAccess(_self), Seek(0, SeekOrigin::Current)));
#else
	Win32Check(::SetEndOfFile((HANDLE)(intptr_t)HandleAccess(_self)));
#endif
}


#ifdef WIN32
OVERLAPPED *File::SetOffsetForFileOp(OVERLAPPED& ov, int64_t offset) {
	OVERLAPPED *pov = 0;
#	if UCFG_WCE
	switch (offset) {				//!!! Thread-unsafe
	case -1:
		SeekToEnd();
	case CURRENT_OFFSET:
		break;
	default:
		Seek(offset);
	}
#	else
	if (offset != CURRENT_OFFSET)
		ZeroStruct(*(pov = &ov));
#	endif
	if (pov) {
		ov.Offset = DWORD(offset);
		ov.OffsetHigh = DWORD(offset >> 32);
	}
	return pov;
}
#endif // WIN32

void File::Write(const void *buf, size_t size, int64_t offset) {
#if UCFG_USE_POSIX
	if (offset >= 0)
		CCheck(::pwrite((int)(intptr_t)HandleAccess(_self), buf, size, offset));
	else
		CCheck(::write((int)(intptr_t)HandleAccess(_self), buf, size));
#else
	OVERLAPPED ov, *pov = SetOffsetForFileOp(ov, offset);
	DWORD nWritten;
	for (const byte *p = (const byte*)buf; size; size -= nWritten, p += nWritten) {
		if (pov) {
			ov.Offset = DWORD(offset);
			ov.OffsetHigh = DWORD(offset >> 32);
		}
		Win32Check(::WriteFile((HANDLE)(intptr_t)HandleAccess(_self), p, std::min(size, (size_t)0xFFFFFFFF), &nWritten, pov));
		if (offset >= 0)
			offset += nWritten;
	}
#endif
}

uint32_t File::Read(void *buf, size_t size, int64_t offset) {
#if UCFG_USE_POSIX
	ssize_t r = offset>=0 ? ::pread((int)(intptr_t)HandleAccess(_self), buf, size, offset) : ::read((int)(intptr_t)HandleAccess(_self), buf, size);
	CCheck(r>=0 ? 0 : -1);
	return r;
#elif UCFG_NTAPI
	IO_STATUS_BLOCK iost;
	LARGE_INTEGER li;
	li.QuadPart = offset;
	switch (NTSTATUS st = ::NtReadFile(HandleAccess(_self), 0, 0, 0, &iost, buf, size, &li, 0)) {
	case STATUS_END_OF_FILE:
	case STATUS_PIPE_BROKEN:
		return 0;
	default:
		NtCheck(st);
	}
	return (uint32_t)iost.Information;	//!!!?
#else
	OVERLAPPED ov, *pov = SetOffsetForFileOp(ov, offset);
	DWORD nRead;
	Win32Check(::ReadFile(HandleAccess(_self), buf, std::min(size, (size_t)0xFFFFFFFF), &nRead, pov), ERROR_BROKEN_PIPE);
	return nRead;
#endif
}

void File::Lock(uint64_t pos, uint64_t len, bool bExclusive, bool bFailImmediately) {
#if UCFG_USE_POSIX
	int64_t prev = File::Seek(pos, SeekOrigin::Begin);
	int rc = ::lockf((int)(intptr_t)HandleAccess(_self), F_LOCK, len);
	File::Seek(prev, SeekOrigin::Begin);
	CCheck(rc);
#else
	OVERLAPPED ovl = { 0 };
	ovl.Offset = DWORD(pos);
	ovl.OffsetHigh = DWORD(pos >> 32);
	DWORD flags = bExclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
	if (bFailImmediately)
		flags |= LOCKFILE_FAIL_IMMEDIATELY;
	Win32Check(::LockFileEx(HandleAccess(_self), flags, 0, DWORD(len), DWORD(len >> 32), &ovl));
#endif
}

void File::Unlock(uint64_t pos, uint64_t len) {
#if UCFG_USE_POSIX
	int64_t prev = File::Seek(pos, SeekOrigin::Begin);
	int rc = ::lockf((int)(intptr_t)HandleAccess(_self), F_ULOCK, len);
	File::Seek(prev, SeekOrigin::Begin);
	CCheck(rc);
#else
	OVERLAPPED ovl = { 0 };
	ovl.Offset = DWORD(pos);
	ovl.OffsetHigh = DWORD(pos >> 32);
	Win32Check(::UnlockFileEx((HANDLE)(intptr_t)HandleAccess(_self), 0, DWORD(len), DWORD(len >> 32), &ovl));
#endif
}

size_t File::PhysicalSectorSize() const {			// return 0 if not detected
#if UCFG_WIN32_FULL
	typedef NTSTATUS (NTAPI *PFN_ZwQueryVolumeInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, NT::FS_INFORMATION_CLASS);
	static DlProcWrap<PFN_ZwQueryVolumeInformationFile> pfnZwQueryVolumeInformationFile(::LoadLibrary(_T("NTDLL.DLL")), "ZwQueryVolumeInformationFile");
	if (pfnZwQueryVolumeInformationFile) {
		IO_STATUS_BLOCK io;
		NT::FILE_FS_SECTOR_SIZE_INFORMATION outs;
		if (!pfnZwQueryVolumeInformationFile((HANDLE)DangerousGetHandle(), &io, &outs, sizeof outs, NT::FileFsSectorSizeInformation))
			return outs.PhysicalBytesPerSectorForPerformance;
	}
#endif
	return 0;
}

int64_t File::Seek(const int64_t& off, SeekOrigin origin) {
#if UCFG_USE_POSIX
	return CCheck(::lseek((int)(intptr_t)HandleAccess(_self), (long)off, (int)origin));
#else
	ULARGE_INTEGER uli;
	uli.QuadPart = (ULONGLONG)off;
	uli.LowPart = ::SetFilePointer((HANDLE)(intptr_t)HandleAccess(_self), (LONG)uli.LowPart, (LONG*)&uli.HighPart, (DWORD)origin);
	if (uli.LowPart == MAXDWORD)
		Win32Check(GetLastError() == NO_ERROR);
	return uli.QuadPart;
#endif
}

void File::Flush() {
#if UCFG_USE_POSIX
	CCheck(::fsync((int)(intptr_t)HandleAccess(_self)));
#else
	Win32Check(::FlushFileBuffers(HandleAccess(_self)));
#endif
}  

uint64_t File::get_Length() const {
#if UCFG_USE_POSIX
	struct stat st;
	CCheck(::fstat((int)(intptr_t)HandleAccess(_self), &st));
	return st.st_size;
#else
	ULARGE_INTEGER uli;
	uli.LowPart = ::GetFileSize((HANDLE)(intptr_t)HandleAccess(_self), &uli.HighPart);
	if (uli.LowPart == INVALID_FILE_SIZE)
		Win32Check(GetLastError() == NO_ERROR);
	return uli.QuadPart;
#endif
}

void File::put_Length(uint64_t len) {
#if UCFG_USE_POSIX
	CCheck(::ftruncate((int)(intptr_t)HandleAccess(_self), len));
#else
	Seek((int64_t)len, SeekOrigin::Begin);
	SetEndOfFile();
#endif
}

void FileStream::Open(const path& p, FileMode mode, FileAccess access, FileShare share, size_t bufferSize, FileOptions options) {
	intptr_t h = File(p, mode, access, share).Detach();
#if UCFG_WIN32_FULL
	int flags = 0;
	if (mode == FileMode::Append)
		flags |= _O_APPEND;
	switch (access) {
	case FileAccess::Read:		flags |= _O_RDONLY; break;
	case FileAccess::Write:		flags |= _O_WRONLY; break;
	case FileAccess::ReadWrite:	flags |= _O_RDWR; break;
	}

	int fd = _open_osfhandle(h, flags);
#elif UCFG_WCE
	HANDLE fd = h;
#else
	int fd = (int)(intptr_t)h;				// on POSIX our HANDLE used only as int
#endif

	char smode[20] = "";
	if (access == FileAccess::Read || access == FileAccess::ReadWrite)
		strcat(smode, "r");
	switch (mode) {
	case FileMode::Append:
		strcpy(smode, "a");
		if ((int)access & (int)FileAccess::Read)
			strcat(smode, "+");
		break;
	case FileMode::CreateNew:
	case FileMode::Create:
		strcat(smode, "w");
		break;
	case FileMode::Open:
	case FileMode::OpenOrCreate:
		if (access == FileAccess::ReadWrite)
			strcat(smode, "+");
		else if (access == FileAccess::Write)
			strcat(smode, "w");
		break;
	case FileMode::Truncate:
		strcpy(smode, "w+");
		break;
	}

	if (!TextMode)
		strcat(smode, "b");

	if (bool(options & FileOptions::RandomAccess))
		strcat(smode, "R");
	if (bool(options & FileOptions::SequentialScan))
		strcat(smode, "S");
	if (bool(options & FileOptions::DeleteOnClose))
		strcat(smode, "D");

#if UCFG_WCE
	m_fstm = _wfdopen(fd, String(smode));
	if (!m_fstm)
		Throw(E_FAIL);
#else
	m_fstm = fdopen(fd, smode);
	if (!m_fstm)
		CCheck(-1);
#endif

	CCheck(setvbuf(m_fstm, 0, _IOFBF, bufferSize) ? -1 : 0);
}

intptr_t FileStream::get_Handle() const {
	if (m_fstm) {
#if UCFG_WIN32_FULL
		return (intptr_t)_get_osfhandle(fileno(m_fstm));
#else
		return (intptr_t)(uintptr_t)fileno(m_fstm);
#endif
	} else if (m_pFile)
		return m_pFile->DangerousGetHandle();
	else
		Throw(E_FAIL);
}

uint64_t FileStream::get_Length() const {
	if (m_fstm) {
		File file(Handle, false);
		return file.Length;
	} else if (m_pFile)
		return m_pFile->Length;
	else
		Throw(E_FAIL);
}

bool FileStream::Eof() const {
	if (m_fstm) {
		if (feof(m_fstm))
			return true;
		int ch = fgetc(m_fstm);
		if (ch == EOF)
			CFileCheck(ferror(m_fstm));		//!!!Check
		else
			ungetc(ch, m_fstm);
		return ch == EOF;
	} else if (m_pFile)
		return Position == m_pFile->Length;
	else
		Throw(E_FAIL);
}

size_t FileStream::Read(void *buf, size_t count) const {
	if (m_fstm)
		return fread(buf, 1, count, m_fstm);
		
#if UCFG_WIN32_FULL
	if (m_ovl) {
		uint32_t n;
		Win32Check(::ResetEvent(m_ovl->hEvent));
		if (!m_pFile->Read(buf, count, &n, m_ovl)) {
			int r = ::WaitForSingleObjectEx(m_ovl->hEvent, INFINITE, TRUE);
			if (r == WAIT_IO_COMPLETION)
				Throw(ExtErr::ThreadInterrupted);
			if (r != 0)
				Throw(E_FAIL);
			try {
				DBG_LOCAL_IGNORE_CONDITION(errc::broken_pipe);
				n = m_pFile->GetOverlappedResult(*m_ovl);
			} catch (const system_error& ex) {
				if (ex.code() != errc::broken_pipe)
					throw;
				n = 0;
			}
		}
		return n;
	}
#endif
	return m_pFile->Read(buf, (UINT)count);
}

void FileStream::ReadBuffer(void *buf, size_t count) const {
	if (m_fstm) {
		size_t r = fread(buf, 1, count, m_fstm);
		if (r != count) {
			if (feof(m_fstm))
				Throw(ExtErr::EndOfStream);
			else {
				CFileCheck(ferror(m_fstm));
				Throw(E_FAIL);
			}
		}
		return;
	}

	while (count) {
		uint32_t n;
#if UCFG_WIN32_FULL
		if (m_ovl) {
			Win32Check(::ResetEvent(m_ovl->hEvent));
			bool b = m_pFile->Read(buf, count, &n, m_ovl);
			if (!b) {
				int r = ::WaitForSingleObjectEx(m_ovl->hEvent, INFINITE, TRUE);
				if (r == WAIT_IO_COMPLETION)
					Throw(ExtErr::ThreadInterrupted);
				if (r != 0)
					Throw(E_FAIL);
				n = m_pFile->GetOverlappedResult(*m_ovl);
			}
		} else
#endif
			n = m_pFile->Read(buf, (UINT)count);
		if (n) {    
			count -= n;
			(BYTE*&)buf += n;
		} else
			Throw(ExtErr::EndOfStream);
	}
}

void FileStream::WriteBuffer(const void *buf, size_t count) {
	if (m_fstm) {
		size_t r = fwrite(buf, 1, count, m_fstm);
		if (r != count) {
			CFileCheck(ferror(m_fstm));
			Throw(E_FAIL);
		}
		return;
	}
#if UCFG_WIN32_FULL
	uint32_t n;
	if (m_ovl) {
		Win32Check(::ResetEvent(m_ovl->hEvent));
		bool b = m_pFile->Write(buf, count, &n, m_ovl);
		if (!b) {
				int r = ::WaitForSingleObjectEx(m_ovl->hEvent, INFINITE, TRUE);
			if (r == WAIT_IO_COMPLETION)
				Throw(ExtErr::ThreadInterrupted);
			if (r != 0)
				Throw(E_FAIL);
			n = m_pFile->GetOverlappedResult(*m_ovl);
			if (n != count)
				Throw(E_FAIL);
		}
	} else
#endif
		m_pFile->Write(buf, (UINT)count);
}

void FileStream::Close() const {
	if (m_fstm) {
		CCheck(fclose(exchange(m_fstm, nullptr)));
	} else if (m_pFile)
		m_pFile->Close();
}

void FileStream::Flush() {
	if (m_fstm)
		CCheck(fflush(m_fstm));
	else if (m_pFile)
		m_pFile->Flush();
	else
		Throw(E_FAIL);
}

size_t PositionOwningFileStream::Read(void *buf, size_t count) const {
	uint32_t cb = m_pFile->Read(buf, count, m_pos);
	m_pos += cb;
	return cb;
}

void PositionOwningFileStream::ReadBuffer(void *buf, size_t count) const {
	uint32_t cb = m_pFile->Read(buf, count, m_pos);
	m_pos += cb;
	if (cb != count)
		Throw(ExtErr::EndOfStream);
}

void PositionOwningFileStream::WriteBuffer(const void *buf, size_t count) {
	m_pFile->Write(buf, count, m_pos);
	m_pos += count;
}

#ifdef WIN32
WIN32_FIND_DATA FileSystemInfo::GetData() const {
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(String(FullPath.native()), &findFileData);
	Win32Check(hFind != INVALID_HANDLE_VALUE);
	Win32Check(FindClose(hFind));
	return findFileData;
}

DWORD FileSystemInfo::get_Attributes() const {
	DWORD r = ::GetFileAttributes(ExcLastStringArgKeeper(FullPath.native()));
	Win32Check(r != INVALID_FILE_ATTRIBUTES);
	return r;
}
#endif

DateTime FileSystemInfo::get_CreationTime() const {
#if UCFG_USE_POSIX
	struct stat st;
	CCheck(::stat(FullPath.native(), &st));
	return DateTime::from_time_t(st.st_ctime);
#else
	return GetData().ftCreationTime;
#endif
}

void FileSystemInfo::put_CreationTime(const DateTime& dt) {
#if UCFG_USE_POSIX
	Throw(E_NOTIMPL);
#else
	File file;
	file.Create(String(FullPath.native()), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING);
	FILETIME ft = dt;
	Win32Check(::SetFileTime((HANDLE)(intptr_t)Handle(file), &ft, 0, 0));
#endif
}

DateTime FileSystemInfo::get_LastAccessTime() const {
#if UCFG_USE_POSIX
	struct stat st;
	CCheck(::stat(FullPath.native(), &st));
	return DateTime::from_time_t(st.st_atime);
#else
	return GetData().ftLastAccessTime;
#endif
}

void FileSystemInfo::put_LastAccessTime(const DateTime& dt) {
#if UCFG_USE_POSIX
	utimbuf ut;
	timeval tv, tvDt;
	get_LastWriteTime().ToTimeval(tv);
	ut.modtime = tv.tv_sec;
	dt.ToTimeval(tvDt);
	ut.actime = tvDt.tv_sec;
	CCheck(::utime(FullPath.native(), &ut));
#else
	File file;
	file.Create(String(FullPath.native()), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING);
	FILETIME ft = dt;
	Win32Check(::SetFileTime((HANDLE)(intptr_t)Handle(file), 0, &ft, 0));
#endif
}

DateTime FileSystemInfo::get_LastWriteTime() const {
#if UCFG_USE_POSIX
	struct stat st;
	CCheck(::stat(FullPath.native(), &st));
	return DateTime::from_time_t(st.st_mtime);
#else
	return GetData().ftLastWriteTime;
#endif
}

void FileSystemInfo::put_LastWriteTime(const DateTime& dt) {
#if UCFG_USE_POSIX
	utimbuf ut;
	timeval tv, tvDt;
	get_LastAccessTime().ToTimeval(tv);
	ut.actime = tv.tv_sec;
	dt.ToTimeval(tvDt);
	ut.modtime = tvDt.tv_sec;
	CCheck(::utime(FullPath.native(), &ut));
#else
	File file;
	file.Create(String(FullPath.native()), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE, OPEN_EXISTING);
	FILETIME ft = dt;
	Win32Check(::SetFileTime((HANDLE)(intptr_t)Handle(file), 0, 0, &ft));
#endif
}



#if UCFG_WIN32_FULL

vector<String> AFXAPI SerialPort::GetPortNames() {
	vector<String> r;
	try {
		DBG_LOCAL_IGNORE(E_ACCESSDENIED);

		RegistryKey key(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", false);
		CRegistryValues rvals(key);
		for (int i=0, count=rvals.Count; i<count; ++i)
			r.push_back(rvals[i]);
	} catch(RCExc) {
	}
	return r;
}

#endif // UCFG_WIN32_FULL



} // Ext::


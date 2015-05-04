/*######   Copyright (c) 2014-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>

#include "filesystem"

#if UCFG_USE_POSIX
#	include <dirent.h>
#endif

#if UCFG_WIN32
#	include <shlwapi.h>

#	include <el/libext/win32/ext-win.h>
#endif

namespace ExtSTL { namespace sys {
	using namespace Ext;
	using namespace chrono;

#if !UCFG_USE_POSIX
static error_code Win32_error_code(DWORD code = GetLastError()) {
	return error_code((int)code, system_category());
}

#endif

static error_code Last_error_code() {
#if UCFG_USE_POSIX
	return error_code(errno, generic_category());
#else
	return Win32_error_code();
#endif
}

void path::iterator::Retrieve() {
	m_from = m_to;
	string_type::const_iterator itRootNameEnd = m_pPath->m_s.begin() +  m_pPath->RootNameLen();
	if (m_to < itRootNameEnd) {
		m_to = itRootNameEnd;
		m_el = string_type(m_from, m_to);
	} else if (m_to==itRootNameEnd && m_to!=m_pPath->m_s.end() && IsSeparator(*m_to)) {
		m_el = string_type(preferred_separator);
		while (++m_to!=m_pPath->m_s.end() && IsSeparator(*m_to))
			;
	} else {
		int nSlash = 0;
		for (;m_to!=m_pPath->m_s.end() && IsSeparator(*m_to); ++nSlash)
			++m_to;
		string_type::const_iterator it = m_to;
		while (m_to!=m_pPath->m_s.end() && !IsSeparator(*m_to))
			++m_to;
		m_el = it!=m_to || !nSlash ? string_type(it, m_to) : ".";
	}
}

path::iterator& path::iterator::operator++() {
	Retrieve();
	return *this;
}

path::iterator& path::iterator::operator--() {
	for (iterator sav=*this, it(*m_pPath, m_pPath->m_s.begin()) ; it != sav;)
		*this = it++;
	return *this;
}

path path::parent_path() const {
	path r;
	if (!empty()) {
		for (iterator it=begin(), e=--end(); it!=e; ++it)
			r /= *it;
	}
	return r;
}

path path::stem() const {
	return Path::SplitPath(m_s).m_fname;
}

path path::extension() const {
	if (empty())
		return path();
	return Path::SplitPath(m_s).m_ext;
}

size_t path::RootNameLen() const {
	if (m_s.size() <= 2 || !IsSeparator(m_s[0]) || !IsSeparator(m_s[1]) || IsSeparator(m_s[2]))
		return max(size_t(0), m_s.find(':') + 1);
	else {
		size_t r = 3;
		for (; r<m_s.size() && !IsSeparator(m_s[r]); ++r)
			;
		return r;
	}
}

path path::root_name() const {
	return m_s.substr(0, RootNameLen());
}

path path::root_directory() const {
	size_t rlen = RootNameLen();
	return rlen < m_s.size() && IsSeparator(m_s[rlen]) ? string_type(m_s[rlen]) : string_type();
}

path path::root_path() const {
	return root_name() / root_directory();
}

path path::relative_path() const {
	size_t i = root_path().m_s.size();
	while (i<m_s.size() && IsSeparator(m_s[i]))
		++i;
	return m_s.substr(i);
}

path& path::replace_extension(const path& e) {
	return *this = parent_path() / (stem().native() + (e.empty() || e.c_str()[0]=='.' ?  "" : "." ) + e.native());
}

path& path::make_preferred() {
	for (size_t i=m_s.size(); i--;)
		m_s.SetAt(i, IsSeparator(m_s[i]) ? string_type::value_type(preferred_separator) : m_s[i]);	//!!!?
	return *this;
}

path& path::operator/=(const path& p) {
	if (!p.empty()) {
		if (empty())
			*this = p;
		else {
			if (!p.empty() && !IsSeparator(p.m_s[0]) && !IsSeparator(m_s.back()) && m_s.back()!=':')
				m_s += String(preferred_separator);
			m_s += p.m_s;
		}
	}
	return *this;
}

path& path::operator+=(const path::string_type& s) {
	m_s += s;
	return *this;
}

path AFXAPI u8path(Ext::RCString s) {
	return path(s);
}

static file_status StatusImp(const path& p, error_code& ec, bool bLstat) noexcept {
	ec.clear();
#if UCFG_USE_POSIX
	struct stat s;
	if ((bLstat ? lstat : stat)(p.native(), &s) < 0) {
		ec = error_code(errno, generic_category());
		return file_status(ec==errc::no_such_file_or_directory ? file_type::not_found : file_type::unknown);
	}
	return file_status(S_ISDIR(s.st_mode) ? file_type::directory
		: S_ISREG(s.st_mode) ? file_type::regular
		: S_ISLNK(s.st_mode) ? file_type::symlink
		: S_ISSOCK(s.st_mode) ? file_type::socket
		: S_ISFIFO(s.st_mode) ? file_type::fifo
		: S_ISBLK(s.st_mode) ? file_type::block
		: S_ISCHR(s.st_mode) ? file_type::character
		: file_type::unknown);
#else
	const int _FILE_ATTRIBUTE_REGULAR = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
		FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY;

	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (::GetFileAttributesEx(p.native(), GetFileExInfoStandard, &fad)) {
		return file_status(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? file_type::directory
			: fad.dwFileAttributes & _FILE_ATTRIBUTE_REGULAR ? file_type::regular
			: bLstat && (fad.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? file_type::symlink		//!!!? TEST
			: file_type::unknown);

	} else {
		int rc = GetLastError();
		ec = Win32_error_code(rc);
		switch (rc) {
		case ERROR_BAD_NETPATH:
		case ERROR_BAD_PATHNAME:
		case ERROR_FILE_NOT_FOUND:
		case ERROR_INVALID_DRIVE:
		case ERROR_INVALID_NAME:
		case ERROR_INVALID_PARAMETER:
		case ERROR_PATH_NOT_FOUND:
			return file_status(file_type::not_found);
		default:
			return file_status(file_type::none);
		}
	}
#endif	// !UCFG_USE_POSIX
}

file_status AFXAPI status(const path& p, error_code& ec) noexcept {
	return StatusImp(p, ec, false);
}

file_status AFXAPI status(const path& p) {
	error_code ec;
	file_status r = status(p, ec);
	if (r.type() == file_type::none)
		throw filesystem_error("status(p) error", p, ec);
	return r;
}

file_status AFXAPI symlink_status(const path& p, error_code& ec) noexcept {
	return StatusImp(p, ec, true);
}

file_status AFXAPI symlink_status(const path& p) {
	error_code ec;
	file_status r = symlink_status(p, ec);
	if (r.type() == file_type::none)
		throw filesystem_error("symlink_status(p) error", p, ec);
	return r;
}

file_time_type last_write_time(const path& p, error_code& ec) noexcept {
	ec.clear();
#if UCFG_USE_POSIX
	struct stat s;
	if (stat(p.native(), &s) >= 0) {
		file_time_type r = system_clock::from_time_t(s.st_mtime);
#ifdef STAT_HAVE_NSEC
		r += duration_cast<file_time_type::duration>(nanoseconds(s.st_mtime_nsec));
#endif
		return r;
	}
#else
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (GetFileAttributesEx(p.native(), GetFileExInfoStandard, &fad))
		return time_point_cast<file_time_type::duration>(DateTime::time_point(DateTime(fad.ftLastWriteTime)));
#endif
	ec = Last_error_code();
	return file_time_type::min();	//!!!?  -1 in the SPEC
}

file_time_type last_write_time(const path& p) {
	error_code ec;
	file_time_type r = last_write_time(p, ec);
	if (ec)
		throw filesystem_error("last_write_time(p) error", p, ec);
	return r;
}

file_status directory_entry::symlink_status(error_code& ec) const noexcept {
	ec.clear();
	return status_known(m_symlink_status) ? m_symlink_status
		: (m_symlink_status = std::sys::symlink_status(m_path, ec));
}

file_status directory_entry::symlink_status() const {
	error_code ec;
	file_status r = symlink_status(ec);
	if (ec)
		throw filesystem_error("directory_entry::symlink_status() error", m_path, ec);
	return r;
}

file_status directory_entry::status(error_code& ec) const noexcept {
	ec.clear();
	return status_known(m_status) ? m_status
		: (m_status = status_known(m_symlink_status) && !is_symlink(m_symlink_status) ? m_symlink_status : std::sys::status(m_path, ec));
}

file_status directory_entry::status() const {
	error_code ec;
	file_status r = status(ec);
	if (ec)
		throw filesystem_error("directory_entry::status() error", m_path, ec);
	return r;
}

DirectoryObj::~DirectoryObj() {
#if UCFG_USE_POSIX
	CCheck(::closedir((DIR*)m_dir)); //!!!noexcept
#else
	Win32Check(::FindClose((HANDLE)m_dir));	//!!!
#endif
}

directory_iterator::directory_iterator(const path& p) {
	error_code ec;
	Init(p, ec);
	if (ec)
		throw filesystem_error("directory_iterator(p)", p, ec);
}

directory_iterator::directory_iterator(const path& p, error_code& ec) noexcept {
	ec.clear();
	Init(p, ec);
}

void directory_iterator::Init(const path& p, error_code& ec) noexcept {
	m_prefix = p;
#if UCFG_USE_POSIX
	if (DIR *dir = opendir(p.native())) {
		(m_p = new DirectoryObj)->m_dir = (intptr_t)dir;
		increment(ec);
	} else
		ec = error_code(errno, generic_category());
#else
	WIN32_FIND_DATA wfd;
	if (HANDLE h = ::FindFirstFile((p.empty() ? p : p / "*").native(), &wfd)) {
		(m_p = new DirectoryObj)->m_dir = intptr_t(h);
		do {
			if (StrCmp(wfd.cFileName, _T(".")) && StrCmp(wfd.cFileName, _T(".."))) {
				m_entry = directory_entry(m_prefix / wfd.cFileName);
				return;
			}
		} while (::FindNextFile(h, &wfd));
	}
	switch (int err = GetLastError()) {
	case ERROR_FILE_NOT_FOUND:
	case ERROR_NO_MORE_FILES:
		break;
	default:
		ec = Win32_error_code(err);
	}
	m_p.reset();
#endif
}

directory_iterator& directory_iterator::increment(error_code& ec) noexcept {
	ec.clear();
#if UCFG_USE_POSIX
	DIR *dir = (DIR*)m_p->m_dir;
	errno = 0;
	while (dirent *e = ::readdir((DIR*)m_p->m_dir)) {
		if (strcmp(e->d_name, ".") && strcmp(e->d_name, "..")) {
			m_entry = directory_entry(m_prefix / e->d_name);
			return *this;
		}
	}
	ec = error_code(errno, generic_category());
#else
	WIN32_FIND_DATA wfd;
	while (::FindNextFile(HANDLE(m_p->m_dir), &wfd)) {
		if (StrCmp(wfd.cFileName, _T(".")) && StrCmp(wfd.cFileName, _T(".."))) {
			m_entry = directory_entry(m_prefix / wfd.cFileName);
			return *this;
		}
	}
	switch (int err = GetLastError()) {
	case ERROR_NO_MORE_FILES: break;
	default:
		ec = Win32_error_code();
	}
#endif
	m_p.reset();
	return *this;
}

directory_iterator& directory_iterator::operator++() {
	error_code ec;
	increment(ec);
	if (ec)
		throw filesystem_error("++directory_iterator", m_prefix, ec);
	return *this;
}

/*!!!R
vector<String> Directory::GetItems(RCString path, RCString searchPattern, bool bDirs) {
#if UCFG_USE_POSIX
#	if UCFG_STLSOFT
	typedef unixstl::glob_sequence gs_t;
	gs_t gs(path.c_str(), searchPattern.c_str(), gs_t::files | (bDirs ? gs_t::directories : 0));
	return vector<String>(gs.begin(), gs.end());
#	else
	vector<String> r;
	glob_t gt;
	CCheck(::glob(Path::Combine(path, searchPattern), GLOB_MARK, 0, &gt));
	for (int i=0; i<gt.gl_pathc; ++i) {
		const char *p = gt.gl_pathv[i];
		size_t len = strlen(p);
		if (p[len-1] != '/')
			r.push_back(p);
		else if (bDirs)
			r.push_back(String(p, len-1));
	}
	globfree(&gt);
	if (searchPattern == "*") {
		CCheck(::glob(Path::Combine(path, ".*"), GLOB_MARK, 0, &gt));
		for (int i=0; i<gt.gl_pathc; ++i) {
			const char *p = gt.gl_pathv[i];
			size_t len = strlen(p);
			if (p[len-1] != '/')
				r.push_back(p);
			else if (bDirs) {
				String s(p, len-1);
				if (s.Right(3) != "/.." && s.Right(2) != "/.")
					r.push_back(s);
			}
		}
		globfree(&gt);
	}
	return r;
#	endif
#else
	vector<String> r;
	CWinFileFind ff(Path::Combine(path, searchPattern));
	for (WIN32_FIND_DATA fd; ff.Next(fd);) {
		if (!(bool(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ^ bDirs)) {
			String name = fd.cFileName;
			if (name != "." && name != "..")
				r.push_back(Path::Combine(path, name));
		}
	}
	return r;
#endif
}
*/

static bool StatEx(const path& p, long& dev, uint64_t& ino, error_code& ec) {
#if UCFG_USE_POSIX
	struct stat s;
	if (::stat(p.native(), &s)<0) {
		ec = error_code(errno, generic_category());
		return false;
	}
	dev = s.st_dev;
	ino = s.st_ino;
	return true;
#else
	HANDLE h = ::CreateFile(p.native(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, 0, 0);
	BY_HANDLE_FILE_INFORMATION info = { 0 };
	bool r = h!=INVALID_HANDLE_VALUE && ::GetFileInformationByHandle(h, &info);
	if (r) {
		dev = info.dwVolumeSerialNumber;
		ino = (uint64_t(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
	} else
		ec = Win32_error_code();
	::CloseHandle(h);
	return r;
#endif
}

bool AFXAPI equivalent(const path& p1, const path& p2, error_code& ec) noexcept {
	ec.clear();
	long dev1, dev2;
	uint64_t ino1, ino2;
	return StatEx(p1, dev1, ino1, ec) && StatEx(p2, dev2, ino2, ec) && dev1==dev2 && ino1==ino2;
}

bool AFXAPI equivalent(const path& p1, const path& p2) {
	error_code ec;
	bool r = equivalent(p1, p2, ec);
	if (ec)
		throw filesystem_error("equivalent(p1, p2) error", p1, p2, ec);
	return r;
}

bool AFXAPI copy_file(const path& from, const path& to, copy_options options, error_code& ec) {
	ec.clear();
	bool bExistsTo = exists(to);
	if (bExistsTo && bool(options & copy_options::skip_existing))
		return false;
	if (!exists(from) || bExistsTo && equivalent(from, to, ec)) {
		ec = make_error_code(errc::file_exists);
		return false;
	}
#if UCFG_USE_POSIX
	Blob buf(0, 8192);
	ifstream ifs(from.native(), ios::binary);
	ofstream ofs(to.native(), ios::openmode(ios::binary | (bool(options & copy_options::overwrite_existing) ? ios::trunc : 0)));
	if (!ifs || !ofs) {
		ec = make_error_code(errc::operation_not_permitted);
		return false;
	}
	while (ifs) {									 //!!!TODO detect fail during copy process
		ifs.read((char*)buf.data(), buf.Size);
		ofs.write((char*)buf.data(), ifs.gcount());
	}
#else
	if (!::CopyFile(from.native(), to.native(), !bool(options & copy_options::overwrite_existing))) {
		ec = Win32_error_code();
		return false;
	}
#endif
	return true;
}

bool AFXAPI copy_file(const path& from, const path& to, copy_options options) {
	error_code ec;
	bool r = copy_file(from, to, options, ec);
	if (ec)
		throw filesystem_error("copy_file(f, t, o) error", from, to, ec);
	return r;
}

uintmax_t AFXAPI file_size(const path& p, error_code& ec) noexcept {
#if UCFG_USE_POSIX
	struct stat st;
	if (::stat(p.c_str(), &st) >= 0)
		return st.st_size;
#else
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (::GetFileAttributesEx(p.native(), GetFileExInfoStandard, &fad))
		return uint64_t(fad.nFileSizeHigh) << 32 | fad.nFileSizeLow;
#endif
	ec = Last_error_code();
	return static_cast<uintmax_t>(-1);
}

uintmax_t AFXAPI file_size(const path& p) {
	error_code ec;
	uintmax_t r = file_size(p, ec);
	if (ec)
		throw filesystem_error("file_size(p) error", p, ec);
	return r;
}

bool AFXAPI create_directory(const path& p, error_code& ec) noexcept {
	ec.clear();
#if UCFG_USE_POSIX
	bool b = ::mkdir(p.c_str(), 0777) >= 0;
	if (!b && errno!=EEXIST && errno!=EISDIR)
		ec = error_code(errno, generic_category());
#else
	bool b = ::CreateDirectory(p.c_str(), 0);
	if (!b && GetLastError()!=ERROR_ALREADY_EXISTS)
		ec = Win32_error_code();
#endif
	return b;
}

bool AFXAPI create_directory(const path& p) {
	error_code ec;
	bool r = create_directory(p, ec);
	if (ec)
		throw filesystem_error("create_directory(p) error", p, ec);
	return r;
}

bool AFXAPI create_directories(const path& p, error_code& ec) {
	ec.clear();
	if (p.empty())
		return false;
	switch (status(p).type()) {
	case file_type::not_found:
		create_directories(p.parent_path(), ec);
		return !ec && create_directory(p);
	case file_type::directory:
		return false;
	default:
		ec = make_error_code(errc::not_a_directory);
		return false;
	}
}

bool AFXAPI create_directories(const path& p) {
	error_code ec;
	bool r = create_directories(p, ec);
	if (ec)
		throw filesystem_error("create_directories(p) error", p, ec);
	return r;
}

bool AFXAPI remove(const path& p, error_code& ec) noexcept {
	ec.clear();
	if (!exists(p))
		return false;
#if UCFG_USE_POSIX
	if (is_directory(p, ec)) {
		if (::rmdir(p.native()) < 0) {
			ec = error_code(errno, generic_category());
			return false;
		}
		return true;
	} else {
		bool r = ::remove(p.native()) >= 0;
		if (!r)
			ec = error_code(errno, generic_category());
		return r;
	}
#else
	if (is_directory(p, ec)) {
		if (!::RemoveDirectory(p.native())) {
			ec = Win32_error_code();
			return false;
		}
		return true;
	} else {
		bool r = ::DeleteFile(p.native());
		if (!r && GetLastError()!=ERROR_FILE_NOT_FOUND)
			ec = Win32_error_code();
		return r;
	}
#endif
}

bool AFXAPI remove(const path& p) {
	error_code ec;
	bool r = remove(p, ec);
	if (ec)
		throw filesystem_error("remove(p) error", p, ec);
	return r;
}

uintmax_t AFXAPI remove_all(const path& p, error_code& ec) {
	ec.clear();
	if (!is_directory(p, ec))
		return remove(p, ec);
	else {
		uintmax_t r = 0;
		for (directory_iterator it, e; (it=directory_iterator(p))!=e;) {
			r += remove_all(it->path(), ec);
			if (ec)
				return r;
		}
		r += remove(p, ec);
		return r;
	}
}

uintmax_t AFXAPI remove_all(const path& p) {
	error_code ec;
	bool r = remove_all(p, ec);
	if (ec)
		throw filesystem_error("remove_all(p) error", p, ec);
	return r;
}

void AFXAPI rename(const path& old_p, const path& new_p, error_code& ec) noexcept {
	ec.clear();
	if (!exists(new_p, ec) || remove(new_p, ec)) {
#if UCFG_USE_POSIX
		if (::rename(old_p.native(), new_p.native()) < 0)
#else
		if (!::MoveFile(old_p.native(), new_p.native()))
#endif
			ec = Last_error_code();
	}
}

void AFXAPI rename(const path& old_p, const path& new_p) {
	error_code ec;
	rename(old_p, new_p, ec);
	if (ec)
		throw filesystem_error("rename(p1, p2) error", old_p, new_p, ec);
}

path AFXAPI temp_directory_path(error_code& ec) {
	ec.clear();
	path r;
#if UCFG_USE_POSIX
	String s = Environment::GetEnvironmentVariable("TMPDIR");
	s = !!s ? s : Environment::GetEnvironmentVariable("TMP");
	s = !!s ? s : Environment::GetEnvironmentVariable("TEMP");
	s = !!s ? s : Environment::GetEnvironmentVariable("TEMPDIR");
	r = !!s ? s : String("/tmp");
#else
	TCHAR buf[MAX_PATH];
	DWORD len = ::GetTempPath(size(buf), buf);
	if (!len || len>=size(buf)) {
		ec = Win32_error_code();
		return path();
	}
	r = buf;
#endif
	if (!is_directory(r))
		ec = make_error_code(errc::no_such_file_or_directory);
	return r;
}

path AFXAPI temp_directory_path() {
	error_code ec;
	path r = temp_directory_path(ec);
	if (ec)
		throw filesystem_error("temp_directory_path() error", ec);
	return r;
}


#if UCFG_WCE
	statis path s_currentDirectory;			// non-Thead-Safe
#endif

path AFXAPI current_path(error_code& ec) {
	ec.clear();
#if UCFG_USE_POSIX
	char buf[PATH_MAX];
	if (char *r = ::getcwd(buf, sizeof buf))
		return r;
	ec = error_code(errno, generic_category());
#elif UCFG_WCE
	if (s_currentDirectory.empty())
		s_currentDirectory = System.ExeFilePath.parent_path();
	return s_currentDirectory;
#else
	TCHAR buf[_MAX_PATH], *p = buf;
	DWORD dw = ::GetCurrentDirectory(_MAX_PATH, p);
	if (dw > _MAX_PATH) {
		p = (_TCHAR*)alloca(dw*sizeof(_TCHAR));
		dw = ::GetCurrentDirectory(dw, p);
	}
	if (dw)
		return p;
	ec = Win32_error_code();
#endif
	return path();
}

path AFXAPI current_path() {
	error_code ec;
	path r = current_path(ec);
	if (ec)
		throw filesystem_error("current_path() error", ec);
	return r;
}

void AFXAPI current_path(const path& p, error_code& ec) {
	ec.clear();
#if UCFG_USE_POSIX
	if (::chdir(p.c_str()) < 0)
		ec = error_code(errno, generic_category());
#elif UCFG_WCE
	s_currentDirectory = p;
#else
	if (!::SetCurrentDirectory(p.native()))
		ec = Win32_error_code();
#endif
}

void AFXAPI current_path(const path& p) {
	error_code ec;
	current_path(p, ec);
	if (ec)
		throw filesystem_error("current_path(p) error", p, ec);
}

path AFXAPI absolute(const path& p, const path& base) {
	if (p.has_root_name() && p.has_root_directory())
		return p;
	path a = absolute(base);
	return p.has_root_directory() ? a.root_name() / p
		: p.has_root_name() ? p.root_name() / a.root_directory() / a.relative_path() / p.relative_path()
		: a / p;
}

path AFXAPI canonical(const path& p, const path& base, error_code& ec) {
	ec.clear();
	path a = absolute(p, base);
#if UCFG_USE_POSIX
	Throw(E_NOTIMPL);
#else
	_TCHAR *buf = (TCHAR*)alloca(_MAX_PATH*sizeof(TCHAR));
	_TCHAR *pbuf;
	DWORD dw = ::GetFullPathName(a.native(), _MAX_PATH, buf, &pbuf);
	Win32Check(dw);
	if (dw >= _MAX_PATH) {
		int len = dw+1;
		buf = (TCHAR*)alloca(len*sizeof(TCHAR));
		Win32Check(::GetFullPathName(a.native(), len, buf, &pbuf));
	}
	return buf;
#endif
}

path AFXAPI canonical(const path& p, const path& base) {
	error_code ec;
	path r = canonical(p, base, ec);
	if (ec)
		throw filesystem_error("canonical(p, base) error", p, base, ec);
	return r;
}

path AFXAPI canonical(const path& p, error_code& ec) {
	path base = current_path(ec);
	return ec ? path() : canonical(p, base, ec);
}






}}  // ExtSTL::sys::



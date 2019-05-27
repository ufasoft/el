/*######   Copyright (c) 2013-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#include <el/ext.h>


#if UCFG_WIN32_FULL
#	include <el/win/nt.h>
#	pragma comment(lib, "ntdll")
	using namespace NT;
#endif // UCFG_WIN32_FULL


#if defined(HAVE_SQLITE3) || defined(HAVE_SQLITE4)

#	include "ext-sqlite.h"

#	if UCFG_USE_SQLITE==3
#		include <sqlite3.h>
#		if UCFG_LIB_DECLS
#			if UCFG_USE_SQLITE_MDB
#				pragma comment(lib, "sqlite3_mdb")
#			else
#				pragma comment(lib, "sqlite3")
#			endif
#		endif

#		define sqlite_prepare sqlite3_prepare_v2
#	else
#		include <sqlite4.h>
#		pragma comment(lib, "sqlite4")

#		define sqlite_prepare sqlite4_prepare

#		define SQLITE_OK				SQLITE_(OK)
#		define SQLITE_ROW				SQLITE_(ROW)
#		define SQLITE_DONE				SQLITE_(DONE)
#		define SQLITE_OPEN_EXCLUSIVE	SQLITE_(OPEN_EXCLUSIVE)
#		define SQLITE_OPEN_READONLY		SQLITE_(OPEN_READONLY)
#		define SQLITE_OPEN_READWRITE	SQLITE_(OPEN_READWRITE)
#	endif

#define sqlite_close					sqlite_(close)
#define sqlite_column_int64				sqlite_(column_int64)
#define sqlite_column_text16			sqlite_(column_text16)
#define sqlite_complete					sqlite_(complete)
#define sqlite_complete16				sqlite_(complete16)
#define sqlite_extended_result_codes	sqlite_(extended_result_codes)
#define sqlite_file						sqlite_(file)
#define sqlite_last_insert_rowid		sqlite_(last_insert_rowid)
#define sqlite_changes					sqlite_(changes)
#define sqlite_mem_methods				sqlite_(mem_methods)
#define sqlite_progress_handler			sqlite_(progress_handler)
#define sqlite_step						sqlite_(step)
#define sqlite_stmt						sqlite_(stmt)
#define sqlite_vfs_unregister			sqlite_(vfs_unregister)
#define sqlite_vfs						sqlite_(vfs)

namespace Ext { namespace DB { namespace sqlite_(NS) {

static struct CSqliteExceptionFabric : CExceptionFabric {
	CSqliteExceptionFabric(int fac)
		:	CExceptionFabric(fac)
	{
	}

	DECLSPEC_NORETURN void ThrowException(HRESULT hr, RCString msg) {
		throw SqliteException((uint16_t)hr, msg);
	}
} s_exceptionFabric(FACILITY_SQLITE);

int SqliteCheck(sqlite_db *db, int code) {
	switch (code) {
	case SQLITE_OK:
	case SQLITE_ROW:
	case SQLITE_DONE:
		return code;
	}
#if SQLITE_VERSION_NUMBER >= 3008000
	String s = db ? String((const Char16*)sqlite_(errmsg16)(db)) : String(sqlite3_errstr(code));
#else
	String s = db ? String((const Char16*)sqlite_(errmsg16)(db)) : String("SQLite Error") + Convert::ToString(code);
#endif
	throw SqliteException(code, s);
}

static class SQLiteCategory : public ErrorCategoryBase {
	typedef ErrorCategoryBase base;
public:
	SQLiteCategory()
		:	base("SQLite", FACILITY_SQLITE)
	{}

	string message(int errval) const override {
#if SQLITE_VERSION_NUMBER >= 3008000
		return sqlite3_errstr(errval);
#else
		return Convert::ToString(code);
#endif
	}
} s_sqliteErrorCategory;

const error_category& sqlite_category() {
	return s_sqliteErrorCategory;
}

SqliteException::SqliteException(int errval, RCString s)
	: base(error_code(errval, sqlite_category()), s)
{
}

bool SqliteIsComplete(const char *sql) {
	switch (int rc = ::sqlite_complete(sql)) {
	case 0:
	case 1:
		return rc;
	case SQLITE_(NOMEM):
		Throw(E_OUTOFMEMORY);
	default:
		Throw(E_FAIL);
	}
}

bool SqliteIsComplete16(const void *sql) {
	switch (int rc = ::sqlite_complete16(sql)) {
	case 0:
	case 1:
		return rc;
	case SQLITE_(NOMEM):
		Throw(E_OUTOFMEMORY);
	default:
		Throw(E_FAIL);
	}
}

SqliteReader::SqliteReader(SqliteCommand& cmd)
	:	m_cmd(cmd)
{
}

SqliteReader::~SqliteReader() {
	if (std::uncaught_exception()) {
		try {
			m_cmd.ResetHandle();
		} catch (RCExc) {
		}
	} else
		m_cmd.ResetHandle();
}

bool SqliteReader::Read() {
	return SqliteCheck(m_cmd.m_con, ::sqlite_(step)(m_cmd)) == SQLITE_(ROW);
}

int32_t SqliteReader::GetInt32(int i) {
	return sqlite_(column_int)(m_cmd, i);
}

int64_t SqliteReader::GetInt64(int i) {
	return sqlite_(column_int64)(m_cmd, i);
}

double SqliteReader::GetDouble(int i) {
	return sqlite_(column_double)(m_cmd, i);
}

String SqliteReader::GetString(int i) {
	return (const Char16*)sqlite_(column_text16)(m_cmd, i);
}

Span SqliteReader::GetBytes(int i) {
	sqlite_(value) *value = sqlite_(column_value)(m_cmd, i);
	return Span((const uint8_t*)sqlite_(value_blob)(value), sqlite_(value_bytes)(value));
}

DbType SqliteReader::GetFieldType(int i) {
	switch (int typ = ::sqlite_(column_type)(m_cmd, i)) {
	case SQLITE_(NULL): return DbType::Null;
	case SQLITE_(INTEGER): return DbType::Int;
	case SQLITE_(FLOAT): return DbType::Float;
	case SQLITE_(BLOB): return DbType::Blob;
	case SQLITE_(TEXT): return DbType::String;
	default:
		Throw(E_FAIL);
	}
}

int SqliteReader::FieldCount() {
	return ::sqlite_(column_count)(m_cmd);
}

String SqliteReader::GetName(int idx) {
	return (const Char16*)::sqlite_(column_name16)(m_cmd, idx);
}

SqliteCommand::SqliteCommand(SqliteConnection& con)
	:	m_con(con)
{
	m_con.Register(_self);
}

SqliteCommand::SqliteCommand(RCString cmdText, SqliteConnection& con)
	:	m_con(con)
{
	m_con.Register(_self);
	CommandText = cmdText;
}

SqliteCommand::~SqliteCommand() {
	m_con.Unregister(_self);
	Dispose();
}

void SqliteCommand::Dispose() {
	if (m_stmt) {
		int rc = ::sqlite_(finalize)(exchange(m_stmt, nullptr));
		if (rc != SQLITE_(BUSY) && !std::uncaught_exception())
			SqliteCheck(m_con, rc);
	}
}

sqlite_(stmt) *SqliteCommand::Handle() {
	if (!m_stmt) {
		sqlite_(stmt) *pst;
#if UCFG_USE_SQLITE==3
		const void *tail = 0;
		SqliteCheck(m_con, ::sqlite3_prepare16_v2(m_con, (const String::value_type*)CommandText, -1, &pst, &tail));
		m_stmt.reset(pst);
#else
		const char *tail = 0;
		Blob utf = Encoding::UTF8.GetBytes(CommandText);
		SqliteCheck(m_con, ::sqlite4_prepare(m_con, (const char*)utf.constData(), -1, &pst, &tail));
#endif
		m_stmt.reset(pst);
	}
	return m_stmt;
}

sqlite_stmt *SqliteCommand::ResetHandle(bool bNewNeedReset) {
	sqlite_(stmt) *h = Handle();
	if (m_bNeedReset) {
		int rc = ::sqlite_(reset)(h);
		if (SQLITE_(CONSTRAINT) != (uint8_t)rc)
			SqliteCheck(m_con, rc);
	}
	m_bNeedReset = bNewNeedReset;
	return h;
}

void SqliteCommand::ClearBindings() {
	SqliteCheck(m_con, ::sqlite_(clear_bindings)(Handle()));
}

SqliteCommand& SqliteCommand::Bind(int column, std::nullptr_t) {
	SqliteCheck(m_con, ::sqlite_(bind_null)(ResetHandle(), column));
	return _self;
}

SqliteCommand& SqliteCommand::Bind(int column, int32_t v) {
	SqliteCheck(m_con, ::sqlite_(bind_int)(ResetHandle(), column, v));
	return _self;
}

SqliteCommand& SqliteCommand::Bind(int column, int64_t v) {
	SqliteCheck(m_con, ::sqlite_(bind_int64)(ResetHandle(), column, v));
	return _self;
}

SqliteCommand& SqliteCommand::Bind(int column, double v) {
	SqliteCheck(m_con, ::sqlite_(bind_double)(ResetHandle(), column, v));
	return _self;
}

SqliteCommand& SqliteCommand::Bind(int column, RCSpan mb, bool bTransient) {
	SqliteCheck(m_con, ::sqlite_(bind_blob)(ResetHandle(), column, mb.data(), mb.size(), bTransient ? SQLITE_(TRANSIENT) : SQLITE_(STATIC)));
	return _self;
}

SqliteCommand& SqliteCommand::Bind(int column, RCString s) {
	const Char16 *p = (const Char16*)s;
	SqliteCheck(m_con, p ? ::sqlite_(bind_text16)(ResetHandle(), column, p, s.length() * 2, SQLITE_(TRANSIENT)) : ::sqlite_(bind_null)(ResetHandle(), column));
	return _self;
}

SqliteCommand& SqliteCommand::Bind(RCString parname, std::nullptr_t v) {
	return Bind(::sqlite_(bind_parameter_index)(ResetHandle(), parname), v);
}

SqliteCommand& SqliteCommand::Bind(RCString parname, int32_t v) {
	return Bind(::sqlite_(bind_parameter_index)(ResetHandle(), parname), v);
}

SqliteCommand& SqliteCommand::Bind(RCString parname, int64_t v) {
	return Bind(::sqlite_(bind_parameter_index)(ResetHandle(), parname), v);
}

SqliteCommand& SqliteCommand::Bind(RCString parname, double v) {
	return Bind(::sqlite_(bind_parameter_index)(ResetHandle(), parname), v);
}

SqliteCommand& SqliteCommand::Bind(RCString parname, RCSpan mb, bool bTransient) {
	return Bind(::sqlite_(bind_parameter_index)(ResetHandle(), parname), mb, bTransient);
}

SqliteCommand& SqliteCommand::Bind(RCString parname, RCString s) {
	return Bind(::sqlite_(bind_parameter_index)(ResetHandle(), parname), s);
}

void SqliteCommand::ExecuteNonQuery() {
	SqliteCheck(m_con, ::sqlite_(step)(ResetHandle(true)));
}

DbDataReader SqliteCommand::ExecuteReader() {
	ResetHandle(true);
	return DbDataReader(new SqliteReader(_self));
}

DbDataReader SqliteCommand::ExecuteVector() {
	DbDataReader r = SqliteCommand::ExecuteReader();
	if (!r.Read())
		Throw(ExtErr::DB_NoRecord);
	return r;
}

String SqliteCommand::ExecuteScalar() {
	if (SqliteCheck(m_con, ::sqlite_step(ResetHandle(true))) == SQLITE_ROW)
		return (const Char16*)sqlite_column_text16(m_stmt, 0);
	Throw(ExtErr::DB_NoRecord);
}

int64_t SqliteCommand::ExecuteInt64Scalar() {
	if (SqliteCheck(m_con, ::sqlite_step(ResetHandle(true))) == SQLITE_ROW)
		return ::sqlite_column_int64(m_stmt, 0);
	Throw(ExtErr::DB_NoRecord);
}

ptr<IDbCommand> SqliteConnection::CreateCommand() {
	return ptr<IDbCommand>(new SqliteCommand(_self));
}

void SqliteConnection::ExecuteNonQuery(RCString sql) {
	String s = sql.TrimEnd();
	if (s.length() > 1 && s[s.length()-1] != ';')
		s += ";";

	sqlite_(stmt) *pst;
#if UCFG_USE_SQLITE==3
	for (const void *tail=(const Char16*)s; SqliteIsComplete16(tail);) {
		SqliteCommand cmd(_self);
		SqliteCheck(_self, ::sqlite3_prepare16_v2(_self, tail, -1, &pst, &tail));
#else
	Blob utf = Encoding::UTF8.GetBytes(s);
	for (const char *tail=(const char*)utf.constData(); SqliteIsComplete(tail);) {
		SqliteCommand cmd(_self);
		SqliteCheck(_self, ::sqlite_prepare(_self, tail, -1, &pst, &tail));
#endif
		cmd.m_stmt.reset(pst);
		cmd.ExecuteNonQuery();
	}
}

int64_t SqliteConnection::get_LastInsertRowId() {
	return sqlite_last_insert_rowid(m_db);
}

int SqliteConnection::get_NumberOfChanges() {
	return sqlite_changes(m_db);
}

pair<int, int> SqliteConnection::Checkpoint(int eMode) {
#if UCFG_USE_SQLITE==3
	int log, ckpt;
	SqliteCheck(_self, ::sqlite3_wal_checkpoint_v2(_self, 0, eMode, &log, &ckpt));
	return make_pair(log, ckpt);
#else
	ExecuteNonQuery("PRAGMA lsm_checkpoint");
	return make_pair(0, 0);							//!!!?
#endif
}

/*!!!
static void __cdecl DbTraceHandler(void *cd, const char *zSql) {
	String s = zSql;
	OutputDebugString(s);
}*/

void SqliteConnection::SetProgressHandler(int(*pfn)(void*), void*p, int n) {
	::sqlite_progress_handler(m_db, n, pfn, p);
}

void SqliteConnection::Create(const path& file) {
	sqlite_db *pdb;
#if UCFG_USE_SQLITE==3
	SqliteCheck(m_db, ::sqlite3_open16((const String::value_type*)String(file), &pdb));
	m_db.reset(pdb);
	::sqlite_extended_result_codes(m_db, true);	//!!!? only Sqlite3?
#else
	Blob utf = Encoding::UTF8.GetBytes(file);
	SqliteCheck(m_db, ::sqlite4_open(0, (const char*)utf.constData(), &pdb));
	m_db.reset(pdb);
#endif
	ExecuteNonQuery("PRAGMA encoding = \"UTF-8\"");
	ExecuteNonQuery("PRAGMA foreign_keys = ON");

/*!!!
#ifndef SQLITE_OMIT_TRACE
	sqlite3_trace(m_con.db, DbTraceHandler, this);
#endif
	*/
}

void SqliteConnection::Open(const path& file, FileAccess fileAccess, FileShare share) {
	int flags = 0;
#if UCFG_USE_SQLITE==3
	if (share == FileShare::None)
		flags = SQLITE_OPEN_EXCLUSIVE;
#else
	Blob utf = Encoding::UTF8.GetBytes(String(file));
#endif
	switch (fileAccess) {
	case FileAccess::ReadWrite: flags |= SQLITE_OPEN_READWRITE; break;
	case FileAccess::Read: flags |= SQLITE_OPEN_READONLY; break;
	default:
		Throw(E_INVALIDARG);
	}
	sqlite_db *pdb;
#if UCFG_USE_SQLITE==3
	SqliteCheck(m_db, ::sqlite3_open_v2(String(file), &pdb, flags, 0));
	m_db.reset(pdb);
	::sqlite3_extended_result_codes(m_db, true);
#else
	SqliteCheck(m_db, ::sqlite4_open(0, (const char*)utf.constData(), &pdb));
	m_db.reset(pdb);
#endif
	ExecuteNonQuery("PRAGMA foreign_keys = ON");
}

void SqliteConnection::Close() {
	if (m_db) {
		DisposeCommands();
		sqlite_db *db = m_db.release();
		SqliteCheck(db, ::sqlite_close(db));
	}
}

void SqliteConnection::BeginTransaction() {
	ExecuteNonQuery("BEGIN TRANSACTION");
}

void SqliteConnection::Commit() {
	ExecuteNonQuery("COMMIT");
}

void SqliteConnection::Rollback() {
	ExecuteNonQuery("ROLLBACK");
}


#if UCFG_USE_SQLITE==3

static void *SqliteMallocFun(int size) {
	return Malloc(size);
}

static void SqliteFree(void *p) {
	free(p);
}

static void *SqliteRealloc(void *p, int size) {
	return Realloc(p, size);
}

static int SqliteSize(void *p) {
	if (!p)
		return 0;
#if UCFG_INDIRECT_MALLOC
	return (int)CAlloc::s_pfnMSize(p);
#else
	return (int)CAlloc::MSize(p);
#endif
}

static int SqliteRoundup(int size) {
	return (size+7) & ~7;
}

static int SqliteInit(void *) {
	return 0;
}

static void SqliteShutdown(void *) {
}

SqliteMalloc::SqliteMalloc() {
	sqlite_mem_methods memMeth = {
		&SqliteMallocFun,
		&SqliteFree,
		&SqliteRealloc,
		&SqliteSize,
		&SqliteRoundup,
		&SqliteInit,
		SqliteShutdown
	};
	SqliteCheck(0, ::sqlite3_config(SQLITE_CONFIG_MALLOC, &memMeth));
//!!! don't disable MemoryControl	SqliteCheck(0, ::sqlite3_config(SQLITE_CONFIG_MEMSTATUS, 0));
	::sqlite3_soft_heap_limit64(64*1024*1024);
}

typedef int (*PFN_Sqlite_xOpen)(sqlite_vfs*, const char *zName, sqlite_file*, int flags, int *pOutFlags);

#ifdef _WIN32

static int Sqlite_xDeviceCharacteristics(sqlite3_file*) {
	return SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN | SQLITE_IOCAP_ATOMIC512 | SQLITE_IOCAP_ATOMIC1K | SQLITE_IOCAP_ATOMIC2K | SQLITE_IOCAP_ATOMIC4K | SQLITE_IOCAP_SAFE_APPEND | SQLITE_IOCAP_POWERSAFE_OVERWRITE;
}

static PFN_Sqlite_xOpen s_pfnSqlite_xOpen;
static sqlite3_io_methods s_myMethods;

#if UCFG_WIN32_FULL

struct CWinFile {
  void *pMethod;/* Must be first */
  sqlite3_vfs *pVfs;
  HANDLE h;               /* Handle for accessing the file */
};

static int NT_Read(sqlite3_file *file, void *data, int iAmt, sqlite3_int64 iOfst) {
	CWinFile& wf = *(CWinFile*)file;
	IO_STATUS_BLOCK iost;
	LARGE_INTEGER li;
	li.QuadPart = iOfst;
	NTSTATUS st = ::NtReadFile(wf.h, 0, 0, 0, &iost, data, iAmt, &li, 0);
	if (st == STATUS_END_OF_FILE)
		iost.Information = 0;
	else if (st != 0)
		return SQLITE_IOERR_READ;
	if (int(iost.Information) < iAmt) {
	    memset(&((char*)data)[iost.Information], 0, iAmt-iost.Information);
		return SQLITE_IOERR_SHORT_READ;
	}
	return SQLITE_OK;
}

static int NT_Write(sqlite3_file *file, const void *data, int iAmt, sqlite3_int64 iOfst) {
	CWinFile& wf = *(CWinFile*)file;
	IO_STATUS_BLOCK iost;
	LARGE_INTEGER li;
	li.QuadPart = iOfst;
	NTSTATUS st = ::NtWriteFile(wf.h, 0, 0, 0, &iost, (void*)data, iAmt, &li, 0);
	if (st != 0)
		return SQLITE_IOERR_WRITE;
	if (int(iost.Information) < iAmt)
		return SQLITE_FULL;					//!!!?
	return SQLITE_OK;
}

#endif // UCFG_WIN32_FULL


int Sqlite_xOpen(sqlite3_vfs*vfs, const char *zName, sqlite3_file *file, int flags, int *pOutFlags) {
	int rc = s_pfnSqlite_xOpen(vfs, zName, file, flags, pOutFlags);
	if (SQLITE_OK == rc) {
		sqlite3_io_methods methods = *file->pMethods;					//!!!O
		methods.xDeviceCharacteristics = &Sqlite_xDeviceCharacteristics;
#if UCFG_WIN32_FULL
		methods.xRead = &NT_Read;
		methods.xWrite = &NT_Write;
#endif
		s_myMethods = methods;
		file->pMethods = &s_myMethods;
	}
	return rc;
}

#endif


SqliteVfs::SqliteVfs(bool bDefault)
	:	m_pimpl(new sqlite3_vfs)
{
	sqlite3_vfs *defaultVfs = ::sqlite3_vfs_find(0);
	*m_pimpl = *defaultVfs;
#ifdef _WIN32
	s_pfnSqlite_xOpen = m_pimpl->xOpen;
	m_pimpl->xOpen = &Sqlite_xOpen;
#endif
	SqliteCheck(0, ::sqlite3_vfs_register(m_pimpl, bDefault));
}

SqliteVfs::~SqliteVfs() {
	unique_ptr<sqlite3_vfs> p(m_pimpl);
	SqliteCheck(0, ::sqlite_vfs_unregister(p.get()));
}
#endif // UCFG_USE_SQLITE==3



}}} // namespace Ext::DB::sqlite_(NS)::

#endif // HAVE_SQLITE3 || HAVE_SQLITE4

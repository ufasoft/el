/*######   Copyright (c) 2013-2019 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once

#include "db-itf.h"

#ifndef UCFG_USE_SQLITE
#	define UCFG_USE_SQLITE 3
#endif

#ifndef UCFG_USE_SQLITE_MDB
#	define UCFG_USE_SQLITE_MDB 0
#endif

#if UCFG_USE_SQLITE == 3
#	define sqlite_(name) sqlite3_##name
#	define SQLITE_(name) SQLITE_##name

	struct sqlite3;
	typedef sqlite3 sqlite_db;
#elif UCFG_USE_SQLITE == 4
#	define sqlite_(name) sqlite4_##name
#	define SQLITE_(name) SQLITE4_##name

	struct sqlite4;
	typedef sqlite4 sqlite_db;
#endif

struct sqlite_(stmt);
struct sqlite_(vfs);

#define SQLITE_CHECKPOINT_RESTART 2		//!!! dup

namespace Ext { namespace DB { namespace sqlite_(NS) {

const error_category& sqlite_category();

int SqliteCheck(sqlite_db *db, int code);

class SqliteException : public DbException {
	typedef DbException base;
public:
	SqliteException(int errval, RCString s);
};

class SqliteConnection;
class SqliteCommand;

class SqliteReader : public IDataReader {
	SqliteCommand& m_cmd;
public:
	SqliteReader(SqliteCommand& cmd);
	~SqliteReader();
	int32_t GetInt32(int i) override;
	int64_t GetInt64(int i) override;
	double GetDouble(int i) override;
	String GetString(int i) override;
	Span GetBytes(int i) override;
	DbType GetFieldType(int i) override;
	int FieldCount() override;
	String GetName(int idx) override;
	bool Read() override;
};

class DbDataReader : public Pimpl<SqliteReader> {
	typedef Pimpl<SqliteReader> base;
public:
	DbDataReader() {
	}

	bool Read() { return m_pimpl->Read(); }
	int32_t GetInt32(int i) { return m_pimpl->GetInt32(i); }
	int64_t GetInt64(int i) { return m_pimpl->GetInt64(i); }
	double GetDouble(int i) { return m_pimpl->GetDouble(i); }
	String GetString(int i) { return m_pimpl->GetString(i); }
	Span GetBytes(int i) { return m_pimpl->GetBytes(i); }
	DbType GetFieldType(int i) { return m_pimpl->GetFieldType(i); }
	int FieldCount() { return m_pimpl->FieldCount(); }
	String GetName(int idx) { return m_pimpl->GetName(idx); }
	int GetOrdinal(RCString name) { return m_pimpl->GetOrdinal(name); }
	bool IsDBNull(int i) { return m_pimpl->IsDBNull(i); }
protected:
	DbDataReader(SqliteReader *sr) {
		m_pimpl = sr;
	}

	friend class SqliteCommand;
};

class SqliteCommand : public IDbCommand {
	observer_ptr<sqlite_(stmt)> m_stmt;
	CBool m_bNeedReset;
public:
	SqliteConnection& m_con;

	SqliteCommand(SqliteConnection& con);
	SqliteCommand(RCString cmdText, SqliteConnection& con);
	~SqliteCommand();
	operator sqlite_(stmt)*() { return m_stmt; }
	void ClearBindings();

	SqliteCommand& Bind(int column, std::nullptr_t) override;
	SqliteCommand& Bind(int column, int32_t v) override;
	SqliteCommand& Bind(int column, int64_t v) override;
	SqliteCommand& Bind(int column, double v) override;
	SqliteCommand& Bind(int column, RCSpan mb, bool bTransient = true) override;
	SqliteCommand& Bind(int column, RCString s) override;

	SqliteCommand& Bind(RCString parname, std::nullptr_t) override;
	SqliteCommand& Bind(RCString parname, int32_t v) override;
	SqliteCommand& Bind(RCString parname, int64_t v) override;
	SqliteCommand& Bind(RCString parname, double v) override;
	SqliteCommand& Bind(RCString parname, RCSpan mb, bool bTransient = true) override;
	SqliteCommand& Bind(RCString parname, RCString s) override;

#if	UCFG_SEPARATE_LONG_TYPE
	SqliteCommand& Bind(int column, long v) { return Bind(column, int64_t(v)); }
	SqliteCommand& Bind(RCString parname, long v) { return Bind(parname, int64_t(v)); }
#endif

	void Dispose() override;
	void ExecuteNonQuery() override;
	DbDataReader ExecuteReader();
	DbDataReader ExecuteVector();
	String ExecuteScalar() override;
	int64_t ExecuteInt64Scalar();
private:
	sqlite_(stmt) *Handle();
	sqlite_(stmt) *ResetHandle(bool bNewNeedReset = false);

	friend class SqliteConnection;
	friend class SqliteReader;
};

class SqliteConnection : public IDbConn, public ITransactionable {
	observer_ptr<sqlite_db> m_db;
public:
	SqliteConnection() {
	}

	SqliteConnection(const path& file) {
		Open(file);
	}

	~SqliteConnection() {
		Close();
	}

	operator sqlite_db*() { return m_db; }

	void SetProgressHandler(int(*pfn)(void*), void* p = 0, int n = 1);
	void Create(const path& file) override;
	void Open(const path& file, FileAccess fileAccess = FileAccess::ReadWrite, FileShare share = FileShare::ReadWrite) override;
	void Close() override;

	ptr<IDbCommand> CreateCommand() override;
	void ExecuteNonQuery(RCString sql) override;
	int64_t get_LastInsertRowId() override;
	int get_NumberOfChanges();
	pair<int, int> Checkpoint(int eMode);

	void BeginTransaction() override;
	void Commit() override;
	void Rollback() override;
};

class SqliteMalloc {
public:
	SqliteMalloc();
};

class SqliteVfs {
	sqlite_(vfs)* m_pimpl;
public:
	SqliteVfs(bool bDefault = true);
	~SqliteVfs();
};

class MMappedSqliteVfs : public SqliteVfs {
	typedef SqliteVfs base;
public:
	MMappedSqliteVfs();
};



}}} // namespace Ext::DB::sqlite_(NS)::



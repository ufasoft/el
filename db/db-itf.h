/*######     Copyright (c) 1997-2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com #########################################################################################################
#                                                                                                                                                                                                                                            #
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation;  either version 3, or (at your option) any later version.          #
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.   #
# You should have received a copy of the GNU General Public License along with this program; If not, see <http://www.gnu.org/licenses/>                                                                                                      #
############################################################################################################################################################################################################################################*/

#pragma once

namespace Ext { namespace DB {

class Sql {
public:
	static String AFXAPI Escape(RCString s);
};

ENUM_CLASS(DbType) {
	Null,
	Int,
	Float,
	Blob,
	String
} END_ENUM_CLASS(DbType);

interface IDataRecord : public Object {
	virtual int32_t GetInt32(int i) =0;
	virtual int64_t GetInt64(int i) =0;
	virtual double GetDouble(int i) =0;
	virtual String GetString(int i) =0;
	virtual ConstBuf GetBytes(int i) =0;
	virtual DbType GetFieldType(int i) =0;
	virtual int FieldCount() =0;
	virtual String GetName(int idx) =0;

	virtual int GetOrdinal(RCString name) {
		for (int i=0, n=FieldCount(); i<n; ++i)
			if (GetName(i) == name)
				return i;
		Throw(E_INVALIDARG);
	}

	virtual bool IsDBNull(int i) { return GetFieldType(i) == DbType::Null; }
};

interface IDataReader : public IDataRecord {
	virtual bool Read() =0;
};

interface IDbCommand : public Object {
	String CommandText;

	virtual IDbCommand& Bind(int column, std::nullptr_t) =0;
	virtual IDbCommand& Bind(int column, int32_t v) =0;
	virtual IDbCommand& Bind(int column, int64_t v) =0;
	virtual IDbCommand& Bind(int column, double v) =0;
	virtual IDbCommand& Bind(int column, const ConstBuf& mb, bool bTransient = true) =0;
	virtual IDbCommand& Bind(int column, RCString s) =0;

	virtual IDbCommand& Bind(RCString parname, std::nullptr_t) =0;
	virtual IDbCommand& Bind(RCString parname, int32_t v) =0;
	virtual IDbCommand& Bind(RCString parname, int64_t v) =0;
	virtual IDbCommand& Bind(RCString parname, double v) =0;
	virtual IDbCommand& Bind(RCString parname, const ConstBuf& mb, bool bTransient = true) =0;
	virtual IDbCommand& Bind(RCString parname, RCString s) =0;

	virtual void Dispose() =0;
	virtual void ExecuteNonQuery() =0;
	virtual String ExecuteScalar() =0;
};

interface ITransactionable {
public:
	virtual void BeginTransaction() =0;
	virtual void Commit() =0;
	virtual void Rollback() =0;
};

class TransactionScope : noncopyable {
public:
	ITransactionable& m_db;

	TransactionScope(ITransactionable& db)
		:	m_db(db)
	{
		m_db.BeginTransaction();
	}

	~TransactionScope() {
		if (!m_bCommitted) {
			InException ? m_db.Rollback() : m_db.Commit();
		}
	}

	void Commit() {
		m_db.Commit();
		m_bCommitted = true;
	}
private:
	CInException InException;
	CBool m_bCommitted;
};

interface IDbConn : public Object {
	typedef IDbConn class_type;
public:
	virtual void Create(RCString file) =0;
	virtual void Open(RCString file, FileAccess fileAccess, FileShare share = FileShare::ReadWrite) =0;
	virtual void Close() =0;
	virtual ptr<IDbCommand> CreateCommand() =0;

	virtual void ExecuteNonQuery(RCString sql) {
		ptr<IDbCommand> cmd = CreateCommand();
		cmd->CommandText = sql;
		cmd->ExecuteNonQuery();
	}
	
	virtual int64_t get_LastInsertRowId() =0;
	DEFPROP_VIRTUAL_GET(int64_t, LastInsertRowId);

	void Register(IDbCommand& cmd) { m_commands.insert(&cmd); }
	void Unregister(IDbCommand& cmd) { m_commands.erase(&cmd); }

protected:
	unordered_set<IDbCommand*> m_commands;

	void DisposeCommands() {
		EXT_FOR (IDbCommand *cmd, m_commands) {
			cmd->Dispose();
		}
		m_commands.clear();
	}
};

class DbException : public Exception {
	typedef Exception base;
public:
	DbException(HRESULT hr, RCString s)
		:	base(hr, s)
	{
	}
};

ENUM_CLASS(KVEnvFlags) {
	ReadOnly = 1,
	NoSync = 2
} END_ENUM_CLASS(KVEnvFlags);

ENUM_CLASS(CursorPos) {
	First,
	Next,
	Prev,
	Last,
	FindKey
} END_ENUM_CLASS(CursorPos);

ENUM_CLASS(KVStoreFlags) {
	NoOverwrite = 1
} END_ENUM_CLASS(KVStoreFlags);

}} // Ext::DB::



#include "CppMysql.h"

namespace Mysql
{
	MysqlConnection::MysqlConnection()
	{
		mysql_init(&m_Conn);

		char reconnect = 1;
		mysql_options(&m_Conn, MYSQL_OPT_RECONNECT, &reconnect);
		unsigned int timeout = 60;
		mysql_options(&m_Conn, MYSQL_OPT_CONNECT_TIMEOUT, (const char*)(&timeout));
		mysql_options(&m_Conn, MYSQL_OPT_READ_TIMEOUT, (const char*)&timeout);
		mysql_options(&m_Conn, MYSQL_OPT_WRITE_TIMEOUT, (const char*)&timeout);

		m_bAutoCommit = true;
	}

	bool MysqlConnection::Open(St_MysqlConf Conf)		// host:Ip或主机名
	{
		MYSQL* ret = mysql_real_connect(&m_Conn, Conf.m_Host.c_str(), Conf.m_User.c_str(), Conf.m_Passwd.c_str(), Conf.m_Db.c_str(), static_cast<unsigned int>(Conf.m_Port), nullptr, CLIENT_MULTI_STATEMENTS);
		if (nullptr == ret)
		{
			printf("Failed to connect to database: Error: %s\n", mysql_error(&m_Conn));
			//mysql_close(&m_Conn);
			return false;
		}
		char reconnect = 1;
		mysql_options(&m_Conn, MYSQL_OPT_RECONNECT, &reconnect);

		SetCharSet("utf8");
		return true;
	}

	MysqlResultSet* MysqlConnection::Query(const string& Sql)
	{
		if (mysql_real_query(&m_Conn, Sql.c_str(), Sql.size()) != 0)
		{
			unsigned int err = mysql_errno(&m_Conn);
			if (err == CR_SERVER_LOST || err == CR_CONN_HOST_ERROR || err == CR_SERVER_GONE_ERROR)
			{
				mysql_ping(&m_Conn);
				if (mysql_real_query(&m_Conn, Sql.c_str(), Sql.size()) != 0)
				{
					return nullptr;
				}
			}
			else return nullptr;
		}

		MYSQL_RES* res = mysql_store_result(&m_Conn);
		if (res == nullptr)
		{
			return nullptr;
		}

		int nRows = (int)mysql_affected_rows(&m_Conn);

		return new MysqlResultSet(res, nRows);
	}

	INT64 MysqlConnection::Update(const string& Sql)
	{
		if (mysql_real_query(&m_Conn, Sql.c_str(), Sql.size()) != 0)
		{
			unsigned int err = mysql_errno(&m_Conn);
			if (err == CR_SERVER_LOST || err == CR_CONN_HOST_ERROR || err == CR_SERVER_GONE_ERROR)
			{
				mysql_ping(&m_Conn);
				if (mysql_real_query(&m_Conn, Sql.c_str(), Sql.size()) != 0)
				{
					return -1;
				}
			}
			else return -1;
		}

		return (INT64)mysql_affected_rows(&m_Conn);
	}

	bool MysqlConnection::Execute(const string& Sql)
	{
		if (mysql_real_query(&m_Conn, Sql.c_str(), Sql.size()) != 0)
		{
			unsigned int err = mysql_errno(&m_Conn);
			if (err == CR_SERVER_LOST || err == CR_CONN_HOST_ERROR || err == CR_SERVER_GONE_ERROR)
			{
				mysql_ping(&m_Conn);
				if (mysql_real_query(&m_Conn, Sql.c_str(), Sql.size()) != 0)
				{
					return false;
				}
			}
			else {
				return false;
			}
		}

		// 因为执行了多条语句，所以要释放每条语句的结果集
		int iStatus = 0;
		do {
			MYSQL_RES* res = mysql_store_result(&m_Conn);
			if (res == NULL) {
				if (mysql_field_count(&m_Conn) != 0) {
					throw std::runtime_error("should have returned data");
				}
			}
			mysql_free_result(res);
			if ((iStatus = mysql_next_result(&m_Conn)) > 0) {
				throw std::runtime_error("Could not execute statement, error: " + string(mysql_error(&m_Conn)) + ", sql: " + Sql);
			}
		} while (iStatus == 0);

		return true;
	}

	bool MysqlConnection::TableMigration(const string&& TableName, std::vector<St_MysqlFieldMigration> vField) 
	{
		// 没有字段不创建
		if (vField.size() <= 0)
			return true;
		int PrimeNum = 0;
		for (auto it : vField)
		{
			if (it.IsPrimeKey)
			{
				PrimeNum++;
				if (PrimeNum > 1){
					return false;
				}
			}
		}

		// 创建表
		{
			string HasTableSql = "SELECT EXISTS ( SELECT 1 FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + TableName + " ') AS table_exists";
			auto HasTable1 = Query(HasTableSql);
			if (!HasTable1 || !HasTable1->NextRow() || HasTable1->GetField("table_exists").GetInt32Type() <= 0)
			{
				auto TmpRet = Query("CREATE TABLE " + TableName + " (Tempppppppppp INT) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci; ");
				FreeResultSet(TmpRet);

				auto HasTable2 = Query(HasTableSql);
				if (!HasTable2 || !HasTable2->NextRow() || HasTable2->GetField("table_exists").GetInt32Type() <= 0) {
					FreeResultSet(HasTable2);
					return false;
				}
				FreeResultSet(HasTable2);
			}
			FreeResultSet(HasTable1);
		}

		// 遍历字段创建
		for (auto it : vField)
		{
			string HasvFieldSql = "SELECT EXISTS ( SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + TableName + "' AND COLUMN_NAME = '" + it.Name + "') AS column_exists";
			auto HasField1 = Query(HasvFieldSql);
			if (HasField1 && HasField1->NextRow() && HasField1->GetField("column_exists").GetInt32Type() > 0)
			{
				if (HasField1)
					FreeResultSet(HasField1);
				continue;
			}
			if (HasField1)
				FreeResultSet(HasField1);

			string AddSql = "ALTER TABLE " + TableName + " ADD COLUMN " + it.Name + " " + GetMysqlTypeByFieldMigration(it) + " ";
			if (it.IsPrimeKey)	AddSql += "PRIMARY KEY ";
			if (it.IsIncr)		AddSql += "AUTO_INCREMENT ";
			if (it.IsNull)		AddSql += "NOT NULL  ";

			auto TmpRet = Query(AddSql);
			FreeResultSet(TmpRet);
			auto HasField2 = Query(HasvFieldSql);
			if (!HasField2 || !HasField2->NextRow() || HasField2->GetField("column_exists").GetInt32Type() <= 0)
			{
				FreeResultSet(HasField2);
				return false;
			}
			if (HasField2)
				FreeResultSet(HasField2);
		}

		// 检查字段类型是否有修改
		for (auto it : vField)
		{
			string HasvFieldSql = "SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + TableName + "' AND COLUMN_NAME = '" + it.Name + "';";
			auto HasField1 = Query(HasvFieldSql);
			if (HasField1 && HasField1->NextRow())
			{
				string vNewType = GetMysqlTypeByFieldMigration(it);
				string OldStrType = HasField1->GetField("COLUMN_TYPE").GetString();
				std::transform(OldStrType.begin(), OldStrType.end(), OldStrType.begin(), [](unsigned char c) { return std::toupper(c); });
				if (vNewType == OldStrType)
				{
					if (HasField1)
						FreeResultSet(HasField1);
					continue;
				}

				string AlterTypeSql = "ALTER TABLE " + TableName + " MODIFY COLUMN " + it.Name + " " + vNewType;
				auto ChangeRet = Query(AlterTypeSql);
				if (ChangeRet)
					FreeResultSet(HasField1);
			}

			if (HasField1)
				FreeResultSet(HasField1);
		}

		// 删除临时字段
		string HasTmpSql = "SELECT EXISTS ( SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = '" + TableName + "' AND COLUMN_NAME = 'Tempppppppppp') AS column_exists";
		auto HasTmp = Query(HasTmpSql);
		if (HasTmp && HasTmp->NextRow() && HasTmp->GetField("column_exists").GetInt32Type() > 0)
		{
			string DropTmoColSql = "ALTER TABLE " + TableName + " DROP COLUMN Tempppppppppp";
			auto TmpRet = Query(DropTmoColSql);
			if (TmpRet)
				FreeResultSet(TmpRet);
		}
	}


	string MysqlConnection::GetMysqlTypeByFieldMigration(St_MysqlFieldMigration it)
	{
		switch (it.Type)
		{
		case St_MysqlFieldMigration::FieldType_IntTiny:			return "TINYINT"; 
		case St_MysqlFieldMigration::FieldType_IntSmall:		return "SMALLINT";
		case St_MysqlFieldMigration::FieldType_IntMeduim:		return "MEDIUMINT"; 
		case St_MysqlFieldMigration::FieldType_Int32:			return "INT";
		case St_MysqlFieldMigration::FieldType_Ite64:			return "BIGINT";
		case St_MysqlFieldMigration::FieldType_Float:			return "FLOAT(" + std::to_string(it.Parm1) + "," + std::to_string(it.Parm2) + ")";
		case St_MysqlFieldMigration::FieldType_Double:			return "DOUBLE(" + std::to_string(it.Parm1) + "," + std::to_string(it.Parm2) + ")";
		case St_MysqlFieldMigration::FieldType_Ddeimal:			return "DECIMAL(" + std::to_string(it.Parm1) + "," + std::to_string(it.Parm2) + ")"; 
		case St_MysqlFieldMigration::FieldType_Date:			return "DATE";
		case St_MysqlFieldMigration::FieldType_Time:			return "TIME";
		case St_MysqlFieldMigration::FieldType_DateTime:		return "DATETIME";
		case St_MysqlFieldMigration::FieldType_Char:			return "CHAR(" + std::to_string(it.Parm1) + ")";
		case St_MysqlFieldMigration::FieldType_VarChar:			return "VARCHAR(" + std::to_string(it.Parm1) + ")";
		case St_MysqlFieldMigration::FieldType_Binary:			return "BINARY(" + std::to_string(it.Parm1) + ")";
		case St_MysqlFieldMigration::FieldType_VarBinary:		return "VARBINARY(" + std::to_string(it.Parm1) + ")";
		case St_MysqlFieldMigration::FieldType_TextTiny:		return "TINYTEXT";
		case St_MysqlFieldMigration::FieldType_Text:			return "TEXT";
		case St_MysqlFieldMigration::FieldType_TextMediun:		return "MEDIUMTEXT";
		case St_MysqlFieldMigration::FieldType_TextLong:		return "LONGTEXT";
		case St_MysqlFieldMigration::FieldType_BlobTiny:		return "TINYBLOB";
		case St_MysqlFieldMigration::FieldType_Blob:			return "BLOB";
		case St_MysqlFieldMigration::FieldType_BlobMediun:		return "MEDIUMBLOB";
		case St_MysqlFieldMigration::FieldType_BlobLong:		return "LONGBLOB";
		}
	}

	UINT64 MysqlConnection::GetInsertID()
	{
		return mysql_insert_id(&m_Conn);
	}


	bool MysqlConnection::SetCharSet(const char* csname)
	{
		if (csname == nullptr)
			return false;
		return mysql_set_character_set(&m_Conn, csname) == 0;
	}

	bool MysqlConnection::BeginTransaction()
	{
		if (!m_bAutoCommit)
			return true;
		if (mysql_autocommit(&m_Conn, 0) != 0)
			return false;
		m_bAutoCommit = false;
		return true;
	}

	void MysqlConnection::Commit()
	{
		if (m_bAutoCommit)
			return;
		mysql_commit(&m_Conn);
		if (mysql_autocommit(&m_Conn, 1) == 0)
			m_bAutoCommit = true;
	}

	void MysqlConnection::Rollback()
	{
		if (m_bAutoCommit)
			return;
		mysql_rollback(&m_Conn);
		if (mysql_autocommit(&m_Conn, 1) == 0)
			m_bAutoCommit = true;
	}

	std::string DBField::GetString() const { if (0 == m_Length) return ""; return std::string(m_Buffer, m_Length); }

	INT8 DBField::GetInt8Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<INT8>(m_Buffer); }
	UINT8 DBField::GetUInt8Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<UINT8>(m_Buffer); }
	INT16 DBField::GetInt16Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<INT16>(m_Buffer); }
	UINT16 DBField::GetUInt16Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<UINT16>(m_Buffer); }
	INT32 DBField::GetInt32Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<INT32>(m_Buffer); }
	UINT32 DBField::GetUInt32Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<UINT32>(m_Buffer); }
	INT64 DBField::GetInt64Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<INT64>(m_Buffer); }
	UINT64 DBField::GetUInt64Type() const { if (0 == m_Length) return 0; return ConvertStringToValue<UINT64>(m_Buffer); }
	FLOAT32 DBField::GetReal32Type() const { if (0 == m_Length) return 0; return (FLOAT32)ConvertStringToValue<FLOAT32>(m_Buffer); }
	FLOAT64 DBField::GetReal64Type() const { if (0 == m_Length) return 0; return (FLOAT64)ConvertStringToValue<FLOAT64>(m_Buffer); }

	size_t DBField::GetBinary(char* buf, size_t buflen) const
	{
		if (0 == m_Length) return 0;
		size_t len = buflen < m_Length ? buflen : m_Length;
		memmove(buf, m_Buffer, len);
		return len;
	}

	MysqlResultSet::MysqlResultSet(MYSQL_RES* res, int rows)
	{
		m_pResult = res;
		m_FieldNum = mysql_num_fields(m_pResult);
		m_pFields = mysql_fetch_fields(m_pResult);
		m_Row = nullptr;
		m_FieldLen = nullptr;
		m_rows = rows;
	}
	MysqlResultSet::~MysqlResultSet() { mysql_free_result(m_pResult); }

	bool MysqlResultSet::NextRow()
	{
		m_Row = mysql_fetch_row(m_pResult);
		m_FieldLen = mysql_fetch_lengths(m_pResult);
		return m_Row != nullptr;
	}

	const DBField MysqlResultSet::MysqlResultSet::GetField(int index) const
	{
		if (index < 0 || index >= m_FieldNum || nullptr == m_Row)
		{
			return DBField();
		}

		DBField::FiledType type = DBField::INTEGER;
		switch (m_pFields[index].type)
		{
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_NEWDECIMAL:
		case MYSQL_TYPE_FLOAT:
		case MYSQL_TYPE_DOUBLE:
			type = DBField::FLOAT;
			break;
		case MYSQL_TYPE_STRING:
		case MYSQL_TYPE_VAR_STRING:
			type = DBField::STRING;
			break;
		case MYSQL_TYPE_BLOB:
		case MYSQL_TYPE_TINY_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
			type = DBField::BINARY;
			break;
		default:
			type = DBField::INTEGER;
			break;
		}

		return DBField(type, m_Row[index], m_FieldLen[index]);
	}

	const DBField MysqlResultSet::MysqlResultSet::GetField(const char* name) const
	{
		if (nullptr == m_Row) return DBField();

		for (int i = 0; i < m_FieldNum; ++i)
		{
			if (strncmp(m_pFields[i].name, name, 64) == 0)
			{
				return GetField(i);
			}
		}
		return DBField();
	}

	int	MysqlResultSet::GetFieldNum() const
	{
		return m_FieldNum;
	}
	std::string MysqlResultSet::GetFieldName(int index) const
	{
		if (index < 0 || index >= m_FieldNum)
			return "";
		return m_pFields[index].name;
	}

	int MysqlResultSet::GetRows()
	{
		return m_rows;
	}

	void AsyncQueryDBCmd::ExecuteSql(MysqlConnection& pConn)
	{
		m_pResultSet = pConn.Query(m_Sql);
		if (m_pResultSet == nullptr)
			m_Error = pConn.GetError();
	}

	void AsyncQueryDBCmd::Callback(MysqlConnection& pConn)
	{
		m_Func(m_pResultSet, m_Error);
		pConn.FreeResultSet(m_pResultSet);
		m_pResultSet = nullptr;
	}

	void AsyncUpdateDBCmd::ExecuteSql(MysqlConnection& pConn)
	{
		m_UpdateResult = pConn.Update(m_Sql);
		if (m_UpdateResult < 0)
			m_Error = pConn.GetError();
		else
			m_InsertKey = pConn.GetInsertID();
	}

	void AsyncUpdateDBCmd::Callback(MysqlConnection& pConn)
	{
		m_Func(m_UpdateResult, m_InsertKey, m_Error);
	}

	bool AsyncMysqlConnection::Open(St_MysqlConf Conf)
	{
		if (m_bWorking) return false;

		if (!m_DBConn.Open(Conf)) return false;

		m_bWorking = true;
		m_WorkerThread = std::thread(Work, this);

		return true;
	}

	void AsyncMysqlConnection::Close()
	{
		if (m_bWorking)
		{
			m_bWorking = false;
			m_WorkerThread.join();
		}

		m_AsyncDBCmdMutex.lock();

		while (!m_AsyncDBCmdQueue.empty())
		{
			auto pCmd = m_AsyncDBCmdQueue.front();
			m_AsyncDBCmdQueue.pop();
			pCmd->Callback(m_DBConn);
			delete pCmd;

			pCmd = m_AsyncDBCmdQueue.front();
		}

		m_AsyncDBCmdMutex.unlock();

		ExecuteCallbacks();

		m_DBConn.Close();
	}

	size_t AsyncMysqlConnection::EscapeString(char* to, const char* from, size_t len)
	{
		return m_DBConn.EscapeString(to, from, len);
	}

	void AsyncMysqlConnection::Work(AsyncMysqlConnection* pConn)
	{
		while (pConn->m_bWorking)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			while (true) //执行所有队列任务
			{
				AsyncDBCmd* pCmd = nullptr;
				pConn->m_AsyncDBCmdMutex.lock();
				if (!pConn->m_AsyncDBCmdQueue.empty())
				{
					pCmd = pConn->m_AsyncDBCmdQueue.front();
					pConn->m_AsyncDBCmdQueue.pop();
				}
				pConn->m_AsyncDBCmdMutex.unlock();

				if (pCmd == nullptr) break;

				pCmd->ExecuteSql(pConn->m_DBConn);

				pConn->m_AsyncDBCallbackMutex.lock();
				pConn->m_AsyncDBCallbackQueue.push(pCmd);
				pConn->m_AsyncDBCallbackMutex.unlock();
			}
		}
	}

	void AsyncMysqlConnection::ExecuteCallbacks()
	{
		while (true)
		{
			AsyncDBCmd* pCmd = nullptr;
			m_AsyncDBCallbackMutex.lock();
			if (!m_AsyncDBCallbackQueue.empty())
			{
				pCmd = m_AsyncDBCallbackQueue.front();
				m_AsyncDBCallbackQueue.pop();
			}
			m_AsyncDBCallbackMutex.unlock();

			if (pCmd == nullptr)
				break;

			pCmd->Callback(m_DBConn);
			delete pCmd;
		}
	}

	bool AsyncMysqlConnection::AsyncQuery(const AsyncQueryCallbackFunc& func, const string sql)
	{
		if (!m_bWorking)
			return false;

		auto pCmd = new AsyncQueryDBCmd(func, sql);
		m_AsyncDBCmdMutex.lock();
		m_AsyncDBCmdQueue.push(pCmd);
		m_AsyncDBCmdMutex.unlock();

		return true;
	}

	bool AsyncMysqlConnection::AsyncUpdate(const AsyncUpdateCallbackFunc& func, const string sql)
	{
		if (!m_bWorking)
			return false;

		auto pCmd = new AsyncUpdateDBCmd(func, sql);
		m_AsyncDBCmdMutex.lock();
		m_AsyncDBCmdQueue.push(pCmd);
		m_AsyncDBCmdMutex.unlock();

		return true;
	}
}
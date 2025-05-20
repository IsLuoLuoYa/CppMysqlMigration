
#ifndef _CPP_MYSQL_H_
#define _CPP_MYSQL_H_

#define _CRT_SECURE_NO_WARNINGS
/*

sudo apt-get install mysql-server
sudo apt-get isntall mysql-client
sudo apt-get install libmysqlclient-dev

��Ĭ������ alter user 'root'@'localhost' identified with mysql_native_password by '123456';   

����Զ������mysql
1.����ǽ���Ŷ˿ڣ�	ufw allow 3306
2.�����ļ��޸ģ�/etc/mysql/mysql.conf.d/mysqld.cnf
	port = 3306							// ��һ��û��Ҫ���ϻ���ȡ��ע��
	# bind-address = 127.0.0,1			// ��һ��Ҫע��
	skip_ssl							// mysql8.0,��һ��û��Ҫ����
3.
	mysql -uroot -p								// ��¼
	use mysql									// �����
	select user, host, plugin from user
	update user set host='%' where user='root'								// ��������ip�ɷ���
	update user set plugin='mysql_native_password' where user='root'		// Զ������������֤��ʽ
	grant all privileges on *.* to 'root'@'%'	// ��Ȩ

	//	����򴴽�һ��localhost��root�������Լ��޷�������
	INSERT INTO `mysql`.`user` (`Host`, `User`, `Select_priv`, `Insert_priv`, `Update_priv`, `Delete_priv`, `Create_priv`, `Drop_priv`, `Reload_priv`, `Shutdown_priv`, `Process_priv`, `File_priv`, `Grant_priv`, `References_priv`, `Index_priv`, `Alter_priv`, `Show_db_priv`, `Super_priv`, `Create_tmp_table_priv`, `Lock_tables_priv`, `Execute_priv`, `Repl_slave_priv`, `Repl_client_priv`, `Create_view_priv`, `Show_view_priv`, `Create_routine_priv`, `Alter_routine_priv`, `Create_user_priv`, `Event_priv`, `Trigger_priv`, `Create_tablespace_priv`, `ssl_type`, `ssl_cipher`, `x509_issuer`, `x509_subject`, `max_questions`, `max_updates`, `max_connections`, `max_user_connections`, `plugin`, `authentication_string`, `password_expired`, `password_last_changed`, `password_lifetime`, `account_locked`, `Create_role_priv`, `Drop_role_priv`, `Password_reuse_history`, `Password_reuse_time`, `Password_require_current`, `User_attributes`) VALUES ('localhost', 'root', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', 'Y', '', '', '', '', '0', '0', '0', '0', 'mysql_native_password', '*6BB4837EB74329105EE4568DDA7DC67ED2CA2AD9', 'N', '2023-08-31 22:39:12', NULL, 'N', 'Y', 'Y', NULL, NULL, NULL, NULL);
	alter user 'root'@'localhost' identified with mysql_native_password by '123456';  
	//

	flush privileges							// ˢ��
	exit
4.
	service mysql stop
	service mysql start
*/

/*
winodws:	����-��������-������-�����ӿ�Ŀ¼-���Ѷ�Ӧ�汾libĿ¼����
linux:		����-��������-������-������������-�� -lmysqlclient;-lpthread;
*/

#ifdef _WIN64
#include <Winsock2.h>
#pragma comment(lib, "libmysql.lib")
#include "MysqlLib/include/mysql.h"
#include "MysqlLib/include/errmsg.h"
#endif

#ifdef WIN32
#include <Winsock2.h>
#pragma comment(lib, "libmysql.lib")
#include "MysqlLib/include/mysql.h"
#include "MysqlLib/include/errmsg.h"
#endif

#ifndef WIN32
#ifndef _WIN64
#include <sys/socket.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <string>
#include <atomic>
#include <stdexcept>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

using std::string;

namespace Mysql
{
#ifdef WIN32
	class MysqlStart
	{
	public:
		MysqlStart() { mysql_library_init(-1, nullptr, nullptr); };
		~MysqlStart() { mysql_library_end(); }
	};
	MysqlStart Global_MysqlStart;
#endif
	class MysqlResultSet;
	class DBField;

	typedef char INT8;
	typedef unsigned char UINT8;
	typedef short INT16;
	typedef unsigned short UINT16;
	typedef long INT32;
	typedef unsigned long UINT32;
	typedef long long INT64;
	typedef unsigned long long UINT64;
	typedef float FLOAT32;
	typedef double FLOAT64;

	template<typename T>
	inline T ConvertStringToValue(const std::string& str) { return ConvertStringToValue<T>(str.c_str()); }
	template<typename T>
	inline T ConvertStringToValue(const char* str);
	template<> inline INT32 ConvertStringToValue<INT32>(const char* str) { if (str == nullptr) return 0; return atoi(str); }
	template<> inline int ConvertStringToValue<int>(const char* str) { if (str == nullptr) return 0; return atoi(str); }
	template<> inline INT8 ConvertStringToValue<INT8>(const char* str) { if (str == nullptr) return 0; return static_cast<INT8>(ConvertStringToValue<INT32>(str)); }
	template<> inline UINT8 ConvertStringToValue<UINT8>(const char* str) { if (str == nullptr) return 0; return static_cast<UINT8>(ConvertStringToValue<INT32>(str)); }
	template<> inline INT16 ConvertStringToValue<INT16>(const char* str) { if (str == nullptr) return 0; return static_cast<INT16>(ConvertStringToValue<INT32>(str)); }
	template<> inline UINT16 ConvertStringToValue<UINT16>(const char* str) { if (str == nullptr) return 0; return static_cast<UINT16>(ConvertStringToValue<INT32>(str)); }
	template<> inline UINT32 ConvertStringToValue<UINT32>(const char* str) { if (str == nullptr) return 0; return static_cast<UINT32>(atoll(str)); }
	template<> inline unsigned int ConvertStringToValue<unsigned int>(const char* str) { if (str == nullptr) return 0; return static_cast<unsigned int>(atoll(str)); }
	template<> inline INT64 ConvertStringToValue<INT64>(const char* str) { if (str == nullptr) return 0; return atoll(str); }
	template<> inline UINT64 ConvertStringToValue<UINT64>(const char* str) { if (str == nullptr) return 0; UINT64 ret = 0; sscanf(str, "%I64u", &ret); return ret; }
	template<> inline FLOAT32 ConvertStringToValue<FLOAT32>(const char* str) { if (str == nullptr) return 0.f; FLOAT32 ret = 0; sscanf(str, "%f", &ret); return ret; }
	template<> inline FLOAT64 ConvertStringToValue<FLOAT64>(const char* str) { if (str == nullptr) return 0.f; return atof(str); }

	template<typename T>
	inline std::string ConvertValueToString(T val) { char buf[100]; buf[0] = 0; ConvertValueToString(val, buf); return buf; }
	template<typename T>
	inline void ConvertValueToString(T val, char* str);
	template<> inline std::string ConvertValueToString<std::string>(std::string str) { return str; }
	template<> inline std::string ConvertValueToString<const char*>(const char* str) { return str; }
	template<> inline std::string ConvertValueToString<char*>(char* str) { return str; }
	template<> inline void ConvertValueToString<int>(int val, char* str) { snprintf(str, 100, "%d", val); }
	template<> inline void ConvertValueToString<unsigned int>(unsigned int val, char* str) { snprintf(str, 100, "%u", val); }
	template<> inline void ConvertValueToString<INT32>(INT32 val, char* str) { snprintf(str, 100, "%d", val); }
	template<> inline void ConvertValueToString<UINT32>(UINT32 val, char* str) { snprintf(str, 100, "%u", val); }
	template<> inline void ConvertValueToString<INT8>(INT8 val, char* str) { ConvertValueToString<INT32>(val, str); }
	template<> inline void ConvertValueToString<UINT8>(UINT8 val, char* str) { ConvertValueToString<INT32>(val, str); }
	template<> inline void ConvertValueToString<INT16>(INT16 val, char* str) { ConvertValueToString<INT32>(val, str); }
	template<> inline void ConvertValueToString<UINT16>(UINT16 val, char* str) { ConvertValueToString<INT32>(val, str); }
	template<> inline void ConvertValueToString<INT64>(INT64 val, char* str) { snprintf(str, 100, "%I64d", val); }
	template<> inline void ConvertValueToString<UINT64>(UINT64 val, char* str) { snprintf(str, 100, "%I64u", val); }
	template<> inline void ConvertValueToString<FLOAT32>(FLOAT32 val, char* str) { snprintf(str, 100, "%f", val); }
	template<> inline void ConvertValueToString<FLOAT64>(FLOAT64 val, char* str) { snprintf(str, 100, "%lf", val); }

	struct St_MysqlConf
	{
		string		Fun;
		int			Type;				//1ͬ��,2�첽
		string		m_Host;				//��ַ
		UINT32		m_Port = 0;			//�˿�
		string		m_User;				//�û���
		string		m_Passwd;			//����
		string		m_Db;				//���ݿ�
	};

	struct St_MysqlFieldMigration	// ͬ���������ݿ��ֶνṹ
	{
		enum FieldType
		{
			FieldType_IntTiny = 1,
			FieldType_IntSmall = 2,
			FieldType_IntMeduim = 3,
			FieldType_Int32 = 4,
			FieldType_Ite64 = 5,
			FieldType_Float = 6,
			FieldType_Double = 7,
			FieldType_Ddeimal = 8,
			FieldType_Date = 9,
			FieldType_Time = 10,
			FieldType_DateTime = 11,
			FieldType_Char = 12,
			FieldType_VarChar = 13,
			FieldType_Binary = 14,
			FieldType_VarBinary = 15,
			FieldType_TextTiny = 16,
			FieldType_Text = 17,
			FieldType_TextMediun = 18,
			FieldType_TextLong = 19,
			FieldType_BlobTiny = 20,
			FieldType_Blob = 21,
			FieldType_BlobMediun = 22,
			FieldType_BlobLong = 23,
		};

		string		Name;
		FieldType	Type;
		bool		IsNull;
		bool		IsPrimeKey;
		bool		IsIncr;
		INT32		Parm1;			// float, double����������Ҫ��������
		INT32		Parm2;
	};

	class MysqlConnection
	{
	public:
		MysqlConnection();
		~MysqlConnection() { Close(); }

	public:
		// ��ȡ������Ϣ
		const char* GetHost() const		{ return m_Conf.m_Host.c_str();}
		UINT32		GetPort() const		{ return m_Conf.m_Port;	}
		const char* GetUser() const		{ return m_Conf.m_User.c_str();	}
		const char* GetPasswd() const	{ return m_Conf.m_Passwd.c_str(); }
		const char* GetDB() const		{ return m_Conf.m_Db.c_str(); }


		// �򿪹ر�
		bool Open(St_MysqlConf Conf);		// host:Ip��������
		void Close(){ mysql_close(&m_Conn);}

		// ��ѯ�ͷŸ���
		MysqlResultSet* Query(const string& Sql);
		void FreeResultSet(MysqlResultSet*& set) { if (!set)return;  delete set; set = nullptr; }
		INT64 Update(const string& Sql);
		bool Execute(const string& Sql);

		// ��ȡ������ַ�ת��
		const char* GetError(){ return mysql_error(&m_Conn); }
		size_t EscapeString(char* to, const char* from, size_t len) { return mysql_real_escape_string(&m_Conn, to, from, len); }		// to Ŀ���ַ�������������������Ϊԭ���ȵ�2����1, lenԭ�ַ�������

		// ���ݿ�Ǩ��
		bool TableMigration(const string&& TableName, std::vector<St_MysqlFieldMigration> vField);			// �����Ƿ����, �������򴴽�, ͬʱ���ݴ�����ֶ��б����Ƿ�����Щ�ֶ�, û���ֶ�Ҳ����
	private:
		string GetMysqlTypeByFieldMigration(St_MysqlFieldMigration T);								// ����Ҫ���������ͷ���mysql�����ַ���

	public:
		UINT64 GetInsertID();
		bool SetCharSet(const char* csname);
		bool BeginTransaction();
		void Commit();
		void Rollback();

	private:

		MYSQL	m_Conn;						//mysql����
		bool	m_bAutoCommit = 0;			//�Ƿ��Զ��ύ

		St_MysqlConf m_Conf;
	};



	// ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	typedef std::function<void(MysqlResultSet* pResultSet, const std::string& error)> AsyncQueryCallbackFunc;		// 1����� 2������Ϣ
	typedef std::function<void(INT64, UINT64, const std::string& error)> AsyncUpdateCallbackFunc;					// 1Ӱ������,��Ϊִ��ʧ�� 2����id 3������Ϣ

	class AsyncDBCmd
	{
	public:
		AsyncDBCmd(const std::string& sql) :m_Sql(sql) {}
		virtual ~AsyncDBCmd() {}
		virtual void ExecuteSql(MysqlConnection& pConn) = 0;
		virtual void Callback(MysqlConnection& pConn) = 0;

	protected:
		std::string m_Sql;
	};

	class AsyncQueryDBCmd : public AsyncDBCmd
	{
	public:
		AsyncQueryDBCmd(const AsyncQueryCallbackFunc& func, const std::string& sql) :AsyncDBCmd(sql), m_Func(func), m_pResultSet(nullptr) {}
		~AsyncQueryDBCmd() {}

		void ExecuteSql(MysqlConnection& pConn);
		void Callback(MysqlConnection& pConn);

	private:
		AsyncQueryCallbackFunc	m_Func;		//�ص�����
		MysqlResultSet* m_pResultSet;		//��ѯ�����
		std::string	m_Error;				//������Ϣ
	};

	class AsyncUpdateDBCmd : public AsyncDBCmd
	{
	public:
		AsyncUpdateDBCmd(const AsyncUpdateCallbackFunc& func, const std::string& sql) :AsyncDBCmd(sql), m_Func(func), m_UpdateResult(-1), m_InsertKey(0) {}
		~AsyncUpdateDBCmd() {}

		void ExecuteSql(MysqlConnection& pConn);
		void Callback(MysqlConnection& pConn);

	private:
		AsyncUpdateCallbackFunc m_Func;			//�ص�����
		INT64	m_UpdateResult;					//���½��
		UINT64	m_InsertKey;					//��������
		std::string	m_Error;					//������Ϣ
	};

	class AsyncMysqlConnection
	{
	public:
		AsyncMysqlConnection() {};
		~AsyncMysqlConnection() { Close(); }
		bool Open(St_MysqlConf Conf);
		void Close();

		size_t EscapeString(char* to, const char* from, size_t len);		// to Ŀ���ַ�������������������Ϊԭ���ȵ�2����1, lenԭ�ַ�������

		bool AsyncQuery(const AsyncQueryCallbackFunc& func, const string sql);			// �첽��ѯ
		bool AsyncUpdate(const AsyncUpdateCallbackFunc& func, const string sql);		// �첽����

		void ExecuteCallbacks();		// ��Ҫʹ�������е���,work�߳���ֻ����ִ�����,������ص�

	private:
		static void Work(AsyncMysqlConnection* pConn);

		MysqlConnection m_DBConn;			// ����

		std::queue<AsyncDBCmd*>	m_AsyncDBCmdQueue;		// �첽ִ�ж���
		std::mutex				m_AsyncDBCmdMutex;		// �첽ִ����

		std::queue<AsyncDBCmd*>	m_AsyncDBCallbackQueue;	// �첽�ص�����
		std::mutex				m_AsyncDBCallbackMutex;	// �첽�ص���

		std::thread				m_WorkerThread;			// �����߳�
		bool					m_bWorking = false;		// �Ƿ��ڹ���
	};

	class DBField
	{
	public:
		enum FiledType
		{
			INTEGER,	//����
			FLOAT,		//����
			STRING,		//�ַ���
			BINARY,		//������
		};

	public:
		DBField()
		{
			m_Type = BINARY;
			m_Buffer = nullptr;
			m_Length = 0;
		}
		DBField(FiledType type, char* buffer, size_t length) :m_Type(type), m_Buffer(buffer), m_Length(length) {}
		~DBField() {}

	public:

		std::string GetString() const;

		INT8 GetInt8Type() const;
		UINT8 GetUInt8Type() const;
		INT16 GetInt16Type() const;
		UINT16 GetUInt16Type() const;
		INT32 GetInt32Type() const;
		UINT32 GetUInt32Type() const;
		INT64 GetInt64Type() const;
		UINT64 GetUInt64Type() const;

		FLOAT32 GetReal32Type() const;
		FLOAT64 GetReal64Type() const;

		size_t GetBinary(char* buf, size_t buflen) const;

	private:
		FiledType	m_Type;			//��������
		char* m_Buffer;		//����
		size_t		m_Length;		//����
	};

	class MysqlResultSet
	{
	public:
		MysqlResultSet() {};
		MysqlResultSet(MYSQL_RES* res, int rows);
		~MysqlResultSet();

	public:
		bool NextRow();
		int	GetFieldNum() const;
		std::string GetFieldName(int index) const;
		const DBField GetField(int index) const;
		const DBField GetField(const char* name) const;
		int GetRows();
	private:

		MYSQL_RES* m_pResult;		//mysql�����
		MYSQL_ROW		m_Row;			//һ�м�¼
		MYSQL_FIELD* m_pFields;		//�ֶ���Ϣ
		int				m_FieldNum;		//�ֶ���
		unsigned long* m_FieldLen;		//�ֶγ���
		int				m_rows;			//����
	};
}

#endif
/****************************************************************************************
*Copyrights  2014:
*All rights reserved.
*
* Filename:	MySQLDBPool.h	
* Indentifier:		
* Description:			
* Version:		V1.0
* Finished:	2014年11月08日
* History:
******************************************************************************************/
#ifndef __DC_ORACLEDB_POOL_H__
#define __DC_ORACLEDB_POOL_H__

#ifndef EXPORTDLL
#define				EXPORTDLL      
#endif

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h>
#include <cppconn/prepared_statement.h>

#include <queue>
#include <string>
#include <pthread.h>

namespace multidb
{
	/****************************************************************************************
	*@description		锁代理封装类，实现自动释放锁功能
	******************************************************************************************/
	class Lock
	{
	private:
		pthread_mutex_t *lock;
	public:
		Lock(pthread_mutex_t *l){
			lock=l;
			if(pthread_mutex_lock(lock)!=0){
				throw sql::SQLException("SystemError: can't get lock");
			}
		}
		virtual ~Lock(){pthread_mutex_unlock(lock);}
	};

	/****************************************************************************************
	*@description		连接池配置信息
	******************************************************************************************/
	typedef struct {
		int 			min;           	//连接池最小连接数
		int 			max;			//连接池最大连接数
		int 			dbid;			//数据库ID
		int 			port;			//MYSQL数据库连接端口
		char 			hostName[128];	//MYSQL数据库连接主机
		char 			user[32];		//MYSQL数据库连接用户
		char 			pass[32];		//MYSQL数据库连接密码
		char 			schema[32];		//MYSQL数据库连接默认schema
		int 			ctimeout;		//MYSQL数据库连接CONNECT TIMEOUT seconds
		int 			wtimeout;		//MYSQL数据库连接WRITE TIMEOUT seconds
		int 			rtimeout;		//MYSQL数据库连接READ TIMEOUT seconds
		int64_t 		expiretime;		//连接过期时间 单位seconds
	} ConnectOptions;

	/****************************************************************************************
	*@description		MYSQL连接对象(mysql::Connection)封装
	******************************************************************************************/
	class CPPCONN_PUBLIC_FUNC Connection {
	private:
		sql::Connection 			*conn;
		ConnectOptions 				*connOpt;
		bool						autoCommitCache;
	public:
		int64_t 					lastTime;   		//最后一次访问时间
		bool						isErr;
		Connection(ConnectOptions *opt,sql::Connection *con){connOpt=opt;conn=con;autoCommitCache=true;isErr=false;}
		virtual ~Connection() {delete conn;};
		virtual sql::PreparedStatement * prepareStatement(const sql::SQLString& _sql){return conn->prepareStatement(_sql);}
		virtual sql::Statement * createStatement(){return conn->createStatement();}
		virtual const sql::SQLWarning * getWarnings(){return conn->getWarnings();}
		virtual void close() {conn->close();}
		virtual void setSchema(const sql::SQLString& catalog){conn->setSchema(catalog);}
		virtual void setAutoCommit(bool autoCommit){if(autoCommitCache!=autoCommit){autoCommitCache=autoCommit;conn->setAutoCommit(autoCommit);}}
		virtual void commit(){conn->commit();}
		virtual bool isValid(){return conn->isValid();}
		virtual bool reconnect(){return conn->reconnect();}
		virtual ConnectOptions *getConnOpt(){return connOpt;}
	};

	/****************************************************************************************
	*@description		数据库连接池实现（单库）
	******************************************************************************************/
	class MySQLSingleDBPool  
	{
	public:
		/****************************************************************************************
		*@input				connectstr:		数据库连接串。
		*@input				min：连接池最小连接数.
		*@input				max：连接池最大连接数
		*@input				id 数据库ID
		******************************************************************************************/
		MySQLSingleDBPool(const char* connectstr,int min,int max,int id=0);

		/****************************************************************************************
		*@description		维护数据库句柄的缓冲池对象死亡时:进行清除
		*@frequency of call	一个维护数据库句柄的缓冲池对象的实例死亡时
		******************************************************************************************/
		 ~MySQLSingleDBPool();

		/****************************************************************************************
		*@return			初始化成功返回TRUE；失败返回FALSE
		*@description		初始化数据库连接的池对象
		*@frequency of call	第一次使用数据库连接池之前调用,连接串读配置获取
		******************************************************************************************/
		EXPORTDLL bool InitPool();
	
		/****************************************************************************************
		*@return			返回一个数据库连接的指针
		*@description		从池中取出一个数据库连接:注意必须和FreeConnection函数一一对应
		*@frequency of call	需要数据库连接时
		******************************************************************************************/
		EXPORTDLL Connection * GetConnection();

		
		/****************************************************************************************
		*@input				pConn:	一个数据库连接的指针
		*@description		把一个数据库连接放入池中:注意必须和GetConnection函数一一对应
		*@frequency of call	需要释放一个数据库连接时
		******************************************************************************************/
		EXPORTDLL void ReleaseConnect(Connection *pOraConn,bool bConngood =true);

		/****************************************************************************************
		*@description		idle连接管理，检查idle连接是否有效、长时间处理idle状态的连接进行关闭规则处理
		*@frequency of call	定时调用，非线程安全
		******************************************************************************************/
		int CheckConnection();
	
	private:

		/****************************************************************************************	
		*@description		释放缓冲池的对象
		*@frequency of call	不需要缓冲池时
		******************************************************************************************/
		void ReleaseAll();
		
	private:
		sql::mysql::MySQL_Driver 	*m_driver;
		volatile bool 				runing;							 	/** 运行状态 **/
		ConnectOptions 				connOpt;				 			/*数据库连接参数*/		
		pthread_mutex_t 			lock = PTHREAD_MUTEX_INITIALIZER;
		std::queue<Connection*> 	freeQueue;							/**数据库句柄队列**/
		volatile int64_t 			usedConns;							/**已使用数据库句柄数**/
		char connectstr[256];
	};

	/****************************************************************************************
	*@description		支持多个数据库连接池的封装
	******************************************************************************************/
	class MySQLDBPool  
	{
	public:

		/****************************************************************************************
		*@return			缓冲池对象的一个指针**
		*@description		提供一个static的接口:借以实现Singleton模式**
		*@frequency of call	一个维护数据库句柄的缓冲池对象的实例死亡时**
		******************************************************************************************/
		EXPORTDLL static MySQLDBPool *GetMySQLPool(){return &m_Pool;}
		
		/****************************************************************************************
		*@input				数据库ID
		*@input				数据库连接串，格式为：root/1@127.0.0.1:3306/test
		*@input				数据库连接池参数-最小连接数
		*@input				数据库连接池参数-最大连接数
		*@return			成功返回TRUE；失败返回FALSE
		*@description		数据库连接信息注册
		*@frequency of call	指定数据库连接池启动之前调用
		******************************************************************************************/
		EXPORTDLL bool RegistDataBase(int dbid,const char* connectstr,int min,int max);

		/****************************************************************************************
		*@return			成功返回TRUE；失败返回FALSE
		*@description		启动所有数据库连接池
		*@frequency of call	使用数据库连接之前调用
		******************************************************************************************/
		EXPORTDLL bool Startup();
	
		/****************************************************************************************
		*@input				数据库ID
		*@return			返回一个数据库连接的指针**
		*@description		从池中取出一个数据库连接:注意必须和FreeConnection函数一一对应**
		*@frequency of call	需要数据库连接时**
		******************************************************************************************/
		EXPORTDLL Connection * GetConnection(int dbid);

		
		/****************************************************************************************
		*@input				pConn:	一个数据库连接的指针**
		*@input		        bConngood： 当前连接是否有效，如无效则释放当前物理连接并创建新的物理连接进行替换
		*@description		把一个数据库连接放入池中:注意必须和GetConnection函数一一对应**
		*@frequency of call	需要释放一个数据库连接时**
		******************************************************************************************/
		EXPORTDLL void FreeConnection(Connection *pOraConn,bool bConngood =true);

		/****************************************************************************************
		*@description		释放缓冲池的对象**
		*@frequency of call	不需要缓冲池时**
		******************************************************************************************/
		void ReleaseAll();
	
	private:
	    
		MySQLDBPool();

		/****************************************************************************************
		*@description		维护数据库句柄的缓冲池对象死亡时:进行清除
		*@frequency of call	一个维护数据库句柄的缓冲池对象的实例死亡时
		******************************************************************************************/
		 ~MySQLDBPool();

		/****************************************************************************************	
		*@description		连接池维护线程函数一   线程入口
		*@frequency of call	Startup()方法中调用
		******************************************************************************************/
		 static void* PoolCheckThFunc(void * pParam);    //连接池维护线程函数一   线程入口

	private:		
		std::map<int,multidb::MySQLSingleDBPool*> 	poolMap;		/** 多数据库连接池映射 **/
		bool 										runing;			/** 运行状态 **/
		static MySQLDBPool 							m_Pool;
	};

	/****************************************************************************************
	*@description		数据库连接资源(Connection、PreparedStatement、Statement、ResultSet)代理封装类，实现自动释放功能
	******************************************************************************************/
	class ProxStream
	{
	public:
		ProxStream(){m_stmt = NULL;m_res=NULL;m_pstmt= NULL;m_con=NULL;}
		~ProxStream()
		{
			if(m_stmt)
				delete m_stmt ;
			if(m_res)
				delete m_res;
			if(m_pstmt)
				delete m_pstmt;

			if(m_con)
			{
				MySQLDBPool::GetMySQLPool()->FreeConnection(m_con);
			}

		}

	public:
		sql::Statement 			*m_stmt;
		sql::PreparedStatement 	*m_pstmt;
		sql::ResultSet 			*m_res; 
		Connection 				*m_con;
	};

};	
	
#endif 



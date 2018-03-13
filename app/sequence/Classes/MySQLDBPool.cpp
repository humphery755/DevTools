/****************************************************************************************
*Copyrights  2014，
*All rights reserved.
*
* Filename：	MySQLDBPool.cpp
* Indentifier：		
* Description：			
* Version：		V1.0
* Finished：	2014年11月08日
* History:
******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <glog/logging.h>
#include "MySQLDBPool.h"
using namespace std;

multidb::MySQLDBPool multidb::MySQLDBPool::m_Pool;
pthread_mutex_t pool_lock = PTHREAD_MUTEX_INITIALIZER;
static int CHECK_INTERVAL=60; //检测时间间隔 120 second

void multidb::MySQLDBPool::updateTime(){
	struct timeval tv;      
	gettimeofday(&tv,NULL);    //该函数在sys/time.h头文件中  
	currentTime = tv.tv_sec;
}

void* multidb::MySQLDBPool::PoolCheckThFunc(void * pParam)
{
	std::map<int,multidb::MySQLSingleDBPool*>::iterator iter;
	int64_t	curTime;
	int interval;//50 second

	multidb::MySQLDBPool* self = (multidb::MySQLDBPool *)pParam;
	self->updateTime();
	//连接池维护线程
	while(self->runing)
	{
		curTime=self->getCurrentTime();
		iter = self->poolMap.begin();
		while(iter != self->poolMap.end())
		{
			iter->second->CheckConnection();
			iter++;	
			self->updateTime();
		}
		interval = CHECK_INTERVAL-(self->getCurrentTime()-curTime);
		for(int i=0;i<interval;i++){
			sleep(1);
			self->updateTime();
		}
	}
	self->updateTime();
	return NULL;
}


multidb::MySQLDBPool::MySQLDBPool()
{
	runing=false;
	pthread_mutex_init (&pool_lock, NULL);	
}


multidb::MySQLDBPool::~MySQLDBPool()
{
	ReleaseAll();
	pthread_mutex_destroy(&pool_lock);
}

void multidb::MySQLDBPool::ReleaseAll()
{
	multidb::MySQLSingleDBPool* spool;
	//std::map<int,multidb::MySQLSingleDBPool*>::iterator iter;
	runing=false;
    try
    {
		Lock l(&pool_lock);
		runing=false;
		while(!poolMap.empty())
		{
			spool = poolMap.begin()->second;
			delete spool;
			poolMap.erase(poolMap.begin());
		}
    }
    catch(...)
    {
        LOG(ERROR)  << "ReleaseAll catch unknow exception";
    }
}
 

bool multidb::MySQLDBPool::RegistDataBase(int dbid,const char* connectstr,int min,int max)
{
	Lock l(&pool_lock);
	/**已经存在的dbid不能再次初始化连接**/
	std::map<int,multidb::MySQLSingleDBPool*>::iterator iter;
	iter = poolMap.find(dbid);
	if(iter != poolMap.end())
	{
		LOG(ERROR)  << "db["<<dbid<<" Already initialized";
		return false;
	}

	multidb::MySQLSingleDBPool *spool=new multidb::MySQLSingleDBPool(connectstr,min,max,dbid);
	poolMap.insert(pair<int,multidb::MySQLSingleDBPool*>(dbid,spool));
	return true;
}

bool multidb::MySQLDBPool::Startup(){
	std::map<int,multidb::MySQLSingleDBPool*>::iterator iter;	
	updateTime();
	try
    {
		Lock l(&pool_lock);
		if(runing)return false;
		runing=true;
		iter = poolMap.begin();
		while(iter != poolMap.end())
		{			
			if(!iter->second->InitPool())return false;
			++iter;
		}
    }catch(sql::SQLException& e)
	{
		LOG(ERROR)  << "exception error[" << e.getErrorCode() << "] errmsg["<< e.what()<<"]";
		return false;
	}
    catch(...)
    {
        LOG(ERROR)  << "ReleaseAll catch unknow exception";
		return false;
    }
	//启动连接池维护线程
	pthread_t localThreadId;
	if ( pthread_create( &localThreadId, NULL, PoolCheckThFunc, this ) != 0 )
	{
		LOG(ERROR)  << "pthread_create failed";
		return false;
	}
	pthread_detach(localThreadId);
	return true;
}


multidb::Connection* multidb::MySQLDBPool::GetConnection(int dbid)
{
	std::map<int,multidb::MySQLSingleDBPool*>::iterator iter;
	iter = poolMap.find(dbid);
	if(iter == poolMap.end())
	{
		return NULL;
	}
	return iter->second->GetConnection();
}


void multidb::MySQLDBPool::FreeConnection(Connection *pConn,bool bConngood)
{
	std::map<int,multidb::MySQLSingleDBPool*>::iterator iter;
	if (NULL == pConn)
	{
		return;
	}

	iter = poolMap.find(pConn->getConnOpt()->dbid);
	if(iter == poolMap.end())
	{
		try{
			pConn->close();			
		}catch(sql::SQLException& e)
		{
			LOG(ERROR) << "exception error[" << e.getErrorCode() << "] errmsg["<< e.what()<<"]";
		}
		catch (...)
		{
			LOG(ERROR) << "catch unknown exception  failed";
		}
		delete pConn;
		return;
	}
	iter->second->ReleaseConnect(pConn,bConngood);
}

/****************************************************************************************
*@input				连接串
*@output			连接属性
*@return			true succeed , false failed
*@description		拆分数据库连接串
*@frequency of call	 InitConn调用
******************************************************************************************/
static bool spliteStr(const char* constr,multidb::ConnectOptions *connOpt)
{
	const char *p = strchr(constr,'/');
	const char * pp;
	if(p)
	{
		strncpy(connOpt->user, constr , p - constr );
		connOpt->user[p - constr]='\0';
	}else{
		return false;
	}

	pp = ++p;
	p = strchr(p,'@');
	if(p)
	{
		strncpy(connOpt->pass, pp , p - pp );
		connOpt->pass[p - pp]='\0';
	}else{
		return false;
	}

	pp = ++p;
	p = strchr(p,':');
	if(p)
	{
		strncpy(connOpt->hostName, pp , p - pp );
		connOpt->hostName[p - pp]='\0';
	}else{
		return false;
	}

	pp = ++p;
	p = strchr(p,'/');
	if(p)
	{
		char port[32];
		strncpy(port, pp , p - pp );
		port[p - pp]='\0';
		connOpt->port=atoi(port);
	}else{
		return false;
	}

	if(++p)
	{
		strcpy(connOpt->schema, p);
	}else{
		return false;
	}
	LOG(INFO) << "dbcfg parse success: " << connOpt->user << "/******(" << strlen(connOpt->pass) << ")@" << connOpt->hostName << ":"<< connOpt->port << "/" << connOpt->schema;
	return true;
}

multidb::MySQLSingleDBPool::MySQLSingleDBPool(const char* connstr,int min,int max,int id)
{
	pthread_mutex_init (&lock, NULL);
	strcpy(connectstr,connstr);
	memset(&connOpt,'\0',sizeof(connOpt));
	connOpt.dbid = id;
	connOpt.min=min;
	connOpt.max=max;
	connOpt.ctimeout=30;
	connOpt.rtimeout=60;
	connOpt.wtimeout=30;
	connOpt.expiretime=120;
	runing=false;
	usedConns=0;
}

multidb::MySQLSingleDBPool::~MySQLSingleDBPool()
{
	ReleaseAll();
	pthread_mutex_destroy(&lock);
}


void multidb::MySQLSingleDBPool::ReleaseAll()
{
	if(!runing)return;
	Connection* pConn;

	try
	{
		Lock l(&lock);
		runing = false;
		while(!freeQueue.empty())
		{   
			pConn = freeQueue.front();
			freeQueue.pop(); 
			if (pConn)
			{
				pConn->close();
				delete pConn;
				pConn = NULL;            
			}
		}
	}
	catch(...)
	{
		LOG(ERROR) << "ReleaseAll catch unknow exception";
	}
}

/****************************************************************************************
*@see https://dev.mysql.com/doc/connector-cpp/en/connector-cpp-connect-options.html
******************************************************************************************/
inline static void buildConnectOptionsMap(multidb::ConnectOptions *connOpt,sql::ConnectOptionsMap *connMap){
	(*connMap)["hostName"] = connOpt->hostName;
	(*connMap)["userName"] = connOpt->user;
	(*connMap)["password"] = connOpt->pass;
	(*connMap)["schema"] = connOpt->schema;
	(*connMap)["port"] = connOpt->port;
	(*connMap)["OPT_RECONNECT"] = true;   //@TODO:: 尚未验证
	(*connMap)["OPT_CONNECT_TIMEOUT"] = connOpt->ctimeout;
	(*connMap)["OPT_READ_TIMEOUT"] = connOpt->rtimeout;
	(*connMap)["OPT_WRITE_TIMEOUT"] = connOpt->wtimeout;
}

bool multidb::MySQLSingleDBPool::InitPool()
{
	Lock l(&lock);
	if(runing)return false;	
	if(!spliteStr(connectstr,&connOpt)){
		LOG(ERROR) << "dbcfg parse failure: " << connOpt.user << "/******(" << strlen(connOpt.pass) << ")@" << connOpt.hostName << ":"<< connOpt.port << "/" << connOpt.schema;
		return false;
	}
	if(connOpt.min<1){
		LOG(ERROR) << "dbcfg minSize < 1";
		return false;
	}
	if(connOpt.min>connOpt.max){
		LOG(ERROR) << "dbcfg minSize > maxSize";
		return false;
	}

	m_driver = sql::mysql::get_mysql_driver_instance();

	sql::ConnectOptionsMap 		connMap;
	buildConnectOptionsMap(&connOpt,&connMap);
	for (int j=1; j<=connOpt.min; j++)
	{
		sql::Connection *conn = m_driver->connect(connMap);
		if(conn == NULL)
		{
			ReleaseAll();
			return false;	
		}
		conn->setAutoCommit(true);
		freeQueue.push(new Connection(&connOpt,conn));
	}
	runing = true;
	return true;

}


multidb::Connection* multidb::MySQLSingleDBPool::GetConnection()
{
	Connection *pconn;
	if(!runing) return NULL; 
	int64_t time = multidb::MySQLDBPool::GetMySQLPool()->getCurrentTime();
	Lock l(&lock);
	if(!freeQueue.empty() ){
		pconn = freeQueue.front();
		freeQueue.pop();
		//pconn->isValid();
		//pconn->reconnect();
		pconn->lastTime = time;
		__sync_add_and_fetch(&usedConns,1);
		return pconn;
	}

	if(usedConns <= connOpt.max){
		sql::ConnectOptionsMap 		connMap;
		buildConnectOptionsMap(&connOpt,&connMap);
		sql::Connection *conn;
		conn = m_driver->connect(connMap); 
		if(conn == NULL)
		{
			return NULL;	
		}
		conn->setAutoCommit(true);
		pconn = new Connection(&connOpt,conn);
		__sync_add_and_fetch(&usedConns,1);
		return pconn;
	}
	LOG(ERROR) << "Cannot get connection: No resources currently available in pool-"<<connOpt.dbid <<" to allocate to applications, please increase the size of the pool and retry..";
	return NULL;
}

void multidb::MySQLSingleDBPool::ReleaseConnect(Connection *pConn,bool bConngood)
{
	if (NULL == pConn)
	{
		return;
	}

	if(runing && bConngood){
		pConn->lastTime = multidb::MySQLDBPool::GetMySQLPool()->getCurrentTime();
		Lock l(&lock);
		freeQueue.push(pConn);
		__sync_add_and_fetch(&usedConns,-1);
		pConn->setAutoCommit(true);
		return;
	}

	pConn->close();
	delete pConn;
	__sync_add_and_fetch(&usedConns,-1);
	return;
}

int multidb::MySQLSingleDBPool::CheckConnection()
{
	Connection *pconn;
	int64_t curTime = multidb::MySQLDBPool::GetMySQLPool()->getCurrentTime();

	std::queue<Connection*> tmpQueue;
	{
		Lock l(&lock);
		int count = freeQueue.size();
		while(count > connOpt.min)
		{
			pconn = freeQueue.front();
			if (pconn==NULL)
			{
				return 0;
			}
			if(curTime - pconn->lastTime < connOpt.expiretime){
				break;
			}
			freeQueue.pop();
			__sync_add_and_fetch(&usedConns,-1);
			tmpQueue.push(pconn);
			--count;
		}
	}
	while(!tmpQueue.empty())
	{
		pconn = tmpQueue.front();
		tmpQueue.pop();
		LOG(INFO) << "connection "<< pconn <<" idle["<<curTime - pconn->lastTime<<"] be closed";
		pconn->close();
		delete pconn;
		pconn = NULL;		
	}
	return 0;
}

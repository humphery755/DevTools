#include<stdlib.h> 
#include<string>
#include <assert.h>
#include <IceBox/IceBox.h>
#include "tdd_sequence.h"
#include <tddl_sequence_SequenceServiceI.h>
#include <glog/logging.h>

#include "MySQLDBPool.h"


using namespace std;
using namespace tddl::sequences;

static int DEFAULT_RETRY_TIMES = 150;
static string DEFAULT_TABLE_NAME = "t_sequence";
static long DELTA = 100000000L;

SequenceDao::SequenceDao()
{
	retryTimes = DEFAULT_RETRY_TIMES;
	tableName = DEFAULT_TABLE_NAME;
	updateSql = "SELECT seq_nextval_v1(?)";
}

//const char * split = (char*)","; 
void SequenceDao::nextRange(string name,SequenceRangeI* out){
	LOG(INFO) << __FUNCTION__ << "(seqName:=" << name << ", SequenceRangeI{value=" << out->value << ",max="<<out->max<<",step="<<out->step << "})";
	if (name == "") {
		LOG(ERROR) << "sequence name can't null";
		throw SequenceException("sequence name can't null");
	}
	//if(1==1)return;
	//从线程池中取出连接（活动连接数＋1）  
	multidb::Connection *con = multidb::MySQLDBPool::GetMySQLPool()->GetConnection(0);
	if(con==NULL){
		LOG(ERROR) << "Failed to create connection for: "<<name;
		throw SequenceException("can't get connection");
	}
	int step = 0;
    try {
		for (int i = 0; i < retryTimes + 1; ++i) {
			string strsql=updateSql;
			//执行SQL语句，返回结果集  
			multidb::ProxStream pstrm;
			pstrm.m_pstmt = con->prepareStatement(strsql); 
			pstrm.m_pstmt->setString(1,name);
			pstrm.m_res =pstrm.m_pstmt->executeQuery();
			int64_t newValue= -1;
			bool isrpc;
			if (pstrm.m_res->next()){
				// str =  value,step,isrpc
				char *p,*str=(char*)pstrm.m_res->getString(1).c_str();

				p = strchr(str,',');
				if(p) {
					*p='\0';
					newValue = atol(str);
					str = ++p;
					p = strchr(p,',');
					if(p) {
						*p='\0';
						step = atol(str);
						if(++p) {
							isrpc = atoi(p);
						}
					}
				}
			}
			if (newValue == -1) {
				stringstream message;
				message << "DBServer busy, Please contact the administrator!";
				LOG(ERROR) << message.str();
				multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con);
				throw SequenceException(message.str());
			}else if (newValue == 0) {
				stringstream message;
				message << "Sequence value cannot be less than zero, value = "<<newValue << ", please check table " << tableName<<" and Sequence name "<<name;
				LOG(ERROR) << message.str();
				multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con);
				throw SequenceException(message.str());
			}else if (newValue < 0x7fffffffffffffffL - DELTA) {
				out->value=newValue;
				//out->value=out->min=newValue;
				out->max=newValue+step;
				out->step = step;
				out->isrpc = isrpc;
				if(isrpc)out->max=newValue;
				multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con);
				return;
			}			

			stringstream message;  
			message << "Sequence value overflow, value = "<<newValue << ", please check table " << tableName<<" and Sequence name "<<name;
			LOG(FATAL)  << message.str();
			multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con);
			throw SequenceException(message.str());
		}
		LOG(ERROR)  << "Retried too many times, retryTimes = " << retryTimes;
		multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con);
	}
	catch(sql::SQLException& ex)
	{
			LOG(ERROR) << "SQLException [" << ex.getErrorCode() << "] errmsg["<< ex.what() <<"]";
			multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con,false);
	}
	//关闭连接（活动连接－1）
	throw SequenceException("System Error");
}


// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//   slice2freeze --dict StringLongMap,string,long StringLongMap
// slice2freeze --dict StringSeqRangeMap,string,tddl::sequences::SeqRange_t StringSeqRangeMap StringSeqRangeMap.ice
// **********************************************************************

#include <Ice/Ice.h>
#include <SequenceServiceI.h>
#include <stdio.h>
#include <string>  
#include <sstream>  
#include <iostream>
#include <glog/logging.h>

#include <city.h>

#include "MySQLDBPool.h"

using namespace std;
using namespace tddl::sequences;


/** 32位序列 - 数据标识id所占的位数,可以部署在32个节点 */
static unsigned int datacenterIdBits32 = 5L;

/** 64位序列 - 数据标识id所占的位数,可以部署在1024个节点*/
static unsigned int datacenterIdBits64 = 10L;

/** 32位序列 - 序列在id中占的位数 */
const static unsigned int sequenceBits32 = 31 - datacenterIdBits32;

/** 64位序列 - 序列在id中占的位数 */
const static unsigned int sequenceBits64 = 61 - datacenterIdBits64;


static long DELTA = 100000000L;
std::string tableName="t_sequence";

inline char *my_strchr(const char *str,char ch)
{
    char *ptr=(char*)str;
    while(*str != '\0')
    {
        if(*ptr == ch)
            return ptr;
        ptr++;
    }
    return 0;
}

//const char * split = (char*)","; 
void nextRange(string name,DefaultSequenceWorker* out){
	//LOG(INFO) << __FUNCTION__ << "(seqName:=" << name << ", SequenceRangeI{value=" << out->value << ",max="<<out->max<<",step="<<out->step << "})";
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
	multidb::ProxStream pstrm;

	int step = 0;
	static string strsql = "SELECT seq_nextval_v2(?)";
    try {
		//执行SQL语句，返回结果集  
	
		pstrm.m_con = con;
		pstrm.m_pstmt = con->prepareStatement(strsql);
		pstrm.m_pstmt->setString(1,name);
		pstrm.m_res =pstrm.m_pstmt->executeQuery();
		int64_t newValue= -1;
		if (pstrm.m_res->next()){
			// str =  value,step,isrpc
			string tmpstr=pstrm.m_res->getString(1);
			char *p,*pstr=(char*)tmpstr.c_str();
			//pstr[tmpstr.size()]='\0';
			p = my_strchr(pstr,',');
			if(p) {
				*p='\0';
				newValue = atol(pstr);
				pstr = ++p;
				if(p) {
					step = atol(pstr);
				}
			}
		}
		if (newValue == -1) {
			stringstream message;
			message << "DBServer busy, Please contact the administrator!";
			LOG(ERROR) << message.str();			
			throw SequenceException(message.str());
		}else if (newValue == 0) {
			stringstream message;
			message << "Sequence value cannot be less than zero, value = "<<newValue << ", please check table " << tableName<<" and Sequence name "<<name;
			LOG(ERROR) << message.str();
			throw SequenceException(message.str());
		}else if (newValue < 0x7fffffffffffffffL - DELTA) {
			out->value=newValue;
			out->max=newValue+step;
			out->step = step;
			return;
		}

		stringstream message;  
		message << "Sequence value overflow, value = "<<newValue << ", please check table " << tableName<<" and Sequence name "<<name;
		LOG(FATAL)  << message.str();
		throw SequenceException(message.str());
	}
	catch(sql::SQLException& ex)
	{
			LOG(ERROR) << "SQLException [" << ex.getErrorCode() << "] errmsg["<< ex.what() <<"]";
			con->isErr=true;
	}
	//关闭连接（活动连接－1）
	throw SequenceException("System Error");
}

DefaultSequenceWorker::DefaultSequenceWorker(string name, unsigned int workerId, int datacenterId,int seqBits):SequenceWorker(name,workerId,datacenterId)
{
	switch(seqBits){
		case 32:
		datacenterIdMask32 = datacenterId << sequenceBits32; //32位序列 - 数据标识向左移22位
		maxDatacenterId = -1 ^ (-1 << datacenterIdBits32);
		//LOG(ERROR) << "datacenterIdMask32: "<<datacenterIdMask32<<" - "<<(datacenterIdMask32>>sequenceBits32) <<" - "<<(maxDatacenterId);
		break;
		case 64:
		datacenterIdMask64=datacenterId;
		datacenterIdMask64 = datacenterIdMask64 << sequenceBits64; //64位序列 - 数据标识向左移57位
		maxDatacenterId = -1 ^ (-1 << datacenterIdBits64);
		//LOG(ERROR) << "datacenterIdMask64: "<<datacenterIdMask64<<" - "<<maxDatacenterId;
		break;
		case 0:
		default:
		maxDatacenterId=0;
		datacenterId=0;
		break;
	}

	if ((unsigned int)datacenterId > maxDatacenterId || datacenterId < 0) {
		stringstream message;  
		message <<"datacenter Id can't be greater than " << maxDatacenterId <<" or less than 0";
		throw SequenceException(message.str());
	}

	this->value=0;
	this->max=0;
	this->step=0;
	this->seqBits=seqBits;

	pthread_rwlock_init(&lock,NULL);

	nextRange(name,this);
}
DefaultSequenceWorker::~DefaultSequenceWorker(){
	pthread_rwlock_destroy(&lock);
}

static inline int addAndIncrement(int step,DefaultSequenceWorker *worker,tddl::sequences::SequenceRange& sr){
	if(worker->value >= worker->max)return -1;
	int64_t currentValue = __sync_add_and_fetch(&worker->value,step);
	if (currentValue > worker->max) {
		if(currentValue-step < worker->max){
			sr.min=currentValue-step+1;
			sr.max=worker->max;
			return 0;
		}
		return -1;
	}	
	sr.min=currentValue-step+1;
	sr.max=currentValue;
	return 0;
}

int DefaultSequenceWorker::getAndIncrement(int step,tddl::sequences::SequenceRange& sr){
	sr.min=0;
	sr.max=0;
	int retv;
	{
		RLock lock(&this->lock);
		retv = addAndIncrement(step,this,sr);
	}	
	if(retv!=0){
		WLock lock(&this->lock);
		if(this->value>=this->max){
			nextRange(name,this);
		}
		retv = addAndIncrement(step,this,sr);
	}

	if(retv==0){
		switch(this->seqBits){
			case 32:
			sr.min=datacenterIdMask32|sr.min;
			sr.max=datacenterIdMask32|sr.max;
			break;
			case 64:
			sr.min=datacenterIdMask64|sr.min;
			sr.max=datacenterIdMask64|sr.max;
			break;
			default:
			break;
		}
		return retv;
	}
	return retv;
}


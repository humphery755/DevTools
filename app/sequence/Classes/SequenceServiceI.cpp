// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//   slice2freeze --dict StringLongMap,string,long StringLongMap
// slice2freeze --dict StringSeqRangeMap,string,tddl::sequences::SeqRange_t StringSeqRangeMap StringSeqRangeMap.ice
// **********************************************************************

#include <Ice/Ice.h>
#include <SequenceServiceI.h>
#include <Freeze/Freeze.h>
#include <stdio.h>
#include <string>  
#include <sstream>  
#include <iostream>
#include <glog/logging.h>

#include <city.h>

#include "MySQLDBPool.h"

using namespace std;
using namespace tddl::sequences;

#define INCREMENT 0
#define INCREMENT32 1
#define INCREMENT64 2
#define SNOW_FLAKE 3

static int retryTimes=5;


static int getAlgorithmCfg(string name){
	multidb::Connection *con = multidb::MySQLDBPool::GetMySQLPool()->GetConnection(0);
	if(con==NULL){
		LOG(ERROR) << "Failed to create connection for: "<<name;
		throw SequenceException("can't get connection");
	}
	multidb::ProxStream pstrm;
	pstrm.m_con = con;
	static string strsql = "select algorithm from t_sequence where name=?";
    try {
			//执行SQL语句，返回结果集  			
			pstrm.m_pstmt = con->prepareStatement(strsql); 
			pstrm.m_pstmt->setString(1,name);
			pstrm.m_res =pstrm.m_pstmt->executeQuery();
			if (pstrm.m_res->next()){
				int algorithm = pstrm.m_res->getInt(1);
				return algorithm;
			}

			stringstream message;
			message << "Sequence("<<name<<") not exists, please check table t_sequence";
			LOG(ERROR) << message.str();
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


SequenceServiceI::SequenceServiceI(unsigned int workerId, int datacenterId)
{
	pthread_rwlock_init(&rangeLock,NULL);
	this->workerId = workerId;
	this->datacenterId=datacenterId;
}

SequenceServiceI::~SequenceServiceI(){
	pthread_rwlock_destroy(&rangeLock);
}

SequenceRange SequenceServiceI::nextValue(const ::std::string& name, ::Ice::Int step, const ::Ice::Current&)
{
	//LOG(INFO) << __FUNCTION__ << "(seqName:=" << name << ", step:=" << step << ")";
	SequenceWorker *currentRange = NULL;
	map<string, SequenceWorker*>::iterator p;
	{
		RLock lock(&this->rangeLock);
		p = rangeMap.find(name);
		if (p != rangeMap.end()) {
			currentRange = (SequenceWorker*)p->second;
		}
	}
	if (currentRange == NULL){
		//每个序列第一次使用时需进行初始化SequenceRangeI，并缓存至rangeMap
		
		//申请写锁
		WLock lock(&this->rangeLock);
		//这里再次进行是否存在判断，确保并发时只会进行一次初始化
		p = rangeMap.find(name);	
		if (p != rangeMap.end()) {
			currentRange = (SequenceWorker*)p->second;
		}
		if (currentRange == NULL){
			int algorithm;
			//获取sequence算法配置
			try{
				algorithm=getAlgorithmCfg(name);
			}catch (SequenceException& e){
				throw e;
			}catch (...) {
				throw SequenceException("UnknownError");
			}
			switch(algorithm){
				case INCREMENT:
					currentRange = new DefaultSequenceWorker(name,0,datacenterId,0);
					break;
				case INCREMENT64:
					currentRange = new DefaultSequenceWorker(name,0,datacenterId,64);
					break;
				case INCREMENT32:
					currentRange = new DefaultSequenceWorker(name,0,datacenterId,32);
					break;
				case SNOW_FLAKE:
					//workerId,datacenterId合二为一
					currentRange = new SnowflakeIdWorker(name,workerId,-1);
					break;
				default:
					LOG(ERROR) << "Unknown Sequence Algorithm config:"<<algorithm;
					throw SequenceException("Unknown Sequence Algorithm config");
			}			
			rangeMap.insert(make_pair(name, currentRange));
		}
	}

	tddl::sequences::SequenceRange retSeqRange;	
	//序列取完后向数据库重新申请序列取值范围，如失败则进行重试
	int i=retryTimes;
	do{
		//告警日志，每超过3即输出一条日志
		int value = currentRange->getAndIncrement(step,retSeqRange);
		if(value==0){
			LOG(INFO) << name << ": SequenceRange{max="<< retSeqRange.max<<", min="<<retSeqRange.min<<"}";
			return retSeqRange;
		}
		VLOG_EVERY_N(1, 3) << name<< ": currentRange->getAndIncrement failed";	
		i--;
	}while(i>=0);
	LOG(ERROR) << name << ": Retried too many times, retryTimes = "<<retryTimes;
	throw SequenceException("Retried too many times");
}

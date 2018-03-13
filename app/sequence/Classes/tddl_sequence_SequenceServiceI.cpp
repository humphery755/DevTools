// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//   slice2freeze --dict StringLongMap,string,long StringLongMap
// slice2freeze --dict StringSeqRangeMap,string,tddl::sequences::SeqRange_t StringSeqRangeMap StringSeqRangeMap.ice
// **********************************************************************

#include <Ice/Ice.h>
#include <tddl_sequence_SequenceServiceI.h>
#include <Freeze/Freeze.h>
#include <stdio.h>
#include <string>  
#include <sstream>  
#include <iostream>
#include <glog/logging.h>

#include <city.h>

using namespace std;
using namespace tddl::sequences;

tddl_sequence_SequenceServiceI::tddl_sequence_SequenceServiceI(const std::string s,IceExt::IceClientUtil *c)
{
	pthread_rwlock_init(&rangeLock,NULL);
	seqDao = new SequenceDao;
	svcName = s;
	clientUtil = c;
	for(int i=0;i<I_SEQ_LOCK_TOTAL;i++){
		pthread_mutex_init(&sequenceLocks[i],NULL);
	}
}

tddl_sequence_SequenceServiceI::~tddl_sequence_SequenceServiceI(){
	pthread_rwlock_destroy(&rangeLock);
	for(int i=0;i<I_SEQ_LOCK_TOTAL;i++){
		pthread_mutex_destroy(&sequenceLocks[i]);
	}
	delete seqDao;
}

SequenceRange tddl_sequence_SequenceServiceI::nextValue(const ::std::string& name, ::Ice::Int step, const ::Ice::Current&)
{
	LOG(INFO) << __FUNCTION__ << "(seqName:=" << name << ", step:=" << step << ")";
	SequenceRangeI *currentRange = NULL;

	if(pthread_rwlock_rdlock(&rangeLock)!=0){
		LOG(ERROR) << "can't get rdlock.";
		throw SequenceException("SystemError");
	}
	map<string, SequenceRangeI*>::iterator p = rangeMap.find(name);	
	if (p != rangeMap.end()) {
		currentRange = p->second;
	}
	pthread_rwlock_unlock(&rangeLock);
	if (currentRange == NULL){
		//每个序列第一次使用时需进行初始化SequenceRangeI，并缓存至rangeMap
		
		//申请写锁
		if(pthread_rwlock_wrlock(&rangeLock)!=0){
			LOG(ERROR) << "can't get wrlock.";
			throw SequenceException("SystemError");
		}
		//这里再次进行是否存在判断，确保并发时只会进行一次初始化
		p = rangeMap.find(name);	
		if (p != rangeMap.end()) {
			currentRange = p->second;
		}
		if (currentRange == NULL){
			currentRange = new SequenceRangeI;
			currentRange->value=1;

			//DB中取得相应配置
			try{
				seqDao->nextRange(name,currentRange);
			}catch (SequenceException& e){
				pthread_rwlock_unlock(&rangeLock);
				throw e;
			}catch (...) {
				pthread_rwlock_unlock(&rangeLock);
				throw SequenceException("UnknownError");
			}
			
			//为提高并行度，采用多个序列锁，理想情况下为每个序列对应一把锁
			char *cname = (char*)name.c_str();
			//散列化序列
			size_t n = CityHash64(cname,strlen(cname));
			//计算出其对应的锁索引位置
			int index = n % I_SEQ_LOCK_TOTAL;
			currentRange->mutex = &sequenceLocks[index];

			rangeMap.insert(make_pair(name, currentRange));
		}
		pthread_rwlock_unlock(&rangeLock);
	}

	//入参校验
	if(step > currentRange->step){
		stringstream message;  
		message <<"'"<<name<<"' step(" << step << ") more than " << currentRange->step;
		throw SequenceException(message.str());
	}

	tddl::sequences::SequenceRange retSeqRange;
	int value = currentRange->getAndIncrement(step,retSeqRange);
	if(value==-1){
		//序列取完后向数据库重新申请序列取值范围

		if(currentRange->isrpc){
			//RPC方式申请序列取值范围
			SequenceServicePrx sequenceServicePrx;
			Ice::ObjectPrx objPrx1 = clientUtil->stringToProxy(svcName);
			if(!objPrx1)
			{
				LOG(ERROR) << "invalid or missing stringToProxy:" << svcName;
				throw SequenceException("rpc configure error");
			}
			//cout << &objPrx1 << endl;
			sequenceServicePrx  = SequenceServicePrx::checkedCast(objPrx1);
			if(!sequenceServicePrx)
			{
				LOG(ERROR) << "SequenceServicePrx::checkedCast error";
				throw SequenceException("rpc configure error");
			}
			tddl::sequences::SequenceRange sr;
			pthread_mutex_lock(currentRange->mutex);
			try
    		{
				sr = sequenceServicePrx->nextValue(name, currentRange->step);
			}catch(const SequenceException& ex)
			{
				pthread_mutex_unlock(currentRange->mutex);
				LOG(ERROR) << ex.reason;
				throw ex;
			}
			//while(!__sync_lock_test_and_set(&currentRange->value,sr.min));
			currentRange->value=sr.min-1;
			currentRange->max=sr.max;
			pthread_mutex_unlock(currentRange->mutex);
			//LOG(INFO) << "step:" << currentRange->step << ", val:" << currentRange->value << ", max:" << currentRange->max;
			currentRange->getAndIncrement(step,retSeqRange);
			//LOG(INFO) << "step:" << step << ", min:" << retSeqRange.min << ", max:" << retSeqRange.max;
		} else {
			//DB方式申请序列取值范围，如失败则进行重试		
			do{	
				//告警日志，每超过10即输出一条日志
				VLOG_EVERY_N(1, 10) << name<< ": currentRange->getAndIncrement failed";
				pthread_mutex_lock(currentRange->mutex);
				try{
					if(currentRange->value >= currentRange->max) {
						seqDao->nextRange(name,currentRange);
					}
				}catch (SequenceException& e){
					LOG(ERROR) << e.reason;
					pthread_mutex_unlock(currentRange->mutex);
					throw e;
				}catch (...) {
					pthread_mutex_unlock(currentRange->mutex);
					LOG(ERROR) << "Unknow Exception";
					throw SequenceException("UnknownError");
				}
				pthread_mutex_unlock(currentRange->mutex);
				value = currentRange->getAndIncrement(step,retSeqRange);
				if(value==-1){
					usleep(1);
				}else{
					break;
				}
			}while (1);
		}
	}
	LOG(INFO) << "return SequenceRange{max="<< retSeqRange.max<<", min="<<retSeqRange.min<<"}";
	return retSeqRange;
}

SequenceRangeI::SequenceRangeI(){
	this->value=0;
	this->max=0;
	//this->min=0;
	this->step=0;
	this->isrpc=false;
}

int SequenceRangeI::getAndIncrement(int step,tddl::sequences::SequenceRange& sr){
	if(this->value>=this->max)return -1;
	int64_t currentValue = __sync_add_and_fetch(&this->value,step);
	if (currentValue > this->max) {
		if(currentValue-step<this->max){
			sr.min=currentValue-step+1;
			sr.max=this->max;
			return 0;
		}
		return -1;
	}	
	sr.min=currentValue-step+1;
	sr.max=currentValue;
	return 0;
}

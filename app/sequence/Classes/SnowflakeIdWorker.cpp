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
#include <sys/time.h>
#include <glog/logging.h>

#include <city.h>

using namespace std;
using namespace tddl::sequences;

/** 开始时间截 (2018-01-01) */
const static int64_t twepoch = 1514764800000;

/** 机器id所占的位数 */
const static unsigned int workerIdBits = 5;

/** 数据标识id所占的位数 */
const static unsigned int datacenterIdBits = 5;

/** 支持的最大机器id，结果是31 (这个移位算法可以很快的计算出几位二进制数所能表示的最大十进制数) */
const static unsigned int maxWorkerId = -1 ^ (-1 << workerIdBits);

/** 支持的最大数据标识id，结果是31 */
const static unsigned int maxDatacenterId = -1 ^ (-1 << datacenterIdBits);

/** 序列在id中占的位数 */
const static unsigned int sequenceBits = 12;

/** 机器ID向左移12位 */
const static unsigned int workerIdShift = sequenceBits;

/** 数据标识id向左移17位(12+5) */
const static unsigned int datacenterIdShift = sequenceBits + workerIdBits;

/** 时间截向左移22位(5+5+12) */
const static unsigned int timestampLeftShift = sequenceBits + workerIdBits + datacenterIdBits;

/** 生成序列的掩码，这里为4095 (0b111111111111=0xfff=4095) */
const static unsigned int sequenceMask = -1 ^ (-1 << sequenceBits);

inline static int64_t timeGen(const int64_t lastTimestamp){
	int64_t timestamp;
	struct timeval tv;
    gettimeofday(&tv,0);
	timestamp=(uint64_t)tv.tv_sec*1000 + (uint64_t)tv.tv_usec/1000;
	//LOG(INFO) << timestamp<<":"<<lastTimestamp;
	if (timestamp < lastTimestamp) {
		LOG(ERROR) << "xxxxxxxxxxxxxxxxxxx: "<< timestamp<<":"<<lastTimestamp;
		
		time_t      tv_sec;
		tv_sec = time(NULL);
		/* Anti race condition sec fix
		* When the milliseconds are at 999 at the time of call to time(), and at 
		* 999+1 = 0 at the time of the GetLocalTime call, then the tv_sec and
		* tv_usec would be set to one second in the past. To correct this, just
		* check if the last decimal digit of the seconds match, and if not, add
		* a second to the tv_sec.
		*/
		
		if (tv.tv_sec % 10 != tv_sec % 10)
			tv.tv_sec++;		

		timestamp=tv.tv_sec*1000 + tv.tv_usec/1000;
		
	}
	return timestamp;
	

	//return tv.tv_sec*1000 + tv.tv_usec/1000;
}

/**
 * 阻塞到下一个毫秒，直到获得新的时间戳
 * @param lastTimestamp 上次生成ID的时间截
 * @return 当前时间戳
 */
inline static int64_t tilNextMillis(const int64_t lastTimestamp) {
	int64_t timestamp;
	do{
		timestamp = timeGen(lastTimestamp);
	} while (timestamp <= lastTimestamp);
	return timestamp;
}

SnowflakeIdWorker::SnowflakeIdWorker(string name, unsigned int workerId, unsigned int datacenterId):SequenceWorker(name,workerId,datacenterId){
	if (workerId > maxWorkerId || workerId < 0) {
		stringstream message;  
		message <<"worker Id can't be greater than " << maxWorkerId <<" or less than 0";
		LOG(ERROR) << message.str();
		throw SequenceException(message.str());
	}
	if (datacenterId > maxDatacenterId || datacenterId < 0) {
		stringstream message;  
		message <<"datacenter Id can't be greater than " << maxDatacenterId <<" or less than 0";
		LOG(ERROR) << message.str();
		throw SequenceException(message.str());
	}

	this->sequence=0;
	this->lastTimestamp=0;
}

SnowflakeIdWorker::~SnowflakeIdWorker(){
}

int SnowflakeIdWorker::getAndIncrement(int step,tddl::sequences::SequenceRange& sr){
	//入参校验
	if((unsigned int)step > sequenceMask){
		stringstream message;  
		message <<"'"<<name<<"' step(" << step << ") more than " << sequenceMask;
		LOG(ERROR) << message.str();
		throw SequenceException(message.str());
	}
	//sr.min2 = 0;
	//sr.max2 = 0;
	int64_t timestamp,timestamp1,tmpSeq;
	int minSeq,maxSeq,min2Seq=-1;//,max2Seq;
	{
		Lock(&this->lock);
		//do{
			timestamp = timeGen(lastTimestamp);
		//} while (timestamp < lastTimestamp);
		//timestamp = timeGen(lastTimestamp);
		//如果当前时间小于上一次ID生成的时间戳，说明系统时钟回退过这个时候应当抛出异常
		if (timestamp < lastTimestamp) {
			stringstream message;  
			//clock_gettime(CLOCK_MONOTONIC, &this->tpend);
			message <<"Clock moved backwards.  Refusing to generate id for " << lastTimestamp - timestamp <<" milliseconds:" <<timestamp<<":" <<lastTimestamp;
			LOG(ERROR) << message.str();
			throw SequenceException(message.str());
		}

		//如果是同一时间生成的，则进行毫秒内序列
		if (lastTimestamp == timestamp) {
			minSeq= sequence+1;
			sequence = (sequence + step) & sequenceMask;
			//毫秒内序列溢出
			if (sequence == 0) {
				//阻塞到下一个毫秒,获得新的时间戳
				timestamp = tilNextMillis(lastTimestamp);
				if((minSeq&sequenceMask)==0){
					minSeq=0;
					sequence = step-1;
					maxSeq= sequence;
					timestamp1 = timestamp;
					LOG(INFO) << "timestamp="<< timestamp1<<": min="<<minSeq<<", max="<<maxSeq;
				}else{
					maxSeq= sequenceMask-1;
					timestamp1 = lastTimestamp;
					sequence = step;
					min2Seq = 0;
					LOG(INFO) << "timestamp="<< timestamp1<<": min="<<minSeq<<", max="<<maxSeq;
					//max2Seq = step;
				}				
			}else{
				timestamp1 = timestamp;
				maxSeq = sequence;
				LOG(INFO) << "timestamp="<< timestamp1<<": min="<<minSeq<<", max="<<maxSeq;
			}
		}
		//时间戳改变，毫秒内序列重置
		else {
			sequence = 0;
			minSeq = sequence;
			maxSeq = step-1;
			timestamp1 = timestamp;
			LOG(INFO) << "lastTimestamp="<<lastTimestamp<<",timestamp="<< timestamp1<<": min="<<minSeq<<", max="<<maxSeq;
		}
		//上次生成ID的时间截
		lastTimestamp = timestamp;
	}
	//移位并通过或运算拼到一起组成64位的ID
	tmpSeq = ((timestamp1 - twepoch) << timestampLeftShift) //
			| (datacenterId << datacenterIdShift) //
			| (workerId << workerIdShift);
	sr.min= tmpSeq | minSeq;
	sr.max= tmpSeq | maxSeq;
	LOG(INFO) << "tmpSeq="<< tmpSeq<<": min="<<sr.min<<", max="<<sr.max<<",twepoch="<<twepoch<<",timestampLeftShift="<<timestampLeftShift<<",datacenterId="<<datacenterId<<",datacenterIdShift="<<datacenterIdShift<<",workerId="<<workerId<<",workerIdShift="<<workerIdShift;
	if(min2Seq==-1){
		return 0;
	}
	//tmpSeq = ((timestamp - twepoch) << timestampLeftShift)	| (datacenterId << datacenterIdShift) | (workerId << workerIdShift);
	//sr.min2= tmpSeq | min2Seq;
	//sr.max2= tmpSeq | max2Seq;
	return 0;
}

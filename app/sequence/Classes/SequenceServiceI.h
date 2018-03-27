// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#ifndef tdd_sequence_SequenceServiceI_I_H
#define tdd_sequence_SequenceServiceI_I_H

#include <IceBox/IceBox.h>
#include <map>
#include <pthread.h>
#include <glog/logging.h>
#include "tdd_sequence.h"
#include "IceExtClientUtil.h"

extern char version[50];

class Lock
{
private:
	pthread_mutex_t *lock;
public:
	Lock(pthread_mutex_t *l):lock(l){
		if(pthread_mutex_lock(lock)!=0){
			LOG(ERROR) << "can't get lock.";
			throw tddl::sequences::SequenceException("SystemError: can't get lock");
		}
	}
	virtual ~Lock(){pthread_mutex_unlock(lock);}
};

class WLock
{
private:
	pthread_rwlock_t *lock;
public:
	WLock(pthread_rwlock_t *l):lock(l){
		if(pthread_rwlock_wrlock(lock)!=0){
			LOG(ERROR) << "can't get wrlock.";
			throw tddl::sequences::SequenceException("SystemError: can't get rwlock");
		}
	}
	virtual ~WLock(){pthread_rwlock_unlock(lock);}
};

class RLock
{
private:
	pthread_rwlock_t *lock;
public:
	RLock(pthread_rwlock_t *l):lock(l){
		if(pthread_rwlock_rdlock(lock)!=0){
			LOG(ERROR) << "can't get rwlock.";
			throw tddl::sequences::SequenceException("SystemError: can't get rdlock");
		}
	}
	virtual ~RLock(){pthread_rwlock_unlock(lock);}
};

class SequenceWorker{
public:
	/**
     * 构造函数
     * @param workerId 工作ID (0~31)
     * @param datacenterId 数据中心ID (0~31)
     */
	SequenceWorker(std::string name, unsigned int workerId, unsigned int datacenterId):name(name),workerId(workerId),datacenterId(datacenterId){
	}
	virtual ~SequenceWorker(){}
	virtual int getAndIncrement(int step,tddl::sequences::SequenceRange&)=0;
	int 		  		step;

protected:
	/** 序列名称 */
	const std::string name;
    /** 工作机器ID(0~31) */
    const unsigned int workerId;
    /** 数据中心ID(0~31) */
    const unsigned int datacenterId;
};

/**
 * DefaultSequenceWorker(每部分用-分开):
 * 0 - 00000 - 000 00000000 00000000 00000000
 * 1位标识，由于int32_t基本类型是带符号的，最高位是符号位，正数是0，负数是1，所以id一般是正数，最高位是0
 * 5位datacenterId(数据中心标识)，可部署在32个数据中心
 * 27位序列，同一数据中心内的计数，27位的计数顺序号支持每个数据中心产生134217728个ID序号
 * 加起来刚好32位，为一个int型。
 */
class DefaultSequenceWorker : public SequenceWorker{
public:
	DefaultSequenceWorker(std::string name, unsigned int workerId, unsigned int datacenterId,int seqBits);
	virtual ~DefaultSequenceWorker();
	int getAndIncrement(int step,tddl::sequences::SequenceRange&);

	volatile int64_t 	max;
	volatile int64_t 	value;
	/** 生成序列的位数 */
	int 				seqBits;
private:
	/** 数据ID在64位序列中的掩码 */
	uint64_t		datacenterIdMask64;
	/** 数据ID在32位序列中的掩码 */
	uint32_t		datacenterIdMask32;

	/** 支持的最大数据标识id，结果是31 */
	unsigned int 	maxDatacenterId;
	pthread_rwlock_t lock;
};

/**
 * SnowflakeIdWorkerRangeI<br>
 * SnowFlake的结构如下(每部分用-分开):<br>
 * 0 - 0000000000 0000000000 0000000000 0000000000 0 - 00000 - 00000 - 000000000000 <br>
 * 1位标识，由于int64_t基本类型是带符号的，最高位是符号位，正数是0，负数是1，所以id一般是正数，最高位是0<br>
 * 41位时间截(毫秒级)，注意，41位时间截不是存储当前时间的时间截，而是存储时间截的差值（当前时间截 - 开始时间截)
 * 得到的值），这里的的开始时间截，一般是我们的id生成器开始使用的时间，由我们程序来指定的（如下下面程序IdWorker类的startTime属性）。41位的时间截，可以使用69年，年T = (1L << 41) / (1000L * 60 * 60 * 24 * 365) = 69<br>
 * 10位的数据机器位，可以部署在1024个节点，包括5位datacenterId和5位workerId<br>
 * 12位序列，毫秒内的计数，12位的计数顺序号支持每个节点每毫秒(同一机器，同一时间截)产生4096个ID序号<br>
 * 加起来刚好64位，为一个Long型。<br>
 * SnowFlake的优点是，整体上按照时间自增排序，并且整个分布式系统内不会产生ID碰撞(由数据中心ID和机器ID作区分)，并且效率较高，经测试，SnowFlake每秒能够产生26万ID左右。
 */
class SnowflakeIdWorker:public SequenceWorker{
public:
	/**
     * 构造函数
     * @param workerId 工作ID (0~31)
     * @param datacenterId 数据中心ID (0~31)
     */
	SnowflakeIdWorker(std::string name, unsigned int workerId, unsigned int datacenterId);
	virtual ~SnowflakeIdWorker();
	int getAndIncrement(int step,tddl::sequences::SequenceRange&);

private:

    /** 毫秒内序列(0~4095) */
    int sequence;

    /** 上次生成ID的时间截 */
    int64_t lastTimestamp;

	pthread_mutex_t 	lock = PTHREAD_MUTEX_INITIALIZER;
};

class SequenceServiceI : public tddl::sequences::SequenceService
{
public:
	SequenceServiceI(unsigned int workerId, unsigned int datacenterId);
	virtual ~SequenceServiceI();
	virtual tddl::sequences::SequenceRange nextValue(const ::std::string&, ::Ice::Int, const ::Ice::Current&);

private:
	std::map<std::string, SequenceWorker*> rangeMap;
	pthread_rwlock_t rangeLock;
	/** 工作机器ID(0~31) */
    unsigned int workerId;
    /** 数据中心ID(0~31) */
    unsigned int datacenterId;
};

class SequenceServiceIcebox : public ::IceBox::Service
{
public:

	SequenceServiceIcebox();
    virtual void start(const ::std::string&,
                       const ::Ice::CommunicatorPtr&,
                       const ::Ice::StringSeq&);
    virtual void stop();
private:
	::Ice::ObjectAdapterPtr _adapter;	
	Ice::CommunicatorPtr   _communicator;
};

#endif

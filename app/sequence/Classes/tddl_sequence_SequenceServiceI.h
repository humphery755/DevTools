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
#include "tdd_sequence.h"
#include "IceExtClientUtil.h"

#define I_SEQ_LOCK_TOTAL 10

extern char version[50];

class SequenceRangeI{
public:
	SequenceRangeI();
	int getAndIncrement(int step,tddl::sequences::SequenceRange&);
	//int64_t 			min;
	int64_t 			max;
	volatile int64_t 	value;
	int 		  		step;
	bool		  		isrpc;
	pthread_mutex_t 	*mutex;
};


class SequenceDao {
public:
	SequenceDao();
	virtual ~SequenceDao(){}
	virtual void nextRange(const std::string name,SequenceRangeI*);

private:
	int retryTimes;
	std::string tableName;
	std::string updateSql;
};

class tddl_sequence_SequenceServiceI : public tddl::sequences::SequenceService
{
public:
	tddl_sequence_SequenceServiceI(const std::string s,IceExt::IceClientUtil *c);
	virtual ~tddl_sequence_SequenceServiceI();
	virtual tddl::sequences::SequenceRange nextValue(const ::std::string&, ::Ice::Int, const ::Ice::Current&);

private:
	std::map<std::string, SequenceRangeI*> rangeMap;
	pthread_mutex_t sequenceLocks[I_SEQ_LOCK_TOTAL];
	pthread_rwlock_t rangeLock;
	SequenceDao *seqDao;
	IceExt::IceClientUtil *clientUtil;
	std::string svcName;
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

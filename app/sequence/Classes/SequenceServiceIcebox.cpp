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
#include "Toolkits.h"
#include "MySQLDBPool.h"
#include "IceExtClientUtil.h"
#include "OrderSequenceAdapter.h"

using namespace std;
using namespace tddl::sequences;
using namespace IceExt;


extern "C"
{

ICE_DECLSPEC_EXPORT IceBox::Service* create(Ice::CommunicatorPtr)
{
    return new SequenceServiceIcebox;
}

}

//#define ngx_trylock(lock)  (*(lock) == 0 && __sync_bool_compare_and_swap(lock, 0, 1))
//#define ngx_unlock(lock)    *(lock) = 0
char version[50];

SequenceServiceIcebox::SequenceServiceIcebox(){}

static void SignalHandle(const char* data, int size)
{
    std::string str = std::string(data,size);
    LOG(ERROR)<<str;
}

static void initGlog(Ice::PropertiesPtr& prop){
	string logdir = prop->getPropertyWithDefault("glog.logDestination","./logs/");
	int maxLogSize = prop->getPropertyAsIntWithDefault("glog.maxLogSize",1000);
	int logbufsecs = prop->getPropertyAsIntWithDefault("glog.logbufsecs",30);

	google::InitGoogleLogging("sequence");
	ostringstream log_dest_stream;
	log_dest_stream<<logdir<<"info";
	google::SetLogDestination(google::GLOG_INFO,log_dest_stream.str().c_str());
	log_dest_stream.str("");
	log_dest_stream<<logdir<<"warn";
	google::SetLogDestination(google::GLOG_WARNING, log_dest_stream.str().c_str()); 
	log_dest_stream.str("");
	log_dest_stream<<logdir<<"error";
	google::SetLogDestination(google::GLOG_ERROR,log_dest_stream.str().c_str());
	log_dest_stream.str("");
	log_dest_stream<<logdir<<"fatal";
	google::SetLogDestination(google::GLOG_FATAL, log_dest_stream.str().c_str());
	FLAGS_stop_logging_if_full_disk = true; //当磁盘被写满时，停止日志输出
	FLAGS_colorlogtostderr = true; //设置输出到屏幕的日志显示相应颜色
	FLAGS_servitysinglelog = true;// 用来按照等级区分log文件

	FLAGS_logbufsecs = logbufsecs;
    FLAGS_max_log_size = maxLogSize;

	google::InstallFailureSignalHandler();
	google::InstallFailureWriter(&SignalHandle);
}


void
SequenceServiceIcebox::start(const string& _name, const Ice::CommunicatorPtr& communicator, const Ice::StringSeq& args)
{
	Ice::PropertiesPtr prop = communicator->getProperties()->clone();//Ice::createProperties();
	string configFile = prop->getPropertyWithDefault("seq.configFile","conf/sequence.properties");
	try
	{
		prop->load(configFile);
	}catch(const Ice::Exception& ex) {  Ice::Error out(Ice::getProcessLogger());  out <<__FILE__<<":"<< __LINE__<<" Ice::Exception:"<< ex;throw ex;}
	catch(...)
	{
		Ice::Error out(Ice::getProcessLogger());  out <<__FILE__<<":"<< __LINE__<<" unknown exception";
		return;
	}

	initGlog(prop);
	if(!startTookits((void*)0))return;
	
	string strVersion = prop->getPropertyWithDefault("app.ver","");
    strcpy(version,strVersion.c_str());
	
	string strUrl = prop->getPropertyWithDefault("seq.database.driver.url","root/1@127.0.0.1:3306/test");
	int minPoolSize = prop->getPropertyAsInt("seq.database.driver.minPoolSize");
	int maxPoolSize = prop->getPropertyAsInt("seq.database.driver.maxPoolSize");
	LOG(INFO) << "driverUrl = " << strUrl << ", minPoolSize=" << minPoolSize<< ", maxPoolSize=" << maxPoolSize;

	if(!multidb::MySQLDBPool::GetMySQLPool()->RegistDataBase(0,strUrl.c_str(),minPoolSize,maxPoolSize))return;
	LOG(INFO) <<  "MySQLDBPool RegistDataBase success.";
	if(!multidb::MySQLDBPool::GetMySQLPool()->Startup())return;
	LOG(INFO) <<  "MySQLDBPool startup success.";

	_communicator=communicator;	
	ostringstream name_stream;
	name_stream << _name << version;
	LOG(INFO) << "start createAdapter: " << name_stream.str();
	_adapter = communicator->createObjectAdapter(name_stream.str());

	startOrderSequence(communicator, prop);
	int workerId = prop->getPropertyAsInt("seq.workerId");
	int snowflakeWorkerId = prop->getPropertyAsInt("seq.snowflake.workerId");
	int datacenterId = prop->getPropertyAsInt("seq.datacenterId");
	tddl::sequences::SequenceServicePtr seqSvc = new SequenceServiceI(workerId,snowflakeWorkerId,datacenterId);
	_adapter->add(seqSvc, communicator->stringToIdentity(_adapter->getName()));
	LOG(INFO) << "The Adapter:" << _adapter->getName() << " will be activated.";
	_adapter->activate();	
}

void
SequenceServiceIcebox::stop()
{
	stopOrderSequence();
	LOG(INFO) << "The Adapter: " << _adapter->getName() << " be destroy." ;
    _adapter->destroy();
	
	multidb::MySQLDBPool::GetMySQLPool()->ReleaseAll();

	google::ShutdownGoogleLogging();
}

// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//   slice2freeze --dict StringLongMap,string,long StringLongMap
// slice2freeze --dict StringSeqRangeMap,string,tddl::sequences::SeqRange_t StringSeqRangeMap StringSeqRangeMap.ice
// **********************************************************************

#include <Ice/Ice.h>
#include <Freeze/Freeze.h>

#include <stdio.h>
#include <string>  
#include <sstream>  
#include <iostream>
#include <IceBox/IceBox.h>
#include <Glacier2/PermissionsVerifier.h>
#include <Glacier2/Session.h>
#include <glog/logging.h>
#include "dbutil.h"
#include "SessionServer.h"
#include "Callback.h"

using namespace std;
using namespace Test;

extern "C"
{

//
// Factory function

ICE_DECLSPEC_EXPORT IceBox::Service*
create(Ice::CommunicatorPtr)
{
    return new SessionServerIceBox;
}

}


class SSLRouterPermissionsVerifierI: public Glacier2::SSLPermissionsVerifier
{
public:	
	virtual bool authorize(const Glacier2::SSLInfo& info, std::string& reason, const Ice::Current&) const
	{
		LOG(INFO) << "checkSSLPermissions >> userId:" <<", passwd:"<< info.cipher;
		return true;
	}
};

class RouterPermissionsVerifierI: public Glacier2::PermissionsVerifier
{
public:	
	virtual bool checkPermissions(const string& userId, const string& password, string& reason, const Ice::Current&)const
	{
		LOG(INFO) << "checkPermissions >> userId:" <<userId<<", passwd:"<< password;
		return true;
	}
};
class RouterSessionI : public Glacier2::Session
{
public:
    RouterSessionI(bool shutdown, bool ssl) : _shutdown(shutdown), _ssl(ssl)    {    }
    virtual void    destroy(const Ice::Current& current)    {
        //testContext(_ssl, current.ctx);
        current.adapter->remove(current.id);
        if(_shutdown)        {
            current.adapter->getCommunicator()->shutdown();
        }
	}
	
	virtual int shutdown(const Ice::Current& current)
	{
		current.adapter->getCommunicator()->shutdown();
		return 0;
	}

    virtual void    ice_ping(const Ice::Current& current) const
    {
		LOG(INFO) << "ice_ping" << _ssl;
    }

private:
    const bool _shutdown;
    const bool _ssl;
};

class RouterSessionManagerI : public Glacier2::SessionManager
{
public:
    virtual Glacier2::SessionPrx  create(const string& userId, const Glacier2::SessionControlPrx& ctrl, const Ice::Current& current)
    {
		LOG(INFO) << "Glacier2::SessionManager::create >> userId:" << userId ;       //testContext(userId == "ssl", current.ctx);
		//string category = "_" + userId;
		//ctrl->categories()->add(category);
        Glacier2::SessionPtr session = new RouterSessionI(false, userId == "test");
        return Glacier2::SessionPrx::uncheckedCast(current.adapter->addWithUUID(session));
    }
};
class SSLRouterSessionManagerI : public Glacier2::SSLSessionManager
{
public:
	virtual ::Glacier2::SessionPrx create(const ::Glacier2::SSLInfo& info, const ::Glacier2::SessionControlPrx& sc, const ::Ice::Current& current){
		LOG(INFO) << "Glacier2::SSLSessionManager::create >> userId:" << info.cipher ;       //testContext(userId == "ssl", current.ctx);
		//string category = "_" + userId;
		//ctrl->categories()->add(category);
        Glacier2::SessionPtr session = new RouterSessionI(false, true);
        return Glacier2::SessionPrx::uncheckedCast(current.adapter->addWithUUID(session));
	}
};

class CallbackI : public ::Test::Callback
{
public:
	virtual void initiateCallbackEx_async(const AMD_Callback_initiateCallbackExPtr& proxy, const CallbackReceiverPrx&, const Ice::Current& current){
		//proxy->callbackEx(current.ctx);
	}

    virtual void
    initiateCallbackWithPayload_async(const ::Test::AMD_Callback_initiateCallbackWithPayloadPtr&, const ::Test::CallbackReceiverPrx& proxy, const ::Ice::Current& current)
    {
		Ice::ByteSeq seq(1000 * 1024, 0);
        proxy->callbackWithPayload(seq,current.ctx);
    }
};


void SessionServerIceBox::start(const string& name, const Ice::CommunicatorPtr& communicator, const Ice::StringSeq& args)
{
	//char argv[1][10];
	//int argc;
	//Ice::stringSeqToArgs(args, argc, (char **)argv);
	Ice::PropertiesPtr prop = communicator->getProperties()->clone();//Ice::createProperties();
	string configFile = prop->getPropertyWithDefault("seq.configFile","sequence.properties");
	try
	{
		prop->load(configFile);
	}
	catch(const std::exception& ex)
	{
		cout << ex.what() <<endl;
		return;
	}
	catch(...)
	{
		cout << "unknown exception" <<endl;
		return;
	}
	string logdir = prop->getPropertyWithDefault("glog.logDestination","./logs/");
	int maxLogSize = prop->getPropertyAsIntWithDefault("glog.maxLogSize",1000);
	int logbufsecs = prop->getPropertyAsIntWithDefault("glog.logbufsecs",30);


	google::InitGoogleLogging("sessionManager");
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
	LOG(INFO) << "start createService: " << name;
	

	_communicator=communicator;
	/*
	string strUrl = prop->getPropertyWithDefault("seq.database.driver.url","mysql://127.0.0.1:3306/test?user=test&password=a");
	int poolSize = prop->getPropertyAsInt("seq.database.driver.poolSize");
	LOG(INFO) << "driverUrl = " << strUrl << ", poolSize=" << poolSize;
	MydbUtil_initial(strUrl,poolSize);
	MydbUtil_start();
	*/

    _adapter = communicator->createObjectAdapter("Glacier2.Server");
	_adapter->add(new RouterPermissionsVerifierI, communicator->stringToIdentity("RouterPermissionsVerifier"));
	_adapter->add(new RouterSessionManagerI,communicator->stringToIdentity("RouterSessionManager"));
	_adapter->activate();
	LOG(INFO) << "The Adapter:" << _adapter->getName() << " be activated.";
}

void
SessionServerIceBox::stop()
{
	LOG(INFO) << "The Adapter: " << _adapter->getName() << " be destroy." ;
	_adapter->destroy();
	//LOG(INFO) << "The Adapter: " << _adapter2->getName() << " be destroy." ;
    //_adapter2->destroy();

	//MydbUtil_stop();
	google::ShutdownGoogleLogging();
}


// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//   slice2freeze --dict StringLongMap,string,long StringLongMap
// slice2freeze --dict StringSeqRangeMap,string,tddl::sequences::SeqRange_t StringSeqRangeMap StringSeqRangeMap.ice
// **********************************************************************

#include <Ice/Ice.h>

#include <stdio.h>
#include <string>  
#include <sstream>  
#include <iostream>
#include <curl/curl.h>
#include <IceBox/IceBox.h>
#include <Glacier2/PermissionsVerifier.h>
#include <Glacier2/Session.h>
#include <Ice/Locator.h>
#include <glog/logging.h>
#include "SessionServer.h"
#include "Toolkits.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

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

class ServerLocatorRegistry : public virtual Ice::LocatorRegistry
{
public:
    virtual void
    setAdapterDirectProxy_async(const Ice::AMD_LocatorRegistry_setAdapterDirectProxyPtr& cb, const string&,
                                const Ice::ObjectPrx&, const Ice::Current&)
    {
        cb->ice_response();
    }

    virtual void
    setReplicatedAdapterDirectProxy_async(const Ice::AMD_LocatorRegistry_setReplicatedAdapterDirectProxyPtr& cb,
                                          const string&, const string&, const Ice::ObjectPrx&, const Ice::Current&)
    {
        cb->ice_response();
    }

    virtual void
    setServerProcessProxy_async(const Ice::AMD_LocatorRegistry_setServerProcessProxyPtr& cb,
                                const string&, const Ice::ProcessPrx&, const Ice::Current&)
    {
        cb->ice_response();
    }
};
class ServerLocatorI : public virtual Ice::Locator
{
public:
    ServerLocatorI(Ice::ObjectAdapterPtr adapter) :
        _adapter(adapter)
    {
        _registryPrx = Ice::LocatorRegistryPrx::uncheckedCast(adapter->add(new ServerLocatorRegistry,
                                                        Ice::stringToIdentity("registry")));
    }

    virtual void
    findObjectById_async(const Ice::AMD_Locator_findObjectByIdPtr& cb, const Ice::Identity& id, const Ice::Current&) const
    {
        cb->ice_response(_adapter->createProxy(id));
    }

    virtual void
    findAdapterById_async(const Ice::AMD_Locator_findAdapterByIdPtr& cb, const string& str, const Ice::Current&) const
    {
       cb->ice_response(_adapter->createDirectProxy(Ice::stringToIdentity(str)));
    }

    virtual Ice::LocatorRegistryPrx
    getRegistry(const Ice::Current&) const
    {
        return _registryPrx;
    }

private:
    Ice::ObjectAdapterPtr _adapter;
    Ice::LocatorRegistryPrx _registryPrx;
};
class ServantLocatorI : public virtual Ice::ServantLocator
{
public:

    ServantLocatorI()
    {
    }

    virtual Ice::ObjectPtr locate(const Ice::Current&, Ice::LocalObjectPtr& _backend)
    {
        return NULL;
    }

    virtual void finished(const Ice::Current&, const Ice::ObjectPtr&, const Ice::LocalObjectPtr&)
    {
    }

    virtual void deactivate(const string&)
    {
    }

private:

};


string auth2_check_url;
static size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream)  
{  
    string data((const char*) ptr, (size_t) size * nmemb);
	*((std::stringstream*) stream) << data << endl;
    return size * nmemb;  
} 

class RouterPermissionsVerifierI: public Glacier2::PermissionsVerifier
{
public:	
	virtual bool checkPermissions(const string& userId, const string& password, string& reason, const Ice::Current&)const
	{
		LOG(INFO) << "checkPermissions >> userId:" <<userId<<", passwd:"<< password;

		bool pass=false;
		struct curl_slist* headers = NULL;
		CURLcode res;
		std::stringstream out;
		string src="1";
		string getUrlStr = auth2_check_url;
		std::size_t found = getUrlStr.find("?");
  		if (found==std::string::npos)
		  getUrlStr.append("?");
		getUrlStr.append("&access_token=").append(password);
		getUrlStr.append("&src=").append(src);

		headers = curl_slist_append(headers, "User-Agent: Ice-Glacier2"); 

		CURL *curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_URL, getUrlStr.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		/* this example doesn't verify the server's certificate, which means we 
			might be downloading stuff from an impostor  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);  */  
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);    /*timeout 30s,add by edgeyang*/
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);    /*no signal,add by edgeyang*/
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);  
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);  
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);  
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  
        //curl_easy_setopt(curl, CURLOPT_HEADER, 1); 
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); 
		res = curl_easy_perform(curl); /* ignores error */
		if (res != CURLE_OK) {  
			//curl_easy_strerror进行出错打印  
			LOG(WARNING) << userId << "/" << password << ": curl_easy_perform() failed: " << curl_easy_strerror(res);  
		}else{
			string str_json = out.str();
			Document d;
    		d.Parse(str_json.c_str());
			if(d.HasMember("access_token")){
				Value& v = d["uid"];
				v.GetInt64();//==userId?
				pass=true;
			}
			if(!pass)
				LOG(WARNING) << password << ": " << str_json;
		}

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return pass;
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
		Ice::StringSeq categories = ctrl->categories()->get();
		for(std::vector<string>::iterator p = categories.begin(); p != categories.end(); ++p)
		{
			LOG(INFO) << "categories: " << *p;
		}

        Glacier2::SessionPtr session = new RouterSessionI(false, userId == "test");
        return Glacier2::SessionPrx::uncheckedCast(current.adapter->addWithUUID(session));
    }
};
/*
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
*/
static void SignalHandle(const char* data, int size)
{
    std::string str = std::string(data,size);
    LOG(ERROR)<<str;
}
void initGlog(Ice::PropertiesPtr prop){
	string logdir = prop->getPropertyWithDefault("glog.logDestination","./logs/");
	int maxLogSize = prop->getPropertyAsIntWithDefault("glog.maxLogSize",1000);
	int logbufsecs = prop->getPropertyAsIntWithDefault("glog.logbufsecs",30);

	google::InitGoogleLogging(prop->getPropertyWithDefault("appName","none").c_str());
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

void SessionServerIceBox::start(const string& name, const Ice::CommunicatorPtr& communicator, const Ice::StringSeq& args)
{
	//char argv[1][10];
	//int argc;
	//Ice::stringSeqToArgs(args, argc, (char **)argv);
	Ice::PropertiesPtr prop = communicator->getProperties()->clone();//Ice::createProperties();
	string configFile = prop->getPropertyWithDefault("seq.configFile","conf/sequence.properties");
	prop->load(configFile);
	initGlog(prop);

	auth2_check_url = prop->getPropertyWithDefault("seq.auth2.checkAddr","");

	LOG(INFO) << "start createService: " << name;
	
	if(!startTookits((void*)0))return;

	curl_global_init(CURL_GLOBAL_ALL);

	_communicator=communicator;
	/*
	string strUrl = prop->getPropertyWithDefault("seq.database.driver.url","mysql://127.0.0.1:3306/test?user=test&password=a");
	int poolSize = prop->getPropertyAsInt("seq.database.driver.poolSize");
	LOG(INFO) << "driverUrl = " << strUrl << ", poolSize=" << poolSize;
	MydbUtil_initial(strUrl,poolSize);
	MydbUtil_start();
	*/

    _adapter = communicator->createObjectAdapter("Glacier2.Server");
	_adapter->add(new RouterPermissionsVerifierI, Ice::stringToIdentity("RouterPermissionsVerifier"));
	_adapter->add(new RouterSessionManagerI,Ice::stringToIdentity("RouterSessionManager"));
	_adapter->activate();

    /*
    Ice::ObjectAdapterPtr backendAdapter = communicator->createObjectAdapter("BackendAdapter");
    backendAdapter->addServantLocator(new ServantLocatorI(), "");
    backendAdapter->activate();

    Ice::LocatorPtr locator = new ServerLocatorI(backendAdapter);
    backendAdapter->add(locator, Ice::stringToIdentity("locator"));
	*/
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
	curl_global_cleanup();
}

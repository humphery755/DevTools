// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <glog/logging.h>
#include <tddl_sequence_SequenceServiceI.h>

using namespace std;

static void SignalHandle(const char* data, int size)
{
    std::string str = std::string(data,size);
    LOG(ERROR)<<str;
}
static void initGlog(Ice::PropertiesPtr& prop){
	string logdir = prop->getPropertyWithDefault("glog.logDestination","./logs/");
	int maxLogSize = prop->getPropertyAsIntWithDefault("glog.maxLogSize",1000);
	int logbufsecs = prop->getPropertyAsIntWithDefault("glog.logbufsecs",30);

	google::InitGoogleLogging("testServer");
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

class Server : public Ice::Application
{
public:

    virtual int run(int argc, char* argv[]);
};

int
main(int argc, char* argv[])
{
    Server app;
    int status = app.main(argc, argv);
    return status;
}

int
Server::run(int argc, char*[])
{
    if(argc > 1)
    {
        cerr << appName() << ": too many arguments" << endl;
        return EXIT_FAILURE;
    }

    Ice::PropertiesPtr properties = communicator()->getProperties();
    Ice::PropertiesPtr prop = properties->clone();
    string configFile = prop->getPropertyWithDefault("seq.configFile","conf/sequence.properties");
	try
	{
		prop->load(configFile);
	}catch(const Ice::Exception& ex) {  Ice::Error out(Ice::getProcessLogger());  out <<__FILE__<<":"<< __LINE__<<" Ice::Exception:"<< ex;throw ex;}
	catch(...)
	{
		Ice::Error out(Ice::getProcessLogger());  out <<__FILE__<<":"<< __LINE__<<" unknown exception";
		return -1;
	}
    initGlog(prop);
    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("tddl.sequences.SequenceService");
	tddl::sequences::SequenceServicePtr seqService = new tddl_sequence_SequenceServiceI(0,0);
    //Demo::PricingEnginePtr pricing = new PricingI(properties->getPropertyAsList("Currencies"));
    adapter->add(seqService, communicator()->stringToIdentity("tddl.sequences.SequenceService"));
    adapter->activate();
    communicator()->waitForShutdown();
    return EXIT_SUCCESS;
}

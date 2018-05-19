// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************
#include <string>
#include <string.h>
#include <uuid/uuid.h>
#include <IceUtil/Shared.h>
#include <IceUtil/RecMutex.h>
#include <IceUtil/Monitor.h>
#include <Ice/ObjectAdapter.h>
#include <Ice/InstanceF.h>
#include <Ice/Ice.h>

#include <Ice/InstanceF.h>
#include <glog/logging.h>
#include <benchmark/benchmark.h>
#include "zookeeper_helper.h"
#include <SequenceServiceI.h>
#include "OrderSequenceAdapter.h"
#include "Toolkits.h"

using namespace std;
using namespace tddl::sequences;


#if defined(__GNUC__)
#define BENCHMARK_NOINLINE __attribute__((noinline))
#else
#define BENCHMARK_NOINLINE
#endif

static uint32_t err_total;
static Ice::CommunicatorPtr _communicator;
static Ice::PropertiesPtr _prop;

char version[50];

/**
 * 查看重复脚本
 * perl -n -e 'if ( /^\s*$/){print; next}; $v=substr($_,-188); if (exists($hash{$v})) { print } else { $hash{$v}=1; next }' logs/test_SnowflakeId.INFO
 */
static void BM_HelloService_echo(benchmark::State& state) {

  while (state.KeepRunning()) {    
    try
    {
      startOrderSequence(_communicator, _prop);
      sleep(100);
      stopOrderSequence();
    } 
    catch (const SequenceException& ex) {
      stringstream message;  
      __sync_fetch_and_add(&err_total,1);
      message<<"Failed to getAndIncrement: "<<ex.reason;
      state.SkipWithError(message.str().c_str());
    // ...
    } catch(...)
    {
      //cerr << ex << endl;
	  __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Unknow Error");
      //break; 
    }
	state.counters["Error"] = err_total;
  }
  
  state.SetBytesProcessed(int64_t(state.iterations()) *int64_t(state.range(0)));
}

#define BENCHMARK_XXX(t,arg) \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_HelloService_echo)->Arg(arg)->Threads(t);

int threads=1;

static void initGlog(Ice::PropertiesPtr& prop){
	string logdir = prop->getPropertyWithDefault("glog.logDestination","./logs/");
	int maxLogSize = prop->getPropertyAsIntWithDefault("glog.maxLogSize",1000);
	int logbufsecs = prop->getPropertyAsIntWithDefault("glog.logbufsecs",30);

	google::InitGoogleLogging("test");
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
}

class Tester : public Ice::Application
{
public:    
    virtual int run(int, char*[]);
};

class TestCommunicator : public Ice::CommunicatorPtr
{
public:
    Ice::ObjectAdapterPtr createObjectAdapter(char*){return NULL;}
};


int Tester::run(int argc, char** argv)
{
    
    Ice::PropertiesPtr properties = communicator()->getProperties();
    Ice::PropertiesPtr prop = properties->clone();
    string configFile = prop->getPropertyWithDefault("seq.configFile","conf/sequence.properties");
    try
    {
      prop->load(configFile);
    }catch(const Ice::Exception& ex) {  
      Ice::Error out(Ice::getProcessLogger());
      out <<__FILE__<<":"<< __LINE__<<" Ice::Exception:"<< ex;
      return -1;
    }
    catch(...)
    {
      Ice::Error out(Ice::getProcessLogger());  out <<__FILE__<<":"<< __LINE__<<" unknown exception";
      return -1;
    }
    string strVersion = prop->getPropertyWithDefault("app.ver","");
    strcpy(version,strVersion.c_str());

    initGlog(prop);
    _prop = prop;
    if(!startTookits((void*)0))return 1;
    err_total=0;

    TestCommunicator c;
    _communicator = c;
    startOrderSequence(_communicator, _prop);
      sleep(100);
      stopOrderSequence();

    if(1==1)return 0;

    BENCHMARK_XXX(threads<<1,0)
    BENCHMARK_XXX(threads<<2,0)
    BENCHMARK_XXX(threads<<3,0)
    BENCHMARK_XXX(threads<<4,0)
    BENCHMARK_XXX(threads<<5,0)
    BENCHMARK_XXX(threads<<6,0)
    BENCHMARK_XXX(threads<<7,0)
    BENCHMARK_XXX(threads<<8,0)

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();

    google::ShutdownGoogleLogging();
    
    return 0;
}

int main(int argc, char** argv) {

  if(argc>1){
    char *strTh=argv[1];
    threads=atoi(strTh);
  }

  Tester app;
  return app.main(argc, argv);

  
}


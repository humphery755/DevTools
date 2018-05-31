// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************
#include <string>
#include <Ice/Ice.h>
#include <IceBox/IceBox.h>
#include <Glacier2/PermissionsVerifier.h>
#include <Glacier2/Session.h>
#include <Glacier2/Router.h>
#include <glog/logging.h>
#include "IceExtClientUtil.h"
#include <benchmark/benchmark.h>
#include <string.h>
#include <uuid/uuid.h>

#include "tdd_sequence.h"

using namespace tddl::sequences;

using namespace std;
using namespace Ice;
using namespace IceExt;



#if defined(__GNUC__)
#define BENCHMARK_NOINLINE __attribute__((noinline))
#else
#define BENCHMARK_NOINLINE
#endif

IceClientUtil *clientUtil = new IceClientUtil("test","tests/config.client");
//IceClientUtil *glacier2Util = new IceClientUtil;
static uint32_t err_total;


static void BM_HelloService_echo(benchmark::State& state) {

  if (state.thread_index == 0) {
    // Setup code here.
   // cerr << "startup" <<endl;    
  }
  //char *src=new char[state.range(0)];
  //memset(src, 'x', state.range(0));

  ostringstream stream;
  stream << "test" << state.thread_index;
  string username=stream.str();
  string password="a";

  SequenceServicePrx sequence;
  string s1="tddl.sequences.SequenceService-V1";
  sequence = clientUtil->stringToProxy(sequence,s1);

  while (state.KeepRunning()) {
    try
    {
      SequenceRange seqRange;
      stringstream seqname_stream;  
      seqname_stream << "seq_test_1";
      string name=seqname_stream.str();
      seqRange=sequence->nextValue(name,1);
      LOG(INFO) << "max: "<<seqRange.max<<", min: "<<seqRange.min;
    } 
    catch (const SequenceException& ex) {
       __sync_fetch_and_add(&err_total,1);
      state.SkipWithError(ex.reason.c_str());
    } catch(...)
    {
       __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("unknow exception");
    }
    state.counters["Error"] = err_total;
  }
	
  
  state.SetBytesProcessed(int64_t(state.iterations()) *
                          int64_t(state.range(0)));
  //delete[] src;

  if (state.thread_index == 0) {
    // Teardown code here.
    //cerr << "Teardown" <<endl;
    
  }
}

//BENCHMARK(BM_MultiThreaded)->Threads(2);

//BENCHMARK(BM_DenseThreadRanges)->Arg(8)->DenseThreadRange(1, 2);
//BENCHMARK(BM_DenseThreadRanges)->Arg(8)->ThreadRange(1, 9)->Unit(benchmark::kMillisecond);
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

void initGlog(Ice::PropertiesPtr prop){
	string logdir = prop->getPropertyWithDefault("glog.logDestination","./logs/");
	int maxLogSize = prop->getPropertyAsIntWithDefault("glog.maxLogSize",1000);
	int logbufsecs = prop->getPropertyAsIntWithDefault("glog.logbufsecs",30);

	google::InitGoogleLogging(clientUtil->getAppName().c_str());
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

int main(int argc, char** argv) {
  if(argc>1){
    char *strTh=argv[1];
    threads=atoi(strTh);
  }

  initGlog(clientUtil->getProperties());
  if(clientUtil->initialize()!=0)return 1;  

  err_total=0;
  
  BENCHMARK_XXX(threads,64)
  BENCHMARK_XXX(threads,512)
  BENCHMARK_XXX(threads,1<<10)
  BENCHMARK_XXX(threads,2<<10)
  BENCHMARK_XXX(threads,4<<10)
  BENCHMARK_XXX(threads,8<<10)
  BENCHMARK_XXX(threads,16<<10)
  BENCHMARK_XXX(threads,32<<10)

  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  
  clientUtil->destroy();
}

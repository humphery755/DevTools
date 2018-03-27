// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************
#include <string>
#include <string.h>
#include <uuid/uuid.h>
#include <Ice/Ice.h>
#include <glog/logging.h>
#include <benchmark/benchmark.h>
#include <tdd_sequence.h>
#include <SequenceServiceI.h>

using namespace std;
using namespace Ice;
using namespace IceExt;
using namespace tddl::sequences;


#if defined(__GNUC__)
#define BENCHMARK_NOINLINE __attribute__((noinline))
#else
#define BENCHMARK_NOINLINE
#endif

static uint32_t err_total;
static SequenceWorker *currentRange;

/*
static int64_t lastTimestamp;
static pthread_mutex_t 	lock = PTHREAD_MUTEX_INITIALIZER;
inline static int64_t timeGen(){
	struct timeval tv;
    gettimeofday(&tv,NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

static void BM_HelloService_echo(benchmark::State& state) {
  int64_t timestamp;
  while (state.KeepRunning()) {
    Lock(&lock);
		timestamp = timeGen();
    if (timestamp < lastTimestamp) {
			stringstream message;
			message <<"Clock moved backwards.  Refusing to generate id for " << lastTimestamp - timestamp <<" milliseconds";
			state.SkipWithError(message.str().c_str());
		}
    state.counters["Error"] = err_total;
  }
  state.SetBytesProcessed(int64_t(state.iterations()) *int64_t(state.range(0)));
}
*/

/**
 * 查看重复脚本
 * perl -n -e 'if ( /^\s*$/){print; next}; $v=substr($_,-188); if (exists($hash{$v})) { print } else { $hash{$v}=1; next }' logs/test_SnowflakeId.INFO
 */
static void BM_HelloService_echo(benchmark::State& state) {

	tddl::sequences::SequenceRange retSeqRange;
  while (state.KeepRunning()) {    
    try
    {
      currentRange->getAndIncrement(1,retSeqRange);
      LOG(INFO) << "min="<<retSeqRange.min<<", max="<<retSeqRange.max;
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

int main(int argc, char** argv) {
  google::InitGoogleLogging("test_SnowflakeId");
  string logdir = "./logs/";
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
  FLAGS_servitysinglelog = true;
  //FLAGS_logbufsecs = 0; //缓冲日志输出，默认为30秒，此处改为立即输出

  if(argc>1){
    char *strTh=argv[1];
    threads=atoi(strTh);
  }

  err_total=0;
  currentRange = new SnowflakeIdWorker("SEQ_TX",0,0);


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
}


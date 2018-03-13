// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************
#include <string>
#include <string.h>
#include <uuid/uuid.h>
#include <Ice/Ice.h>
#include <tdd_sequence.h>
#include <glog/logging.h>
#include "IceExt_IceClientUtil.h"
#include <benchmark/benchmark.h>

#include "dbutil.h"
#include <tddl_sequence_SequenceServiceI.h>

using namespace std;
using namespace Ice;
using namespace IceExt;
using namespace tddl::sequences;


#if defined(__GNUC__)
#define BENCHMARK_NOINLINE __attribute__((noinline))
#else
#define BENCHMARK_NOINLINE
#endif

SequenceDao *seqDao;

static uint32_t err_total;

static void BM_HelloService_echo(benchmark::State& state) {

  if (state.thread_index == 0) {
    // Setup code here.
   // cerr << "startup" <<endl;    
  }
  //char *src=new char[state.range(0)];
  //memset(src, 'x', state.range(0));
  SequenceRangeI *currentRange;
  currentRange = new SequenceRangeI;
  stringstream seqname_stream;  
	seqname_stream << "pay_mch_info_seq" << state.thread_index;
  while (state.KeepRunning()) {    
    currentRange->value=1;
    string name=seqname_stream.str();
    try
    {
      seqDao->nextRange(name,currentRange);
    } 
    catch (const SequenceException& ex) {
      __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to echo...");
    // ...
    } catch (const std::exception& ex) {
      __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to echo...");
    // ...
    } catch (const std::string& ex) {
      __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to echo...");
    // ...
    } catch(...)
    {
      //cerr << ex << endl;
	  __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to echo...");
      //break; 
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


int main(int argc, char** argv) {
  google::InitGoogleLogging("sequence");
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

  string strurl = "mysql://192.168.1.12:3306/uhome?user=uhome&password=uhome110";
  MydbUtil_initial(strurl,1);
	MydbUtil_start();
 
  seqDao = new SequenceDao;

  if(argc>1){
    char *strTh=argv[1];
    threads=atoi(strTh);
  }

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
  
  MydbUtil_stop();
	google::ShutdownGoogleLogging();
}

SequenceRangeI::SequenceRangeI(){
}

void SequenceRangeI::reset(){
	this->over=false;
}

int SequenceRangeI::getAndIncrement(int step,tddl::sequences::SequenceRange& sr){
	int64_t currentValue = __sync_add_and_fetch(&this->value,step);
	if (currentValue > this->max) {
		this->over = true;
		if(currentValue-step<this->max){
			sr.min=currentValue-step;
			sr.max=this->max;
			return 0;
		}
		return -1;
	}	
	sr.min=currentValue-step+1;
	sr.max=currentValue;
	return 0;
}

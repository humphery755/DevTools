// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************
#include <string>
#include <Ice/Ice.h>
#include <Glacier2/Router.h>
#include <glog/logging.h>
#include <tdd_sequence.h>
#include <benchmark/benchmark.h>
#include <string.h>
#include <uuid/uuid.h>
#include "IceExtClientUtil.h"
#include <tddl_sequence_SequenceServiceI.h>

using namespace std;
using namespace tddl::sequences;
using namespace Ice;
using namespace IceExt;



#if defined(__GNUC__)
#define BENCHMARK_NOINLINE __attribute__((noinline))
#else
#define BENCHMARK_NOINLINE
#endif

IceClientUtil *clientUtil = new IceClientUtil("clientTest","conf/config.client");


static uint32_t err_total;
#define ARRAY_LEN 1
/*
string array[ARRAY_LEN]={ "user_busi_rel_id_seq",
"user_area_rel_id_seq",
"template_overtime_conf_seq",
"template_node_inst_rel_id",
"template_node_id_seq",
"template_inst_id_seq",
"template_inst_condition_node_id",
"template_id_seq",
"template_dif_id_seq",
"template_busi_type_id_seq",
"template_attr_id_seq",
"tb_uhome_user_seq",
"tb_uhome_user_house_seq",
"tb_uhome_user_community_rel_seq",
"tb_uhome_user_attr_seq",
"tb_uhome_strong_token_seq",
"tb_uhome_address_seq",
"tache_id_seq",
"service_visual_range_id",
"service_authorization_id",
"SEQ_TRACK_ID",
"SEQ_TEST_RPC",
"SEQ_SUPERVISE_ID",
"SEQ_ROTATE_ID",
"SEQ_ORDER_USER_ID",
"SEQ_ORDER_ID",
"SEQ_HANG_ID",
"SEQ_DEAL_USER_ID",
"SEQ_COMMENT_ID",
"SEQ_ATTR_INST_ID",
"pay_record_vpayid_seq",
"pay_record_seq",
"pay_mch_info_seq",
"pay_mch_bz_rel_seq",
"node_inst_id",
"node_attr_value_id_seq",
"node_attr_id_seq",
"management_user_rel_id_seq",
"management_id_seq",
"c_user_account_seq",
"authorization_busi_rel_id_seq",
"attr_value_id_seq",
"attr_inst_id"};
*/
string array[ARRAY_LEN]={ "TEST_1"};


static void BM_SequenceService_nextValue(benchmark::State& state) {

  if (state.thread_index == 0) {
    // Setup code here.
   // cerr << "startup" <<endl;    
  }
  string verkey="app.ver";
  string strVersion = clientUtil->getProperty(verkey,EMPTY_STRING);
  string s1="tddl.sequences.SequenceService";
  s1.append(strVersion);
  SequenceServicePrx sequence;
  sequence = clientUtil->stringToProxy(sequence,s1);

  if(!sequence)
  {
      cerr <<  ": invalid or missing stringToProxy:" << s1<< endl;
      return ;
  }

  //char *src=new char[state.range(0)];
  //memset(src, 'x', state.range(0));
  SequenceRange seqRange;
  Ice::Context ctx;
  ctx["tId"] = "xxxxx";
  ctx["rId"] = "1";

  string seq_name=array[state.thread_index%ARRAY_LEN];
  while (state.KeepRunning()) {
    try
    {
        seqRange=sequence->nextValue(seq_name,1,ctx);//"SEQ_TEST_RPC", 1);
    }catch(const SequenceException& ex)
    {
	  __sync_fetch_and_add(&err_total,1);
      state.SkipWithError(ex.reason.c_str());
    }
    catch(const Ice::Exception& ex)
    {
      cerr << ex << endl;
	  __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to echo...");
      //break; 
    }catch(...)
    {
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

static void BM_OrderSequenceService_nextValue(benchmark::State& state) {
    SequenceServicePrx orderSequence;
    string verkey="app.ver";
    string strVersion = clientUtil->getProperty(verkey,EMPTY_STRING);
    string s="tddl.sequences.OrderSequenceService";
    s.append(strVersion);
    orderSequence = clientUtil->stringToProxy(orderSequence,s);
  if(!orderSequence)
  {
        cerr <<  ": invalid or missing stringToProxy:" << s<< endl;
        return ;
    }

    //char *src=new char[state.range(0)];
    //memset(src, 'x', state.range(0));
    SequenceRange seqRange;
    Ice::Context ctx;
    ctx["tId"] = "xxxxx";
    ctx["rId"] = "1";
  
    string seq_name=array[state.thread_index%ARRAY_LEN];
    while (state.KeepRunning()) {
      try
      {
          seqRange=orderSequence->nextValue(seq_name,1,ctx);
      }catch(const SequenceException& ex)
      {
      __sync_fetch_and_add(&err_total,1);
        state.SkipWithError(ex.reason.c_str());
      }
      catch(const Ice::Exception& ex)
      {
        cerr << ex << endl;
      __sync_fetch_and_add(&err_total,1);
        state.SkipWithError("Failed to echo...");
        //break; 
      }catch(...)
      {
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
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SequenceService_nextValue)->Arg(arg)->Threads(t);

#define BENCHMARK_YYY(t,arg) \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_OrderSequenceService_nextValue)->Arg(arg)->Threads(t);
int threads=1;

SequenceServicePrx orderSequence;
int test(){
  
  SequenceRange seqRange;
  Ice::Context ctx;
  ctx["tId"] = "xxxxx";
  ctx["rId"] = "1";

  string seq_name="TEST_1";//SEQ_TEST_1
  try{
    seqRange=orderSequence->nextValue(seq_name,1,ctx);
    cout << "max: "<<seqRange.max<<", min: "<<seqRange.min<<endl;
  }catch(const SequenceException& ex)
  {
    cerr <<ex.reason.c_str()<<endl;
  } catch(const Ice::Exception& ex)
  {
      cerr << ex << endl;
  } catch(...){
      cerr << " Unknow Exception" << endl;
  }
  return 0;
}

void initGlog(){
  Ice::PropertiesPtr prop = clientUtil->getProperties();
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
  
  initGlog();
  if(
  clientUtil->initialize()!=0)return 1;
  //Ice::RouterPrx defaultRouter = clientUtil->communicator()->getDefaultRouter();
  //Glacier2::RouterPrx router = Glacier2::RouterPrx::checkedCast(defaultRouter);
  //clientUtil->communicator()->setDefaultRouter(router);
  /*


*/
  if(argc>1){
    char *strTh=argv[1];
    threads=atoi(strTh);
  }
if(1==1){
  string verkey="app.ver";
  string strVersion = clientUtil->getProperty(verkey,EMPTY_STRING);
  string s="tddl.sequences.SequenceService";
  s.append(strVersion);
  orderSequence = clientUtil->stringToProxy(orderSequence,s);
  if(!orderSequence)
  {
      cerr <<  ": invalid or missing stringToProxy:" << s<< endl;
      return 0;
  }

  orderSequence = orderSequence->ice_locatorCacheTimeout(0);
  orderSequence = orderSequence->ice_connectionCached(false);

  int i=0;
  for(;i<threads;i++) {
test();
//sleep(1);
  } 
  
  return 1;
}


  err_total=0;
  

  //BENCHMARK_YYY(64,64)
  BENCHMARK_XXX(64,64)
  BENCHMARK_XXX(512,512)
  //BENCHMARK_YYY(512,512)
  BENCHMARK_XXX(1<<10,1<<10)
  //BENCHMARK_YYY(1<<10,1<<10)
  BENCHMARK_XXX(2<<10,2<<10)
  //BENCHMARK_YYY(2<<10,2<<10)
  BENCHMARK_XXX(4<<10,4<<10)
  //BENCHMARK_YYY(4<<10,4<<10)
  BENCHMARK_XXX(8<<10,8<<10)
  //BENCHMARK_YYY(8<<10,8<<10)
  BENCHMARK_XXX(16<<10,16<<10)
  //BENCHMARK_YYY(16<<10,16<<10)
  BENCHMARK_XXX(32<<10,32<<10)
  if(1==0)
  BENCHMARK_YYY(32<<10,32<<10)

  ::benchmark::Initialize(&argc, argv);
  //::benchmark::RunSpecifiedBenchmarks();
  while(1)::benchmark::RunSpecifiedBenchmarks();
  clientUtil->destroy();
}
/*
SequenceRangeI::SequenceRangeI(){
}

int SequenceRangeI::getAndIncrement(int step,tddl::sequences::SequenceRange& sr){
	int64_t currentValue = __sync_add_and_fetch(&this->value,step);
	if (currentValue > this->max) {
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
*/
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
#include "dbutil.h"

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


class CallbackClient : public Application
{
public:

    virtual int run(int, char*[]);
};
int
CallbackClient::run(int, char**)
{
    communicator()->setDefaultRouter(Glacier2::RouterPrx());
    Glacier2::RouterPrx router = Glacier2::RouterPrx::uncheckedCast(
        communicator()->stringToProxy("Glacier2/router:tcp -h 127.0.0.1 -p 12350"));
    communicator()->setDefaultRouter(router);

    //
    // Next try to create a non ssl session. This should succeed.
    //
    cout << "creating non-ssl session with ssl connection... ";
    try
    {
        Glacier2::SessionPrx session = router->createSession("ssl", "");
        session->ice_ping();
        router->destroySession();
    }
    catch(const Glacier2::PermissionDeniedException& ex)
    {
      Error out(getProcessLogger());
                out << ex;
    }
    cout << "ok" << endl;


    communicator()->setDefaultRouter(0);
    //Ice::ProcessPrx process = Ice::ProcessPrx::checkedCast(communicator()->stringToProxy("Glacier2/admin -f Process:tcp -h 192.168.1.10 -p 5065"));
    //process->shutdown();

    return EXIT_SUCCESS;
}

int
test(int argc, char* argv[])
{
#ifdef ICE_STATIC_LIBS
    Ice::registerIceSSL();
#endif

    
    //
    // We must disable connection warnings, because we attempt to ping
    // the router before session establishment, as well as after
    // session destruction. Both will cause a ConnectionLostException.
    //
    Ice::InitializationData initData;
    initData.properties = Ice::createProperties(argc, argv);
    initData.properties->setProperty("Ice.Warn.Connections", "0");
    //initData.properties->setProperty("IceSSL.CertAuthFile", "/home/yuhui/glacier2test/conf/example.com_cacert.crt");
    initData.properties->setProperty("IceSSL.VerifyPeer", "2");
    initData.properties->setProperty("Ice.Plugin.IceSSL", "IceSSL:createIceSSL");
    initData.properties->setProperty("Glacier2.Client.Trace.Request", "2");
    

    CallbackClient app;
    return app.main(argc, argv, initData);
}

static void test1(int argc, char* argv[]){
  ostringstream stream;
  string username=stream.str();
  string password="a";
{
  argc=argc;
  argv=argv;
}
  SequenceServicePrx sequence;
  string s1="tddl.sequences.SequenceService-V1";
  sequence = clientUtil->stringToProxy(sequence,s1);

  SequenceRange seqRange;
  stringstream seqname_stream;  
	seqname_stream << "pay_mch_info_seq";
    string name=seqname_stream.str();
    try
    {
      seqRange=sequence->nextValue(name,1);
      cout << "min:"<<seqRange.min <<", max:"<<seqRange.max<<endl;
    } 
    catch (const SequenceException& ex) {
      cout << "exception:" << ex.reason << endl;
    } catch(...)
    {
      cout << "unknow exception:" << endl;
    }
}

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
      seqname_stream << "pay_mch_info_seq";
      string name=seqname_stream.str();
      seqRange=sequence->nextValue(name,1);
    } 
    catch (const SequenceException& ex) {
      cout << "exception:" << ex.reason << endl;
       __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to createSession...");
    } catch(...)
    {
      cout << "unknow exception:" << endl;
       __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to createSession...");
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
  google::InitGoogleLogging("clientTest");
  string logdir = "./logs/test/";
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

  clientUtil->initialize();
  //clientUtil->initialize(0, NULL, "tests/config.1.client");
  if(1==0){
    test1(argc, argv);
    if(1==1)return 0;
      test(argc, argv);
  }

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
  
  clientUtil->destroy();
}

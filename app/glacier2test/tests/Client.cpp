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
#include <string.h>
#include <uuid/uuid.h>

#include "tdd_sequence.h"

using namespace tddl::sequences;

using namespace std;
using namespace Ice;
using namespace IceExt;



IceClientUtil *clientUtil = new IceClientUtil("test","tests/config.client");


class CallbackClient : public Application
{
public:

    virtual int run(int, char*[]);
};
int
CallbackClient::run(int, char**)
{
  string url = "RouterPermissionsVerifier:default -h 127.0.0.1 -p 12011";
  Glacier2::PermissionsVerifierPrx permission = Glacier2::PermissionsVerifierPrx::uncheckedCast(communicator()->stringToProxy(url));
  string userId="userId", password="password", reason="reason";
  const Ice::Context ctx;
  if(!permission->checkPermissions(userId,password,reason,ctx))return 1;

    communicator()->setDefaultRouter(Glacier2::RouterPrx());
    url = "RouterSessionManager:default -h 127.0.0.1 -p 12011";
    
    Glacier2::RouterPrx router = Glacier2::RouterPrx::uncheckedCast(
        communicator()->stringToProxy(url));
    communicator()->setDefaultRouter(router);

    Glacier2::SessionManagerPrx sessionMgr = Glacier2::SessionManagerPrx::uncheckedCast(communicator()->stringToProxy(url));
    //
    // Next try to create a non ssl session. This should succeed.
    //
    cout << "creating non-ssl session with ssl connection... ";
    Glacier2::SessionControlPrx ctrl;

    try
    {
        Glacier2::SessionPrx session = router->createSession("user", "xxxx");
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
	seqname_stream << "seq_test_1";
    string name=seqname_stream.str();
    try
    {
      seqRange=sequence->nextValue(name,1);
      LOG(INFO) << "max: "<<seqRange.max<<", min: "<<seqRange.min;
    } 
    catch (const SequenceException& ex) {
      LOG(ERROR) << ex.reason.c_str();
    } catch(...)
    {
      LOG(ERROR) <<"unknow exception:";
    }
}



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

  if(1==0){
    test1(argc, argv);
  }

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
  
  //clientUtil->destroy();
}

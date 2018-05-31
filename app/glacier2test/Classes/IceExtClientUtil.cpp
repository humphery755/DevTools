#include <regex.h>
#include <string>
#include <Ice/Ice.h>
#include <Glacier2/Router.h>
#include <Glacier2/Application.h>
#include <IceUtil/Exception.h>
#include "Toolkits.h"
#include "IceExtClientUtil.h"

using namespace std;
using namespace Ice;
using namespace IceExt;
/*
#define ERROR 0
#define INFO 1
#define WARN 2
#define LOG(L) if(L==ERROR){ Error out(getProcessLogger()) }else if(L==WARN){ Warning out(getProcessLogger()) }else if(L==INFO){ Print out(getProcessLogger()) }
*/
static const char* ice_router_reg ="Ice\\.\\w+\\.Router=";
static const char* ice_router_pwd_reg ="(\\|\\w+)/(\\w+)";
static const char* ice_router_val_reg ="(Ice\\.\\w+\\.Router)/([\\.0-9A-Za-z_\\-]+)";
static const char* ice_locator_reg="Ice\\.\\w+\\.Locator=";

class LocatorCommunicator : public CommunicatorProxyI {
public:
    LocatorCommunicator(Ice::PropertiesPtr &prop,string& locator):
        CommunicatorProxyI()
    {
        InitializationData initData;
        StringSeq args;
        initData.properties = createProperties(args,prop);
        initData.properties->setProperty("Ice.Default.Router",""); 
        initData.properties->setProperty("Ice.Default.Locator",locator);
        char *_argv[] = {(char*)"0"};
        int argc = 0;
        _communicator = Ice::initialize(argc, _argv, initData);
    }
    ~LocatorCommunicator(){
        if(_communicator)
        {
            try
            {
                _communicator->destroy();
            }
            catch(const std::exception& ex)
            {
                Error out(getProcessLogger());
                out << ex;
            }
            catch(...)
            {
                Error out(getProcessLogger());
                out << "unknown exception";
            }
            _communicator=0;
        }
    }
    Ice::ObjectPrx stringToProxy(string& id)
    {
        return _communicator->stringToProxy(id);
    }
private:
	Ice::CommunicatorPtr     _communicator;
    std::string              user;
    std::string              passwd;
    volatile bool            runing;
};

class Glacier2Communicator : public IceExt::CommunicatorProxyI, public IceUtil::Thread, public IceUtil::Monitor<IceUtil::Mutex>{
public:
    Glacier2Communicator(Ice::PropertiesPtr &prop,string& s,string svcId,bool ssl);
    ~Glacier2Communicator(){
        runing=false;
        _initialized = false;
        if(_communicator)
        {
            try
            {
                _router->destroySession();
                _communicator->destroy();
            }
            catch(const std::exception& ex)
            {
                Error out(getProcessLogger());
                out <<"["<<name()<<"]"<< ex;
            }
            catch(...)
            {
                Error out(getProcessLogger());
                out <<"["<<name()<<"]"<< "unknown exception";
            }
            _communicator=0;
        }
    }
    virtual Glacier2::SessionPrx createSession();
    virtual Ice::ObjectPrx stringToProxy(string& id);
    virtual void run()
    {
        runing=true;
        while(runing){
            try
            {
                Glacier2::SessionPrx session;
                {
                    Lock sync(*this);
                    _initialized = false; 
                    _communicator->setDefaultRouter(Glacier2::RouterPrx());
                    //Ice::RouterPrx defaultRouter = _communicator->getDefaultRouter();
                    _router = Glacier2::RouterPrx::uncheckedCast(
                    _communicator->stringToProxy(initData.properties->getPropertyWithDefault("Ice.Default.Router","")));
                    _communicator->setDefaultRouter(_router);                    
                    session=createSession();
                }

                Context context;
                context["_fwd"] = "t";
                
                while(runing)
                {
                    if(!_initialized){
                        Lock sync(*this);
                        session->ice_ping(context);
                        //getProcessLogger()->print("ice_ping success.");
                        _initialized = true;
                        notifyAll();
                    }else{
                        int64_t currentTime = getCurrentTime();
                        if(currentTime - last_time > 60){
                            Lock sync(*this);
                            session->ice_ping(context);
                            //getProcessLogger()->print("ice_ping success.");
                            notifyAll();
                        }
                    }                    
                    IceUtil::ThreadControl::sleep(IceUtil::Time::seconds(60));
                    continue;
                }
            }
            catch(const Ice::ConnectionLostException& e1) { getProcessLogger()->warning( "ice_ping failure: ConnectionLostException"); }
            catch(const Ice::ObjectNotExistException& e2) { getProcessLogger()->warning("ice_ping failure: ObjectNotExistException");  }
            catch(const Ice::CommunicatorDestroyedException& e3) { getProcessLogger()->warning("ice_ping failure: CommunicatorDestroyedException");}
            //catch(const Ice::SocketException& e4){  Error out(getProcessLogger());  out<<"SocketException:" << e4; }
            catch(const Ice::Exception& ex) {  Error out(getProcessLogger());  out <<__FILE__<<":"<< __LINE__<<"["<<name()<<"]"<< ex;}
            catch(const std::exception& e5)
            {
                Error out(getProcessLogger());
                out <<__FILE__<<":"<< __LINE__<<"["<<name()<<"]"<< e5;
            } catch(...){
                Error out(getProcessLogger());
                out <<__FILE__<<":"<< __LINE__<<"["<<name()<<"]"<< " Unknow Exception";
            }
            IceUtil::ThreadControl::sleep(IceUtil::Time::seconds(10));
        }        
    }
    //virtual int doMain(int, char**, const Ice::InitializationData&);

private:
	Ice::CommunicatorPtr     _communicator;
    Glacier2::RouterPrx      _router;
    Ice::InitializationData  initData;
    std::string              user;
    std::string              passwd;
    std::string              svcId;
    volatile bool            runing;
    volatile bool            _initialized;
    bool                     ssl;
    uint64_t                 last_time;
};

Glacier2Communicator::Glacier2Communicator(Ice::PropertiesPtr &prop,string& router,string _svcId,bool _ssl): CommunicatorProxyI(),IceUtil::Thread(router),ssl(_ssl)
{
    regex_t reg;
    regmatch_t pmatch[3];
    char *p,*pos,c;

    _initialized = false;
    regcomp(&reg, ice_router_pwd_reg, REG_EXTENDED);
    p=(char*)router.c_str();
    if(regexec(&reg, p, 3, pmatch, 0)!=0){
        regfree(&reg);
        EndpointParseException ex(__FILE__, __LINE__);
        ex.str = "no configure user/passwd: "+router;
        throw ex;
    }

    c = p[pmatch[1].rm_eo];//username
    p[pmatch[1].rm_eo]='\0';
    pos=p+pmatch[1].rm_so;
    for(pos++ ;*pos && *pos == ' ';pos++);
    user=pos;
    p[pmatch[1].rm_eo]=c;

    c = p[pmatch[2].rm_eo];//passwd
    p[pmatch[2].rm_eo]='\0';
    pos=p+pmatch[2].rm_so;
    passwd=pos;
    p[pmatch[2].rm_eo]=c;

    c = p[pmatch[1].rm_so];
    p[pmatch[1].rm_so]='\0';//DemoGlacier2/router:tcp -p 5064
    router=p;
    p[pmatch[1].rm_so]=c;
    regfree(&reg);

    initData.properties = prop->clone();
    initData.properties->setProperty("Ice.RetryIntervals", "-1");
    initData.properties->setProperty("Ice.Default.Locator","");
    initData.properties->setProperty("Ice.Default.Router",router);

    int argc=1;
    char *argv[] = {(char*)router.c_str()};

    Ice::StringSeq args = Ice::argsToStringSeq(argc, argv);
    initData.properties = createProperties(args,initData.properties);

    _communicator = Ice::initialize(argc, argv, initData);    
    svcId=_svcId;
}

Glacier2::SessionPrx Glacier2Communicator::createSession()
{
    Glacier2::SessionPrx session;
    while(runing)
    {
  
        try
        {
            //if(ssl)session = _router->createSessionFromSecureConnection();
            //else
            session = _router->createSession(user, passwd);
            break;
        }
        catch(const Glacier2::PermissionDeniedException& ex)
        {
            Error out(getProcessLogger());
            out <<__FILE__<<":"<< __LINE__<<"["<<name()<<"]"<< " permission denied: " << ex.reason;

        }
        catch(const Glacier2::CannotCreateSessionException& ex)
        {
            Error out(getProcessLogger());
            out <<__FILE__<<":"<< __LINE__<<"["<<name()<<"]"<< " cannot create session: " << ex.reason;
        }
    }
    return session;
}

Ice::ObjectPrx Glacier2Communicator::stringToProxy(string& id)
{
    last_time = getCurrentTime();
    Lock sync(*this);
    if(_initialized){
        return _communicator->stringToProxy(svcId.empty()?id:svcId);
    }

    timedWait(IceUtil::Time::seconds(30));

    if(_initialized){
        return _communicator->stringToProxy(svcId.empty()?id:svcId);
    }
    return NULL;
}

IceClientUtil::IceClientUtil(std::string appName,std::string configFile):_appName(appName){
    InitializationData initData;
    initData.properties = createProperties();      
    initData.properties->load(configFile);
    prop = initData.properties;
}

std::string IceClientUtil::getProperty(std::string& k,std::string& v){return prop->getPropertyWithDefault(k,v);}

int IceClientUtil::initialize()
{
    pthread_rwlock_init(&lock,NULL);

    regex_t reg;
    regmatch_t pmatch[3];
    string key;
    CommunicatorProxyI *communicator;
    Ice::PropertyDict dict = prop->getPropertiesForPrefix("");
    for(Ice::PropertyDict::const_iterator it = dict.begin(); it != dict.end(); ++it)
    {
        key = it->first;
        regcomp(&reg, ice_locator_reg, REG_EXTENDED);
        string key1=key;
        key1.append("=");
        if(regexec(&reg, key1.c_str(), 2, pmatch, 0)==0){
            regfree(&reg);
            string locator = prop->getPropertyWithDefault(key,EMPTY_STRING);
            communicator = new LocatorCommunicator(prop,locator);
            cmap.insert(make_pair(key, communicator));
            continue;
        }
        regfree(&reg);
        regcomp(&reg, ice_router_reg, REG_EXTENDED);
        if(regexec(&reg, key1.c_str(), 2, pmatch, 0)==0){                
            regfree(&reg);
            string router = prop->getPropertyWithDefault(key,EMPTY_STRING);
            string::size_type idx = router.find("/router:ssl ");
            bool isssl = idx != string::npos;
            communicator = new Glacier2Communicator(prop,router,EMPTY_STRING,isssl);
            ((Glacier2Communicator*)communicator)->start();
            cmap.insert(make_pair(key, communicator));
            continue;
        }
        regfree(&reg);

        string val = it->second;//prop->getPropertyWithDefault(key,"");
        regcomp(&reg, ice_router_val_reg, REG_EXTENDED);
        char *p,*pos,c;
        p=(char*)val.c_str();
        if(regexec(&reg, p, 3, pmatch, 0)==0){
            regfree(&reg);            
            c = p[pmatch[1].rm_eo];//routerKey
            p[pmatch[1].rm_eo]='\0';
            pos=p;
            //for(pos++ ;*pos && *pos == ' ';pos++);
            string rKey=pos;
            p[pmatch[1].rm_eo]=c;

            c = p[pmatch[2].rm_eo];//service name
            p[pmatch[2].rm_eo]='\0';
            pos=p+pmatch[2].rm_so;
            string svcId=pos;
            p[pmatch[2].rm_eo]=c;

            string router = prop->getPropertyWithDefault(rKey,EMPTY_STRING);
            if(router.empty()){
                Error out(getProcessLogger());
                out << "no configure: "<<rKey;
                return -1;
            }
            string::size_type idx = router.find("/router:ssl ");
            bool isssl = idx != string::npos;
            communicator = new Glacier2Communicator(prop,router,svcId,isssl);
            ((Glacier2Communicator*)communicator)->start();
            cmap.insert(make_pair(val, communicator));
            continue;
        }
        regfree(&reg);
    }

    std::map<std::string,CommunicatorProxyI*>::iterator it;
    it = cmap.find("Ice.Default.Locator");
    if (it != cmap.end()){
        _communicator = it->second;
        return 0;
    }

    it = cmap.find("Ice.Default.Router");
    if (it != cmap.end()){
        _communicator = it->second;
        return 0;
    }    

    Warning out(getProcessLogger());
    out << "no configure Ice.Default.Locator/Ice.Default.Router";
    return -1;
}

void IceClientUtil::destroy(){
    std::map<std::string,CommunicatorProxyI*>::iterator it;
    CommunicatorProxyI* c;
    it = cmap.begin();
    while(it != cmap.end())
    {
        c=it->second;
        try
        {
            delete c;
        }
        catch(const std::exception& ex)
        {
			Error out(getProcessLogger());
            out << ex;
        }
        catch(...)
        {
			Error out(getProcessLogger());
            out << "unknown exception";
        }
        it++;
    }
	if(_communicator)
    {
        delete _communicator;
    }
    pthread_rwlock_destroy(&lock);
}

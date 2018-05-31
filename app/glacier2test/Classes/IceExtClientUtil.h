#ifndef __IceExt__IceClientUtil_h__
#define __IceExt__IceClientUtil_h__


namespace IceExt
{

class CommunicatorProxyI
{
public:
	CommunicatorProxyI(){}
	virtual ~CommunicatorProxyI(){}
	virtual Ice::ObjectPrx stringToProxy(std::string& id)=0;
};

typedef IceUtil::Handle<CommunicatorProxyI> CommunicatorProxyPtr;
static std::string 							EMPTY_STRING="";

class IceClientUtil
{
public:
	virtual int initialize();
	virtual void destroy();
	template<class T> T stringToProxy(T,std::string& id);
	//Ice::CommunicatorPtr communicator(){return _communicator;}
	virtual std::string getProperty(std::string& k,std::string& v);
	virtual std::string getAppName(){return this->_appName;}
	virtual Ice::PropertiesPtr getProperties(){return prop;}
	IceClientUtil(std::string,std::string);

	
private:
	std::string 								_appName;
	static IceClientUtil* 						m_pInstance;
	Ice::PropertiesPtr 							prop;
	CommunicatorProxyI* 						_communicator;
	std::map<std::string,CommunicatorProxyI*> 	cmap;
	pthread_rwlock_t 							lock;
};

template<class T> T IceClientUtil::stringToProxy(T,std::string& id)
{
	std::string svcId=id;
    std::string s = prop->getPropertyWithDefault(id,EMPTY_STRING);
	CommunicatorProxyI *communicator=NULL;
    if(s.length()>5){
		std::map<std::string,CommunicatorProxyI*>::iterator it;
		it = cmap.find(s);
		if (it != cmap.end()){
			communicator = it->second;
		}else{
			communicator = _communicator;
			svcId=s;
		}
	}else{
		communicator = _communicator;
	}

	if (communicator==NULL){
		Ice::Warning out(Ice::getProcessLogger());
		out << "no configure: "<< s;
		return NULL;
	} 

	Ice::ObjectPrx obj = communicator->stringToProxy(svcId);

	if(!obj)
	{
		Ice::Error out(Ice::getProcessLogger());
		out <<  "invalid or missing stringToProxy:" << svcId;
		return NULL;
	}
	T result;
	result  = T::checkedCast(obj);
	if(!result)
	{
		Ice::Error out(Ice::getProcessLogger());
		out <<  "checkedCast is null by :" << svcId;
		return NULL;
	}
	return result;    
}

}
#endif


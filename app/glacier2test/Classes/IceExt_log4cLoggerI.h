
#ifndef __IceExt_Log4cLoggerI_h__
#define __IceExt_Log4cLoggerI_h__

#include <Ice/Logger.h>

namespace IceExt
{
	
class Log4cLoggerI : virtual public Ice::Logger {
public:
	Log4cLoggerI(const std::string&);
    virtual void print(const std::string&);
    virtual void trace(const std::string&, const std::string&);
    virtual void warning(const std::string&);
    virtual void error(const std::string&);
	virtual std::string getPrefix();
    virtual Ice::LoggerPtr cloneWithPrefix(const std::string&);
	
private:
	std::string prefix;
};

}
#endif

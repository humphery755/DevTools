#include<stdlib.h>
#include<string>
#include <Ice/Logger.h>
#include "IceExtLoggerI.h"
#include <glog/logging.h>

using namespace std;
using namespace Ice;
using namespace IceExt;

Log4cLoggerI::Log4cLoggerI(const string& prefix)
{
	this->prefix = prefix;
}
void Log4cLoggerI::print(const std::string& message){
	LOG(INFO) << prefix << message;
}
void Log4cLoggerI::trace(const std::string& category, const std::string& message){
	LOG(INFO) << prefix << category << message;
}
void Log4cLoggerI::warning(const std::string& message){
	LOG(WARNING) << prefix << message;
}
void Log4cLoggerI::error(const std::string& message){
	LOG(ERROR) << prefix << message;
}
std::string Log4cLoggerI::getPrefix(){
	return prefix;
}
LoggerPtr Log4cLoggerI::cloneWithPrefix(const string& prefix){
	return new Log4cLoggerI(prefix);
}


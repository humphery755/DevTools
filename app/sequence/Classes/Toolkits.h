
#ifndef Toolkits_H_
#define Toolkits_H_

#include <stdint.h>

extern "C" {
/****************************************************************************************
*@description		获取服务器时间缓存
*@frequency of call	Startup()启动后调用，线程安全
******************************************************************************************/
int64_t getCurrentTime();

/****************************************************************************************
*@description		服务器启动时调用，只能调用一次
******************************************************************************************/
bool startTookits(void * pParam);

/****************************************************************************************
*@description		服务器停止时时调用，只能调用一次
******************************************************************************************/
void stopTookits();
}

#endif

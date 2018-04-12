
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <sstream>  
#include <iostream>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include "Toolkits.h"

static bool                     runing=false;
static volatile int64_t         currentTime=0;	/** 时间缓存 **/

int64_t getCurrentTime(){return currentTime;}

/****************************************************************************************	
*@description		为避免每次都调用OS的gettimeofday，采用自行维护时间缓存(针对对于服务器的时间管理不需要精确需求)
*@frequency of call	单线程定时调用
******************************************************************************************/
static void* updateTime(void * pParam){
    while(runing)
	{
        struct timeval tv;
        gettimeofday(&tv,NULL);
        currentTime = tv.tv_sec;
        sleep(1);
    }
    return (void*)0;
}

bool startTookits(void * pParam){
    if(runing)return true;
    runing = true;
	//启动连接池维护线程
	pthread_t localThreadId;
	if ( pthread_create( &localThreadId, NULL, updateTime, pParam ) != 0 )
	{
		std::cout  << "pthread_create failed" <<std::endl;
		return false;
	}
	pthread_detach(localThreadId);
    return true;
}

void stopTookits(){
    runing = false;
}



#include <stdio.h>
#include <string>
#include<stdlib.h>
#include <sstream>  
#include <iostream>
#include <stdint.h>
#include "Toolkits.h"

static bool runing;
static volatile int64_t currentTime=0;

int64_t getCurrentTime(){return currentTime;}

static void* updateTime(void * pParam){
    while(runing)
	{
        struct timeval tv;
        gettimeofday(&tv,NULL);
        currentTime = tv.tv_sec;
        sleep(1);
    }
}

bool startTookits(void * pParam){
    if(runing)return true;
    runing = true;
	//启动连接池维护线程
	pthread_t localThreadId;
	if ( pthread_create( &localThreadId, NULL, updateTime, pParam ) != 0 )
	{
		cout  << "pthread_create failed" <<endl;
		return false;
	}
	pthread_detach(localThreadId);
    return true;
}

void stopTookits(){
    runing = false;
}

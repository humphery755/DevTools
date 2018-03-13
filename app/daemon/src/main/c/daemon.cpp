#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <jni.h>
#include "daemon.h"
#include "hashmap.h"
#include "setproctitle.ci"
#include <string>
#include <iostream>

#define K_NAME "wrapper.name"
#define K_NAME_LEN strlen(K_NAME)
#define K_VM_WORKDIR "wrapper.workdir"
#define K_VM_WORKDIR_LEN strlen(K_VM_WORKDIR)
#define K_PID "wrapper.pid"
#define K_PID_LEN strlen(K_PID)
#define K_LOGFILE "wrapper.logfile"
#define K_LOGFILE_LEN strlen(K_LOGFILE)
#define K_DEBUG "wrapper.debug"
#define K_DEBUG_LEN strlen(K_DEBUG)
#define K_VM_CHECKS_CMD "wrapper.checks.cmd"
#define K_VM_CHECKS_CMD_LEN strlen(K_VM_CHECKS_CMD)
#define K_VM_CHECKS_KEYWORD "wrapper.checks.keyword"
#define K_VM_CHECKS_KEYWORD_LEN strlen(K_VM_CHECKS_KEYWORD)
#define K_VM_CHECKS_RETRYTIMES "wrapper.checks.retryTimes"
#define K_VM_CHECKS_RETRYTIMES_LEN strlen(K_VM_CHECKS_RETRYTIMES)
#define K_VM_CHECKS_INTERVAL "wrapper.checks.interval"
#define K_VM_CHECKS_INTERVAL_LEN strlen(K_VM_CHECKS_INTERVAL)
#define K_VM_EXIT_TIMEOUT "wrapper.jvm_exit.timeout"
#define K_VM_EXIT_TIMEOUT_LEN strlen(K_VM_EXIT_TIMEOUT)

#define K_MAIN_CLASS "wrapper.work.main.class"
#define K_MAIN_CLASS_LEN strlen(K_MAIN_CLASS)
#define K_MAIN_ARGS "wrapper.work.main.args"
#define K_MAIN_ARGS_LEN strlen(K_MAIN_ARGS)
#define K_VM_OPTIONS "wrapper.work.vm.options"
#define K_VM_OPTIONS_LEN strlen(K_VM_OPTIONS)
#define K_VM_STDOUT "wrapper.work.stdout"
#define K_VM_STDOUT_LEN strlen(K_VM_STDOUT)
#define K_VM_DAEMON "wrapper.work.daemon"
#define K_VM_DAEMON_LEN strlen(K_VM_DAEMON)

#define K_EXT_MAIN_CLASS "wrapper.master.main.class"
#define K_EXT_MAIN_CLASS_LEN strlen(K_EXT_MAIN_CLASS)
#define K_EXT_MAIN_ARGS "wrapper.master.main.args"
#define K_EXT_MAIN_ARGS_LEN strlen(K_EXT_MAIN_ARGS)
#define K_EXT_VM_OPTIONS "wrapper.master.vm.options"
#define K_EXT_VM_OPTIONS_LEN strlen(K_EXT_VM_OPTIONS)

using namespace std;

#define dd(...) fprintf(stderr, "wrapper-%d *** %s.%d: ",getpid(), __func__,__LINE__); \
            fprintf(stderr, __VA_ARGS__)

#define ddn(...) fprintf(stderr, __VA_ARGS__)

char *wrapperPid;
int shutdown=0;
int shmid;
int wpid = -1;
int mpid = -1;
wrapper_shm_t *shm;
int jvmExitTimeout = 60;
int count_sigterm = 0;
bool debug = false;

typedef struct{
	char *mainClass;
	char **args;
	int argsLen;
	char **jvmOptions;
	int jvmOptsLen;
}wrapper_ext_conf_t;

typedef struct wrapper_conf_s{
	char *name;
	char *mainClass;
	char **mainArgs;
	int mainArgsLen;
	char **jvmOptions;
	int jvmOptsLen;
	char *logfile;	
	bool stdout;
	bool daemon;
	char *workdir;
	char *checks_cmd;
	char *checks_keyword;
	int	  checks_interval;
	int	  checks_retryTimes;	
	wrapper_ext_conf_t ext_conf;
}wrapper_conf_t;

int daemon()
{
    int fd;
    switch (fork()) {
    case -1:
        return (-1);
    case 0:
        break;
    default:
        _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return (-1);

    return (0);
}

//char* to jstring
jstring s2Jstring(JNIEnv* env, const char* pat)
{
	jclass strClass = env->FindClass("Ljava/lang/String;");
	jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
	jbyteArray bytes = env->NewByteArray(strlen(pat));
	env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*)pat);
	jstring encoding = env->NewStringUTF("utf-8");
	return (jstring)env->NewObject(strClass, ctorID, bytes, encoding);
}
char* jstring2s(JNIEnv* env, jstring jstr)
{
	char* rtn = NULL;
	jclass clsstring = env->FindClass("java/lang/String");
	jstring strencode = env->NewStringUTF("utf-8");
	jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
	jsize alen = env->GetArrayLength(barr);
	jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
	if (alen > 0)
	{
		rtn = (char*)malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	env->ReleaseByteArrayElements(barr, ba, 0);
	return rtn;
}

void disableSTD(){
	dd("The JVM disableSTD...\n");
	int fd;
	if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        if(dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
        }
        if(dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
        }
        if(dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
        }

        if (fd > STDERR_FILENO) {
            if(close(fd) < 0) {
                perror("close");
            }
        }
    }else{
		perror("open file");
	}
}

void del_tail(char *str,int len){
	char *p;	
	for(p=str+(len-1); *p==' ' || *p=='\t'|| *p=='\n' ; --p)*p='\0';	
}
enum conf_tag_status{INIT=0,MAIN_CLASS,MAIN_ARGS,EXT_MAIN_ARGS,JVM_OPTION,EXT_JVM_OPTION};
void parse_properties(const char *pathname, wrapper_conf_t *conf){
    FILE *file;
	int buffSize=1024*2;
    char _line[buffSize];
	char *line=_line;
    int i = 0,ignore,offset;
	int length = 0, equal = 1; //equal will record the location of the '='  
    char *begin,*value;
	
    offset=ignore=0;
	enum conf_tag_status tag;
    file = fopen(pathname, "r");
	if(file==NULL)
	{
		perror("can't open configuration file");
		exit(0);
	}
	conf->stdout=true;
	conf->workdir=NULL;
	conf->logfile=NULL;
	conf->mainClass=NULL;
	conf->name=NULL;
	conf->mainArgsLen=0;
	conf->jvmOptsLen=0;
	conf->ext_conf.mainClass=NULL;
	conf->ext_conf.jvmOptsLen=0;
	conf->checks_cmd=NULL;
	conf->checks_keyword=NULL;
	conf->checks_interval=30;
	conf->checks_retryTimes=3;
	
    while(fgets(line, buffSize, file)){
		if (!strncmp(K_VM_STDOUT, line,K_VM_STDOUT_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			if(!strncmp("false", line,5))
				conf->stdout=false;
			else if(!strncmp("true", line,4))
				conf->stdout=true;
			else{
				dd("Invalid %s value: %s(true/false).\n", K_VM_STDOUT,line);
				exit(-1);
			}
				
		}else if (!strncmp(K_VM_DAEMON, line,K_VM_DAEMON_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			if(!strncmp("false", line,5))
				conf->daemon=false;
			else if(!strncmp("true", line,4))
				conf->daemon=true;
			else{
				dd("Invalid %s value: %s(true/false).\n", K_VM_DAEMON,line);
				exit(-1);
			}
				
		}else if (!strncmp(K_VM_WORKDIR, line,K_VM_WORKDIR_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			conf->workdir=(char*)malloc(length-equal);
			strncpy(conf->workdir, line, length - equal); 	
			del_tail(conf->workdir,length - equal);
		}else if (!strncmp(K_LOGFILE, line,K_LOGFILE_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			conf->logfile=(char*)malloc(length-equal);
			strncpy(conf->logfile, line, length - equal); 	
			del_tail(conf->logfile,length - equal);
		}else if (!strncmp(K_NAME, line,K_NAME_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			conf->name=(char*)malloc(length-equal);
			strncpy(conf->name, line, length - equal); 	
			del_tail(conf->name,length - equal);
		}else if (!strncmp(K_PID, line,K_PID_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			wrapperPid=(char*)malloc(length-equal);
			strncpy(wrapperPid, line, length - equal); 	
			del_tail(wrapperPid,length - equal);
		}else if (!strncmp(K_DEBUG, line,K_DEBUG_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			if(!strncmp("false", line,5))
				debug=false;
			else if(!strncmp("true", line,4))
				debug=true;
			else{
				dd("Invalid %s value: %s(true/false).\n", K_DEBUG,line);
				exit(-1);
			}
		}else if (!strncmp(K_VM_EXIT_TIMEOUT, line,K_VM_EXIT_TIMEOUT_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			char tmp[128];
			strncpy(tmp, line, (length - equal)>128?128:(length - equal));
			del_tail(tmp,length - equal);
			jvmExitTimeout=atoi(tmp);
		}else if (!strncmp(K_VM_CHECKS_CMD, line,K_VM_CHECKS_CMD_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			conf->checks_cmd=(char*)malloc(length-equal);
			strncpy(conf->checks_cmd, line, length - equal); 	
			del_tail(conf->checks_cmd,length - equal);
		}else if (!strncmp(K_VM_CHECKS_KEYWORD, line,K_VM_CHECKS_KEYWORD_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			conf->checks_keyword=(char*)malloc(length-equal);
			strncpy(conf->checks_keyword, line, length - equal); 	
			del_tail(conf->checks_keyword,length - equal);
		}else if (!strncmp(K_VM_CHECKS_INTERVAL, line,K_VM_CHECKS_INTERVAL_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			char tmp[128];
			strncpy(tmp, line, (length - equal)>128?128:(length - equal));
			del_tail(tmp,length - equal);
			conf->checks_interval=atoi(tmp);
		}else if (!strncmp(K_VM_CHECKS_RETRYTIMES, line,K_VM_CHECKS_RETRYTIMES_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			char tmp[128];
			strncpy(tmp, line, (length - equal)>128?128:(length - equal));
			del_tail(tmp,length - equal);
			conf->checks_retryTimes=atoi(tmp);
		}else if (!strncmp(K_MAIN_CLASS, line,K_MAIN_CLASS_LEN)){
			tag=MAIN_CLASS;
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			conf->mainClass=(char*)malloc(length-equal);			
			strncpy(conf->mainClass, line, length - equal);
			del_tail(conf->mainClass,length - equal);
		}else if (!strncmp(K_EXT_MAIN_CLASS, line,K_EXT_MAIN_CLASS_LEN)){
			length = 0, equal = 1;
			length = strlen(line);
			for(begin = line; *begin != '=' && equal <= length; begin ++){  
					equal++;  
			}
			line+=equal;
			conf->ext_conf.mainClass=(char*)malloc(length-equal);			
			strncpy(conf->ext_conf.mainClass, line, length - equal);
			del_tail(conf->ext_conf.mainClass,length - equal);
		}else if (!strncmp(K_MAIN_ARGS, line,K_MAIN_ARGS_LEN)){
			tag=MAIN_ARGS;
		}else if (!strncmp(K_EXT_MAIN_ARGS, line,K_EXT_MAIN_ARGS_LEN)){
			tag=EXT_MAIN_ARGS;
		}else if (!strncmp(K_VM_OPTIONS, line,K_VM_OPTIONS_LEN)){
			tag=JVM_OPTION;
		}else if (!strncmp(K_EXT_VM_OPTIONS, line,K_EXT_VM_OPTIONS_LEN)){
			tag=EXT_JVM_OPTION;
		}else if (!strncmp("[", line,1)){
			offset=i;
			ignore=0;
		}else if (!strncmp("]", line,1)){
			switch(tag){
				case MAIN_ARGS:
					conf->mainArgsLen=i-offset-ignore-1;
					break;
				case EXT_MAIN_ARGS:
					conf->ext_conf.argsLen=i-offset-ignore-1;
					break;
				case JVM_OPTION:
					conf->jvmOptsLen=i-offset-ignore-1;
					break;
				case EXT_JVM_OPTION:
					conf->ext_conf.jvmOptsLen=i-offset-ignore-1;
					break;
				default:
					break;
			}
			tag=INIT;
		}else if (!strncmp("#", line,1)){
			ignore++;
		}else{
			switch(tag){
				case MAIN_ARGS:
					break;
				case EXT_MAIN_ARGS:
					break;
				case JVM_OPTION:
					break;
				case EXT_JVM_OPTION:
					break;
				default:
					for(begin=line; *begin==' ' || *begin=='\t'|| *begin=='\n' ; ++begin);
					if( *begin==0 )
					{
						break;
					}
					dd("Invalid configuration item: %s at %s:%d.\n", line,pathname,i);
					exit(-1);
			}
			
		}
        i++;
    }
    fclose(file);
	
	if(conf->jvmOptsLen==0){
		dd("args: 'wrapper.java.vm.options' is required");
		exit(-1);
	}
	if(conf->mainArgsLen>0){
		conf->mainArgs=(char**)calloc(conf->mainArgsLen, sizeof(char*));
	}
	
	conf->jvmOptions=(char**)calloc(conf->jvmOptsLen, sizeof(char*));
	
	if(conf->ext_conf.jvmOptsLen>0){
		conf->ext_conf.jvmOptions=(char**)calloc(conf->ext_conf.jvmOptsLen, sizeof(char*));
	}
	
	if(conf->ext_conf.argsLen>0){
		conf->ext_conf.args=(char**)calloc(conf->ext_conf.argsLen, sizeof(char*));
	}
	
	i = offset = 0;
    file = fopen(pathname, "r");
    while(fgets(line, buffSize, file)){
		if (!strncmp(K_MAIN_ARGS, line,K_MAIN_ARGS_LEN)){
			tag=MAIN_ARGS;
			i=0;
		}else if (!strncmp(K_EXT_MAIN_ARGS, line,K_EXT_MAIN_ARGS_LEN)){
			tag=EXT_MAIN_ARGS;
			i=0;
		}else if (!strncmp(K_VM_OPTIONS, line,K_VM_OPTIONS_LEN)){
			tag=JVM_OPTION;
			i=0;
		}else if (!strncmp(K_EXT_VM_OPTIONS, line,K_EXT_VM_OPTIONS_LEN)){
			tag=EXT_JVM_OPTION;
			i=0;
		}else if (!strncmp("[", line,1)){
			continue;
		}else if (!strncmp("]", line,1)){
			tag=INIT;
			continue;
		}else if (!strncmp("#", line,1)){
			continue;
		}else{
			switch(tag){
				case MAIN_ARGS:
					length = strlen(line);
					conf->mainArgs[i]=(char*)malloc(length+1);
					strncpy(conf->mainArgs[i], line, length);
					del_tail(conf->mainArgs[i],length);
					i++;
					break;
				case EXT_MAIN_ARGS:
					length = strlen(line);
					conf->ext_conf.args[i]=(char*)malloc(length+1);
					strncpy(conf->ext_conf.args[i], line, length);
					del_tail(conf->ext_conf.args[i],length);
					i++;
					break;
				case JVM_OPTION:
					length = strlen(line);
					conf->jvmOptions[i]=(char*)malloc(length+1);
					strncpy(conf->jvmOptions[i], line, length);
					del_tail(conf->jvmOptions[i],length);
					i++;
					break;
				case EXT_JVM_OPTION:
					length = strlen(line);
					conf->ext_conf.jvmOptions[i]=(char*)malloc(length+1);
					strncpy(conf->ext_conf.jvmOptions[i], line, length);
					del_tail(conf->ext_conf.jvmOptions[i],length);
					i++;
					break;
				default:
					break;
			}
		}
		
    }
    fclose(file);
} 


int callJVM(wrapper_conf_t *conf){
	JNIEnv *env;
	JavaVM * jvm;
	int x;
	dd("Launching a JVM...\n");
	count_sigterm=0;
	jint ret;
	JavaVMOption options[conf->jvmOptsLen];	
	for(int i=0;i<conf->jvmOptsLen;i++){
		options[i].optionString = (char*)conf->jvmOptions[i];
		ddn("%s ",conf->jvmOptions[i]);
	}
	ddn("\n");
	JavaVMInitArgs vm_args;
	memset(&vm_args, 0, sizeof(vm_args));
	vm_args.version = JNI_VERSION_1_6;
	vm_args.options = options;
	vm_args.nOptions = conf->jvmOptsLen;
	vm_args.ignoreUnrecognized = JNI_TRUE;

	ret = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
	if(ret < 0) { dd("Unable to Launch JVM\n"); return -1;}
	//delete options;
	jclass mainClaz;  
    jmethodID mid;  

    mainClaz = env->FindClass(conf->mainClass);  
	if(mainClaz==NULL){
		dd("Can't findClass: %s\n",conf->mainClass);
		goto destroy;
	}
	
    mid = env->GetStaticMethodID(mainClaz, "main", "([Ljava/lang/String;)V"); 
	if(mid==NULL){
		perror("Can't GetStaticMethodID by \"main\": ");
		goto destroy;
	}
	
	jobjectArray jargs;
	jargs=NULL;
	x=0;
	if(strcmp(conf->mainClass,"org/daemon/wrapper/LibC")==0){
		x=1;
	}
	if(conf->mainArgsLen+x > 0){
		jclass strClass = env->FindClass("Ljava/lang/String;");
		jargs=env->NewObjectArray(conf->mainArgsLen+x,strClass,NULL);
	}
	if(strcmp(conf->mainClass,"org/daemon/wrapper/LibC")==0){
		char *strshmid[25];
		sprintf((char*)strshmid,"%d",shmid);
		jstring jstr;
		jstr = s2Jstring(env,(const char*)strshmid);
		env->SetObjectArrayElement(jargs, 0, jstr);
	}
	for(int i=0;i<conf->mainArgsLen;i++){
		jstring jstr = s2Jstring(env,conf->mainArgs[i]);
		env->SetObjectArrayElement(jargs, i+x, jstr);
	}
	dd("Call %s.main([Ljava/lang/String;)V\n",conf->mainClass);
	if(!conf->stdout)disableSTD();
	env->CallStaticObjectMethod(mainClaz, mid,jargs);
destroy:
	dd("The JVM Destroying...\n");
	jvm->DestroyJavaVM();
}

int callExtJVM(wrapper_ext_conf_t *conf){
	JNIEnv *env;
	JavaVM * jvm;
	char strshmid[25];
	dd("Launching EXT JVM...\n");
	
	jint ret;
	JavaVMOption options[conf->jvmOptsLen];	
	for(int i=0;i<conf->jvmOptsLen;i++){
		options[i].optionString = (char*)conf->jvmOptions[i];
		ddn("%s ",conf->jvmOptions[i]);
	}
	ddn("\n");
	JavaVMInitArgs vm_args;
	memset(&vm_args, 0, sizeof(vm_args));
	vm_args.version = JNI_VERSION_1_6;
	vm_args.options = options;
	vm_args.nOptions = conf->jvmOptsLen;
	vm_args.ignoreUnrecognized = JNI_FALSE;

	ret = JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
	if(ret < 0) { dd("Unable to Launch JVM\n"); exit(0);}
	//delete options;
	jclass mainClaz;  
    jmethodID mid;  

    mainClaz = env->FindClass(conf->mainClass);  
	if(mainClaz==NULL){
		dd("Can't findClass: %s\n",conf->mainClass);
		goto destroy;
	}
	
    mid = env->GetStaticMethodID(mainClaz, "main", "([Ljava/lang/String;)V"); 
	if(mid==NULL){
		perror("Can't GetStaticMethodID by \"main\": ");
		goto destroy;
	}
	
	jobjectArray jargs;
	jclass strClass;
	strClass = env->FindClass("Ljava/lang/String;");
	jargs=env->NewObjectArray(1+conf->argsLen,strClass,NULL);
	   
    sprintf((char*)strshmid,"%d",shmid);
	jstring jstr;
	jstr = s2Jstring(env,(const char*)strshmid);
	env->SetObjectArrayElement(jargs, 0, jstr);
	
	for(int i=1;i<conf->argsLen+1;i++){
		jstring jstr = s2Jstring(env,conf->args[i-1]);
		env->SetObjectArrayElement(jargs, i, jstr);
	}

	dd("Call %s.main([Ljava/lang/String;)V\n",conf->mainClass);
	env->CallStaticObjectMethod(mainClaz, mid,jargs);
	
destroy:
	dd("The JVM Destroying...\n");
	jvm->DestroyJavaVM();
	return -1;
}

void sigmpty_handler(int dunno){}

struct  timeval  sigterm_start_time;
void sigterm_handler(int dunno)
{
  switch (dunno)
  {
    case SIGHUP:
		perror("Get a signal -- SIGHUP ");
	case SIGTERM:
		count_sigterm++;
		if(count_sigterm==1){
			dd("SIGTERM signal.  Shutting down pid: %d.\n",shm->wp.pid);
			gettimeofday(&sigterm_start_time,NULL);	
		}else if(((count_sigterm++) % 10000)==0){
			dd("SIGTERM signal.  Shutting down pid: %d.\n",shm->wp.pid);
			sleep(2);
		}else{
			struct  timeval  end;
			unsigned long timer;
			gettimeofday(&end,NULL);
			timer = 1000000 * (end.tv_sec-sigterm_start_time.tv_sec)+ end.tv_usec-sigterm_start_time.tv_usec;
			if(timer/1000000 > jvmExitTimeout){
				dd("The process will be killed: SIGTERM timeout(%d)\n",timer/1000000);
				kill(shm->wp.pid,9);
				break;
			}
			sleep(1);
		}
		shutdown=1;
		if(shm->wp.pid>0)
			kill(shm->wp.pid,15);
		break;
    default:
		dd("Get a signal -- %d ",dunno);
		break;
  }
  return;
}

int m_restart_times=0;
int w_restart_times=0;

struct  timeval  start;
struct  timeval  end;
unsigned long timer;

void sigchld_handler(int signo) 
{ 
       pid_t   pid; 
   int     stat; 
   while((pid = waitpid(-1, NULL, WNOHANG)) > 0){
	   //dd("The Process %d terminated\n", pid); 
	   if(pid==mpid){
			dd("Stop Master Process[%d, times: %d]...\n",mpid,m_restart_times);
			kill(mpid,15);
			pid = waitpid(mpid, NULL, 0);
			shm->mp.pid=0;
			mpid=-1;
			m_restart_times++;
			gettimeofday(&end,NULL);
			timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;
			if(timer/1000000 < 10)
				sleep(10);
			gettimeofday(&start,NULL);
		}
		wpid = shm->wp.pid;
		if(pid==wpid || pid==mpid){
			dd("Stop Work Process[%d, times: %d] ...\n",wpid,w_restart_times);
			kill(wpid,15);
			pid = waitpid(wpid, NULL, 0);
			shm->wp.pid=0;
			wpid=-1;
			w_restart_times++;
			gettimeofday(&end,NULL);
			timer = 1000000 * (end.tv_sec-start.tv_sec)+ end.tv_usec-start.tv_usec;
			if(timer/1000000 < 10)
				sleep(10);
			gettimeofday(&start,NULL);
		}
   }
	return; 
}

char bash_buffer[2048];
int checks_tries = 0;
int checks_interval=0;
int checkJVM(wrapper_conf_t *conf){
	if(conf->checks_cmd==NULL || conf->checks_keyword==NULL)return 0;
	FILE  *fp=popen(conf->checks_cmd,"r");
	if (!fp) {
		perror("can't popen wrapper.pid: ");
        return -1;
	}
	memset(bash_buffer,'\0',sizeof(bash_buffer));
	fread(bash_buffer,sizeof(char),sizeof(bash_buffer),fp);
	pclose(fp);
	if(debug)
	dd("checks result: %s\n",bash_buffer);
	if(strstr(bash_buffer,conf->checks_keyword)!=NULL){
		checks_tries++;
		if(checks_tries>conf->checks_retryTimes){
			checks_tries=0;
			dd("checks failure: %s\n",bash_buffer);
			return 1;
		}
		return 0;
	}
	checks_tries=0;
	return 0;
}
int forkWork(wrapper_conf_t *conf){
	gettimeofday(&start,NULL);
	shm->wp.pid=0;
	int wpid=fork();
	if(wpid==-1)exit(-1);
	if(wpid==0){
		char tmpName[strlen(conf->name)+7];
		sprintf(tmpName,"%s[work]",conf->name);
		//spt_init(argc,argv);
		setproctitle(tmpName);
		sigset_t           set;
		sigemptyset(&set);
		sigaddset(&set, SIGHUP);
		//signal(SIGTERM, sigmpty_handler);
		prctl(PR_SET_PDEATHSIG, SIGHUP);		
		int ret = callJVM(conf);
		exit(ret);
	}
	shm->wp.pid=wpid;
	return wpid;
}

int forkMaster(wrapper_conf_t *conf){
	gettimeofday(&start,NULL);
	if(conf->ext_conf.mainClass!=NULL){
		shm->dp.status=1;//wait;
		shm->mp.pid=0;
		int spid=fork();
		if(spid==-1)exit(-1);
		if(spid==0){
			char tmpName[strlen(conf->name)+9];
			sprintf(tmpName,"%s[master]",conf->name);
			//spt_init(argc,argv);
			setproctitle(tmpName);
			sigset_t           set;
			sigemptyset(&set);
			sigaddset(&set, SIGHUP);
			//signal(SIGTERM, sigmpty_handler);
			prctl(PR_SET_PDEATHSIG, SIGHUP);
			int ret = callExtJVM(&conf->ext_conf);
			if(ret==1)return 0;
			shm->dp.status=0;
			exit(ret);
		}
		shm->mp.pid=spid;
		return spid;
	}
	return 0;
}

int main(int argc, char *argv[]){
	int i;
	char *p=NULL;
	char *confFile = NULL;
	char *cmd = NULL;
	
	wrapperPid=(char*)"wrapper.pid";

	for (i = 1; i < argc; i++)
	{
		if (!strcmp("-f", argv[i]))
		{
			confFile = argv[i + 1];
			i++;
		}else if (!strcmp("-s", argv[i]))
		{
			cmd = argv[i + 1];
			i++;
		}else{
				dd("usage: %s -f config-file or -s "
				 "stop|restart\n", argv[0]);
				return 0;
		}
	}

	if(confFile==NULL){
		dd("args: '-f' is required");
		return 0;
	}

	wrapper_conf_t *conf=new wrapper_conf_t;
	parse_properties(confFile, conf);
	if(conf->workdir!=NULL && chdir(conf->workdir) != 0) {
		perror("chdir");
		exit(-1);
	}
	
	FILE *fp;
	char pr[10];
	int oldpid;
	if(cmd!=NULL){
		if((fp=fopen(wrapperPid,"rb"))==NULL)
		{
			dd("can't open %s: ",wrapperPid);
			perror("");
			exit(0);
		}
		memset(pr,'\0',10);
		fread(pr,1, 10, fp);
		oldpid=atoi(pr);
		if (!strcmp("stop", cmd)){
			dd("Shutting down pid: %d\n",oldpid);
			while(kill(oldpid,SIGTERM)==0){
				sleep(1);
			}			
			//system("cat wrapper.pid|xargs kill");
			fclose(fp);
			remove(wrapperPid);
			return 0;
		}else if (!strcmp("restart", cmd)){
			dd("Shutting down(restart) pid: %d\n",oldpid);
			while(kill(oldpid,SIGTERM)==0){
				sleep(1);
			}
			
		}else{
			dd("usage: %s -s stop|restart\n", argv[0]);
			fclose(fp);
			return 0;
		}
		fclose(fp);
	}
	
	if((access(wrapperPid,F_OK))==0)
	{
		dd("The %s is exsit: ",wrapperPid);
		perror("");
		exit(0);
	}
	
	if(conf->mainClass==NULL){
		dd("args: 'wrapper.java.main.class' is required");
		return 0;
	}
	p=conf->mainClass;
	while(*p){
		if(*p=='.')*p='/';p++;
	}
	
	if(conf->ext_conf.mainClass!=NULL){
		p=conf->ext_conf.mainClass;
		while(*p){
			if(*p=='.')*p='/';p++;
		}	
	}
	
	//end conf parse
	// start init
	// set process title
	{
		char tmpName[strlen(conf->name)+9];
		sprintf(tmpName,"%s[daemon]",conf->name);
		spt_init(argc,argv);
		setproctitle(tmpName);
	}
	
	daemon();
	int pid=getpid();
	sigset_t           set;
	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGCHLD);
	signal(SIGTERM, sigterm_handler);	
	signal(SIGCHLD, sigchld_handler);
	if(conf->workdir!=NULL && chdir(conf->workdir) != 0) {
		perror("chdir");
		exit(-1);
	}
	int fd;
	if ((fd = open(conf->logfile, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR)) != -1) {
        if(dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            return (-1);
        }
        if(dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            return (-1);
        }
        if(dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            return (-1);
        }

        if (fd > STDERR_FILENO) {
            if(close(fd) < 0) {
                perror("close");
                return (-1);
            }
        }
    }else{
		perror("can't open file");
	}
	
	//Create shared memory
	i=0;
	while((shmid = shmget(IPC_PRIVATE, sizeof(wrapper_shm_t), IPC_CREAT|0660))<=0){
		i++;
		if(i>3){
			dd("Create a shared memory Fail!");
			return -1;
		}
	}
	
	shm = (wrapper_shm_t *)shmat(shmid, (void *)0, 0);
	if (shm == (void *)-1) 
	{
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	shm->dp.pid=getpid();
	shm->mp.pid=0;
	shm->wp.pid=0;
	dd("Create a shared memory[%d] Success!\n",shmid);
	
	if((fp=fopen(wrapperPid,"wb"))==NULL)
	{
		dd("can't open %s: ",wrapperPid);
		perror("");
		exit(0);
	}
	memset(pr,'\0',10);
	sprintf((char*)pr,"%d",(int)getpid());
	fwrite((void*)pr,sizeof(char),sizeof(pr),fp);
	fclose(fp);

	dd("--> Wrapper Started as Daemon\n");
	
	int mret = 0;
	int status;	
	do {
		if(mpid<=0){
			mpid=forkMaster(conf);
			if(mpid<0){
				sleep(1);
				continue;
			}
		}
		while(shm->dp.status==1 && !shutdown){
			dd("Wait Master Process notify\n");
			sleep(5);
		}
		if(shutdown)break;
		wpid=shm->wp.pid;
		if(wpid<=0){
			wpid=forkWork(conf);
			if(wpid<=0){
				sleep(1);
				continue;
			}
		}else{
			wpid=shm->wp.pid;
		}
		sleep(conf->checks_interval);
		if(checkJVM(conf)==1){
			if(wpid > 0){
				kill(wpid,15);
				while(wpid == shm->mp.pid)
					sleep(1);
			}
		}
	} while (!shutdown);
	if(shm->mp.pid>0)kill(shm->mp.pid,9);
	dd("<-- Wrapper Stopped\n");
}

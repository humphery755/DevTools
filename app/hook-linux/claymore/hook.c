#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h>
#include <limits.h>

#include <netinet/in.h> 
#include <linux/ip.h>
#include <linux/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#include "miner.h"
 
#if defined(RTLD_NEXT)
#  define REAL_LIBC RTLD_NEXT
#else
#  define REAL_LIBC ((void *) -1L)
#endif
#define FN(ptr, type, name, args)  ptr = (type (*)args)dlsym (REAL_LIBC, name)

static pthread_mutex_t 	lock = PTHREAD_MUTEX_INITIALIZER;
static int              inited=0;


/* 
 * get cmdline from PID. Read progress info form /proc/$PID.  
 */  
  
static int read_to_buf(const char *filename, void *buf, int len)  
{  
    int fd;  
    int ret;  
      
    if(buf == NULL || len < 0){  
        printf("%s: illegal para\n", __func__);  
        return -1;  
    }  
  
    memset(buf, 0, len);  
    fd = open(filename, O_RDONLY);  
    if(fd < 0){  
        perror("open:");  
        return -1;  
    }  
    ret = read(fd, buf, len);  
    close(fd);  
    return ret;  
}  
  
static char *get_cmdline_from_pid(int pid, char *buf, int len)  
{  
    char filename[32];  
    char *name = NULL;  
    int n = 0;  
      
    if(pid < 1 || buf == NULL || len < 0){  
        printf("%s: illegal para\n", __func__);  
        return NULL;  
    }  
          
    snprintf(filename, 32, "/proc/%d/cmdline", pid);  
    n = read_to_buf(filename, buf, len);  
    if(n < 0)  
        return NULL;  
  
    if(buf[n-1]=='\n')  
        buf[--n] = 0;  
  
    name = buf;  
    while(n) {  
        if(((unsigned char)*name) < ' ')  
            *name = ' ';  
        name++;  
        n--;  
    }  
    *name = 0;  
    name = NULL;  
  
    if(buf[0])  
        return buf;  
  
    return NULL;      
} 

inline static void init(){
    if(inited)return;
    pthread_mutex_lock(&lock);
    if(inited){
        pthread_mutex_unlock(&lock);
        return;
    }

    char buf[1024];
    get_cmdline_from_pid(getppid(), buf, 1024);
    printf("PPID [ %d ] cmdline: %s\n", getppid(), buf); 
    initializer(buf);
    inited=1;
    pthread_mutex_unlock(&lock);
}

struct hostent* gethostbyname(const char *domain){
    //fprintf(stdout,"1 socket gethostbyname(%s) hooked!!\n",domain);
    init();
    static struct hostent* (*func)(const char*);
    FN(func,struct hostent*,"gethostbyname", (const char*));
    //Syelog("2 socket gethostbyname(%s) hooked!!\n",domain);

    struct hostent *ht = Mine_gethostbyname(domain,func);
    //Syelog("3 socket gethostbyname(%s) hooked!!\n",domain);
    return ht;
}

int getaddrinfo( const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result ){
    init();
    static int (*func)(const char *, const char *, const struct addrinfo *, struct addrinfo **);
    FN(func,int,"getaddrinfo", (const char *, const char *, const struct addrinfo *, struct addrinfo **)); 

    printf("### socket getaddrinfo(%s,%s,%p,%p) hooked!!\n",hostname,service,hints,result);

    int res = (*func) (hostname,service,hints,result);

    return res;
}


int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) 
{ 
    init();
    static int (*func)(int, const struct sockaddr *, socklen_t);
    FN(func,int,"connect", (int, const struct sockaddr *, socklen_t)); 
    // 1. 区分IPv4、IPv6     (struct sockaddr *addr)->sa_family

    int optval;
    int optlen = sizeof(int);
    getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &optval, &optlen);

    if(optval==SOCK_STREAM){
        //printf("TCP connect hooked!!\n");

        Mine_connect(addr,addrlen);
    }

    /*
    print the log
    获取、打印参数信息的时候需要注意
    1. 加锁
    2. 拷贝到本地栈区变量中
    3. 然后再打印
    调试的时候发现直接获取打印会导致core dump
    */
    //printf("socket connect hooked!!\n");

    int ret_code = (*func) (sockfd, addr, addrlen);
    int tmp_errno = errno;
    if (ret_code == -1 && tmp_errno != EINPROGRESS)
    {
        return ret_code;
    }
    //return (*func) (sockfd, (const struct sockaddr *) addr, (socklen_t)addrlen);
    //return (*func) (sockfd, addr, addrlen);
    /** */


    return ret_code;
}  

ssize_t send(int sockfd, const void *buff, size_t nbytes, int flags){
    Mine_send_enter(buff,nbytes);
    static ssize_t (*func)(int, const void*, size_t, int);
    FN(func,ssize_t,"send", (int, const void*, size_t, int)); 
    //printf("socket send(%d,%s,%zd,%d) hooked!!\n",sockfd, (char*)buff, nbytes, flags);

    ssize_t len = (*func) (sockfd, buff, nbytes, flags);

    return len;
}


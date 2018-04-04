#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>  
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include <netinet/in.h> 
#include <linux/ip.h>
#include <linux/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
 
#if defined(RTLD_NEXT)
#  define REAL_LIBC RTLD_NEXT
#else
#  define REAL_LIBC ((void *) -1L)
#endif

#define FN(ptr, type, name, args)  ptr = (type (*)args)dlsym (REAL_LIBC, name)

//ssize_t write(int fd, const void*buf,size_t nbytes);
//ssize_t read(int fd,void *buf,size_t nbyte)
//ssize_t send(int sockfd, const void *buff, size_t nbytes, int flags);
//ssize_t recv(int sockfd, void *buff, size_t nbytes, int flags);
//struct hostent *gethostbyname(const char *name);
//int getaddrinfo( const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result );

int execve(const char *filename, char *const argv[], char *const envp[])
{
    static int (*func)(const char *, char **, char **);
    FN(func,int,"execve",(const char *, char **const, char **const)); 

    //print the log
    printf("filename: %s, argv[0]: %s, envp:%s\n", filename, argv[0], envp);

    return (*func) (filename, (char**) argv, (char **) envp);
} 

int execv(const char *filename, char *const argv[]) 
{
    static int (*func)(const char *, char **);
    FN(func,int,"execv", (const char *, char **const)); 

    //print the log
    printf("filename: %s, argv[0]: %s\n", filename, argv[0]);

    return (*func) (filename, (char **) argv);
}  

struct hostent* gethostbyname(const char *domain){
    static struct hostent* (*func)(const char*);
    FN(func,struct hostent*,"gethostbyname", (const char*));

    printf("socket gethostbyname(%s) hooked!!\n",domain);

    struct hostent *ht = (*func) (domain);

    return ht;
}

int getaddrinfo( const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result ){
    static int (*func)(const char *, const char *, const struct addrinfo *, struct addrinfo **);
    FN(func,int,"getaddrinfo", (const char *, const char *, const struct addrinfo *, struct addrinfo **)); 

    printf("socket getaddrinfo(%s,%s,%p,%p) hooked!!\n",hostname,service,hints,result);

    int res = (*func) (hostname,service,hints,result);

    return res;
}


int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) 
{ 
    static int (*func)(int, const struct sockaddr *, socklen_t);
    FN(func,int,"connect", (int, const struct sockaddr *, socklen_t)); 
    // 1. 区分IPv4、IPv6     (struct sockaddr *addr)->sa_family

    int optval;
    int optlen = sizeof(int);
    getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &optval, &optlen);

    if(optval==SOCK_STREAM){
        printf("tcp connect !!\n");
    }
    /*
    print the log
    获取、打印参数信息的时候需要注意
    1. 加锁
    2. 拷贝到本地栈区变量中
    3. 然后再打印
    调试的时候发现直接获取打印会导致core dump
    */
    printf("socket connect hooked!!\n");

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
    static ssize_t (*func)(int, const void*, size_t, int);
    FN(func,ssize_t,"connect", (int, const void*, size_t, int)); 
    printf("socket send(%d,%s,%d,%d) hooked!!\n",sockfd, buff, nbytes, flags);

    ssize_t len = (*func) (sockfd, buff, nbytes, flags);

    return len;
}

int init_module(void *module_image, unsigned long len, const char *param_values) 
{ 
    static int (*func)(void *, unsigned long, const char *);
    FN(func,int,"init_module",(void *, unsigned long, const char *)); 

    /*
    print the log
    lkm的加载不需要取参数，只需要捕获事件本身即可
    */
    printf("lkm load hooked!!\n");

    return (*func) ((void *)module_image, (unsigned long)len, (const char *)param_values);
}


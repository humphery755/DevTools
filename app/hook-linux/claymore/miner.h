

#ifndef ___HOOK_H
#define ___HOOK_H

#if defined(__WINDOWS_) || defined(_WIN32)

#else

#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#include <netdb.h>

#define Syelog(S,F,...) fprintf(stdout,F,##__VA_ARGS__);
///printf(F,##__VA_ARGS__) 
#define BOOL int
#define TRUE 1
#define FALSE 0

typedef struct hostent* (*f_gethostbyname)(const char*);

#endif 


//ssize_t send(int sockfd, const void *buff, size_t nbytes, int flags);
void Mine_send_enter(char* data, int len);
//int getaddrinfo( const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result );
struct hostent* Mine_gethostbyname(const char* hname,f_gethostbyname fun);

//int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) 
int Mine_connect(const struct sockaddr *addr, socklen_t addrlen);

BOOL initializer(char *cmdline);


#endif

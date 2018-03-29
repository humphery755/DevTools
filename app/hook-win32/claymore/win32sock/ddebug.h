#ifndef _DDEBUG_H_
#define _DDEBUG_H_

//#include <iostream>
#include "syelog.h"

VOID _PrintDump(SOCKET socket, PCHAR pszData, INT cbData);
VOID _PrintEnter(PCSTR psz, ...);
VOID _PrintExit(PCSTR psz, ...);
VOID _Print(PCSTR psz, ...);

VOID AssertMessage(CONST PCHAR pszMsg, CONST PCHAR pszFile, ULONG nLine);

//////////////////////////////////////////////////////////////////////////////
#pragma warning(disable:4127)   // Many of our asserts are constants.


#define ASSERT_ALWAYS(x)   \
    do {                                                        \
    if (!(x)) {                                                 \
            AssertMessage(#x, __FILE__, __LINE__);              \
            DebugBreak();                                       \
    }                                                           \
    } while (0)

#ifndef NDEBUG
#define ASSERT(x)           ASSERT_ALWAYS(x)
#else
#define ASSERT(x)
#endif

extern BOOL s_bLog;
extern LONG s_nTlsIndent;
extern LONG s_nTlsThread;
extern LONG s_nThreadCnt;

#endif // _DDEBUG_H_

#define _WIN32_WINNT        0x0400
#define WIN32
#define NT
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define DBG_TRACE   0

#if _MSC_VER >= 1300
#include <winsock2.h>
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
//#include <tlhelp32.h>
//#include <tchar.h>
#include "detours.h"
#include "syelog.h"
//#include "winternl.h"
#include "ddebug.h"
//
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////// Logging System.
//
VOID _PrintDump(SOCKET socket, PCHAR pszData, INT cbData)
{
    if (pszData && cbData > 0) {
        CHAR szBuffer[256];
        PCHAR pszBuffer = szBuffer;
        INT cbBuffer = 0;
        INT nLines = 0;

        while (cbData > 0) {
            if (nLines > 20) {
                *pszBuffer++ = '.';
                *pszBuffer++ = '.';
                *pszBuffer++ = '.';
                cbBuffer += 3;
                break;
            }

            if (*pszData == '\t') {
                *pszBuffer++ = '\\';
                *pszBuffer++ = 't';
                cbBuffer += 2;
                pszData++;
                cbData--;
                continue;
            }
            if (*pszData == '\r') {
                *pszBuffer++ = '\\';
                *pszBuffer++ = 'r';
                cbBuffer += 2;
                pszData++;
                cbData--;
                continue;
            }
            else if (*pszData == '\n') {
                *pszBuffer++ = '\\';
                *pszBuffer++ = 'n';
                cbBuffer += 2;
                *pszBuffer++ = '\0';
                _Print("%p:   %hs\n", socket, szBuffer);
                nLines++;
                pszBuffer = szBuffer;
                cbBuffer = 0;
                pszData++;
                cbData--;
                continue;
            }
            else if (cbBuffer >= 80) {
                *pszBuffer++ = '\0';
                _Print("%p:   %hs\n", socket, szBuffer);
                nLines++;
                pszBuffer = szBuffer;
                cbBuffer = 0;
            }

            if (*pszData < ' ' || *pszData >= 127) {
                *pszBuffer++ = '\\';
                *pszBuffer++ = 'x';
                *pszBuffer++ = "0123456789ABCDEF"[(*pszData & 0xf0) >> 4];
                *pszBuffer++ = "0123456789ABCDEF"[(*pszData & 0x0f)];
                cbBuffer += 4;
            }
            else {
                *pszBuffer++ = *pszData;
            }
            cbBuffer++;
            pszData++;
            cbData--;
        }

        if (cbBuffer > 0) {
            *pszBuffer++ = '\0';
            _Print("%p:   %hs\n", socket, szBuffer);
        }
    }
}


BOOL s_bLog = 1;


VOID _PrintEnter(const CHAR *psz, ...)
{
    DWORD dwErr = GetLastError();

    LONG nIndent = 0;
    LONG nThread = 0;
    if (s_nTlsIndent >= 0) {
        nIndent = (LONG)(LONG_PTR)TlsGetValue(s_nTlsIndent);
        TlsSetValue(s_nTlsIndent, (PVOID)(LONG_PTR)(nIndent + 1));
    }
    if (s_nTlsThread >= 0) {
        nThread = (LONG)(LONG_PTR)TlsGetValue(s_nTlsThread);
    }

    if (s_bLog && psz) {
        CHAR szBuf[1024];
        PCHAR pszBuf = szBuf;
        PCHAR pszEnd = szBuf + ARRAYSIZE(szBuf) - 1;
        LONG nLen = (nIndent > 0) ? (nIndent < 35 ? nIndent * 2 : 70) : 0;
        *pszBuf++ = (CHAR)('0' + ((nThread / 100) % 10));
        *pszBuf++ = (CHAR)('0' + ((nThread / 10) % 10));
        *pszBuf++ = (CHAR)('0' + ((nThread / 1) % 10));
        *pszBuf++ = ' ';
        while (nLen-- > 0) {
            *pszBuf++ = ' ';
        }

        va_list  args;
        va_start(args, psz);

        while ((*pszBuf++ = *psz++) != 0 && pszBuf < pszEnd) {
            // Copy characters.
        }
        *pszEnd = '\0';
        SyelogV(SYELOG_SEVERITY_INFORMATION,
                szBuf, args);

        va_end(args);
    }
    SetLastError(dwErr);
}

VOID _PrintExit(const CHAR *psz, ...)
{
    DWORD dwErr = GetLastError();

    LONG nIndent = 0;
    LONG nThread = 0;
    if (s_nTlsIndent >= 0) {
        nIndent = (LONG)(LONG_PTR)TlsGetValue(s_nTlsIndent) - 1;
        ASSERT(nIndent >= 0);
        TlsSetValue(s_nTlsIndent, (PVOID)(LONG_PTR)nIndent);
    }
    if (s_nTlsThread >= 0) {
        nThread = (LONG)(LONG_PTR)TlsGetValue(s_nTlsThread);
    }

    if (s_bLog && psz) {
        CHAR szBuf[1024];
        PCHAR pszBuf = szBuf;
        PCHAR pszEnd = szBuf + ARRAYSIZE(szBuf) - 1;
        LONG nLen = (nIndent > 0) ? (nIndent < 35 ? nIndent * 2 : 70) : 0;
        *pszBuf++ = (CHAR)('0' + ((nThread / 100) % 10));
        *pszBuf++ = (CHAR)('0' + ((nThread / 10) % 10));
        *pszBuf++ = (CHAR)('0' + ((nThread / 1) % 10));
        *pszBuf++ = ' ';
        while (nLen-- > 0) {
            *pszBuf++ = ' ';
        }

        va_list  args;
        va_start(args, psz);

        while ((*pszBuf++ = *psz++) != 0 && pszBuf < pszEnd) {
            // Copy characters.
        }
        *pszEnd = '\0';
        SyelogV(SYELOG_SEVERITY_INFORMATION,
                szBuf, args);

        va_end(args);
    }
    SetLastError(dwErr);
}

VOID _Print(const CHAR *psz, ...)
{
    DWORD dwErr = GetLastError();

    LONG nIndent = 0;
    LONG nThread = 0;
    if (s_nTlsIndent >= 0) {
        nIndent = (LONG)(LONG_PTR)TlsGetValue(s_nTlsIndent);
    }
    if (s_nTlsThread >= 0) {
        nThread = (LONG)(LONG_PTR)TlsGetValue(s_nTlsThread);
    }

    if (s_bLog && psz) {
        CHAR szBuf[1024];
        PCHAR pszBuf = szBuf;
        PCHAR pszEnd = szBuf + ARRAYSIZE(szBuf) - 1;
        LONG nLen = (nIndent > 0) ? (nIndent < 35 ? nIndent * 2 : 70) : 0;
        *pszBuf++ = (CHAR)('0' + ((nThread / 100) % 10));
        *pszBuf++ = (CHAR)('0' + ((nThread / 10) % 10));
        *pszBuf++ = (CHAR)('0' + ((nThread / 1) % 10));
        *pszBuf++ = ' ';
        while (nLen-- > 0) {
            *pszBuf++ = ' ';
        }

        va_list  args;
        va_start(args, psz);

        while ((*pszBuf++ = *psz++) != 0 && pszBuf < pszEnd) {
            // Copy characters.
        }
        *pszEnd = '\0';
        SyelogV(SYELOG_SEVERITY_INFORMATION,
                szBuf, args);

        va_end(args);
    }

    SetLastError(dwErr);
}

VOID AssertMessage(CONST PCHAR pszMsg, CONST PCHAR pszFile, ULONG nLine)
{
    Syelog(SYELOG_SEVERITY_FATAL,
           "ASSERT(%s) failed in %s, line %d.\n", pszMsg, pszFile, nLine);
}
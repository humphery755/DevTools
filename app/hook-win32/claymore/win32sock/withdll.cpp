//////////////////////////////////////////////////////////////////////////////
//
//  Test DetourCreateProcessWithDll function (withdll.cpp).
//
//  Microsoft Research Detours Package, Version 3.0.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
#include <stdio.h>
#include <windows.h>
#include <detours.h>
#pragma warning(push)
#if _MSC_VER > 1400
#pragma warning(disable:6102 6103) // /analyze warnings
#endif
#include <strsafe.h>
#pragma warning(pop)

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
//
//  This code verifies that the named DLL has been configured correctly
//  to be imported into the target process.  DLLs must export a function with
//  ordinal #1 so that the import table touch-up magic works.
//
struct ExportContext
{
    BOOL    fHasOrdinal1;
    ULONG   nExports;
};

static BOOL CALLBACK ExportCallback(_In_opt_ PVOID pContext,
                                    _In_ ULONG nOrdinal,
                                    _In_opt_ LPCSTR pszSymbol,
                                    _In_opt_ PVOID pbTarget)
{
    (void)pContext;
    (void)pbTarget;
    (void)pszSymbol;

    ExportContext *pec = (ExportContext *)pContext;

    if (nOrdinal == 1) {
        pec->fHasOrdinal1 = TRUE;
    }
    pec->nExports++;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//

//////////////////////////////////////////////////////////////////////////////
//

static BYTE buffer[65536];

typedef union
{
    struct
    {
        DWORD Signature;
        IMAGE_FILE_HEADER FileHeader;
    } ih;

    IMAGE_NT_HEADERS32 ih32;
    IMAGE_NT_HEADERS64 ih64;
} IMAGE_NT_HEADER;

struct SECTIONS
{
    PBYTE   pbBeg;
    PBYTE   pbEnd;
    CHAR    szName[16];
} Sections[256];
DWORD SectionCount = 0;
DWORD Bitness = 0;

PCHAR FindSectionName(PBYTE pbBase, PBYTE& pbEnd)
{
    for (DWORD n = 0; n < SectionCount; n++) {
        if (Sections[n].pbBeg == pbBase) {
            pbEnd = Sections[n].pbEnd;
            return Sections[n].szName;
        }
    }
    pbEnd = NULL;
    return NULL;
}

ULONG PadToPage(ULONG Size)
{
    return (Size & 0xfff)
        ? Size + 0x1000 - (Size & 0xfff)
        : Size;
}

BOOL GetSections(HANDLE hp, PBYTE pbBase)
{
    DWORD beg = 0;
    DWORD cnt = 0;
    SIZE_T done;
    IMAGE_DOS_HEADER idh;

    if (!ReadProcessMemory(hp, pbBase, &idh, sizeof(idh), &done) || done != sizeof(idh)) {
        return FALSE;
    }

    if (idh.e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    IMAGE_NT_HEADER inh;
    if (!ReadProcessMemory(hp, pbBase + idh.e_lfanew, &inh, sizeof(inh), &done) || done != sizeof(inh)) {
        printf("No Read\n");
        return FALSE;
    }

    if (inh.ih.Signature != IMAGE_NT_SIGNATURE) {
        printf("No NT\n");
        return FALSE;
    }

    beg = idh.e_lfanew
        + FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader )
        + inh.ih.FileHeader.SizeOfOptionalHeader;
    cnt = inh.ih.FileHeader.NumberOfSections;
    Bitness = (inh.ih32.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) ? 32 : 64;
#if 0
    printf("%d %d count=%d\n", beg, Bitness, cnt);
#endif

    IMAGE_SECTION_HEADER ish;
    for (DWORD n = 0; n < cnt; n++) {
        if (!ReadProcessMemory(hp, pbBase + beg + n * sizeof(ish), &ish, sizeof(ish), &done) || done != sizeof(ish)) {
            printf("No Read\n");
            return FALSE;
        }
        Sections[n].pbBeg = pbBase + ish.VirtualAddress;
        Sections[n].pbEnd = pbBase + ish.VirtualAddress + PadToPage(ish.Misc.VirtualSize);
        memcpy(Sections[n].szName, ish.Name, sizeof(ish.Name));
        Sections[n].szName[sizeof(ish.Name)] = '\0';
#if 0
        printf("--- %p %s\n", Sections[n].pbBeg, Sections[n].szName);
#endif
    }
    SectionCount = cnt;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////// main.
//
int CDECL main(int argc, char **argv)
{
    //BOOLEAN fNeedHelp = FALSE;
    LPCSTR rpszDllsRaw[256];
    LPCSTR rpszDllsOut[256];
    DWORD nDlls = 0;

    for (DWORD n = 0; n < ARRAYSIZE(rpszDllsRaw); n++) {
        rpszDllsRaw[n] = NULL;
        rpszDllsOut[n] = NULL;
    }
    nDlls =1;
    rpszDllsRaw[0]="trctcp64.dll";

    //int arg = 1;
    (int)argc;
    (char**)argv;

    /////////////////////////////////////////////////////////// Validate DLLs.
    //
    for (DWORD n = 0; n < nDlls; n++) {
        CHAR szDllPath[1024];
        PCHAR pszFilePart = NULL;

        if (!GetFullPathNameA(rpszDllsRaw[n], ARRAYSIZE(szDllPath), szDllPath, &pszFilePart)) {
            printf("withdll.exe: Error: %s is not a valid path name..\n",
                   rpszDllsRaw[n]);
            return 9002;
        }

        DWORD c = (DWORD)strlen(szDllPath) + 1;
        PCHAR psz = new CHAR [c];
        StringCchCopyA(psz, c, szDllPath);
        rpszDllsOut[n] = psz;

        HMODULE hDll = LoadLibraryExA(rpszDllsOut[n], NULL, DONT_RESOLVE_DLL_REFERENCES);
        if (hDll == NULL) {
            printf("withdll.exe: Error: %s failed to load (error %d).\n",
                   rpszDllsOut[n],
                   GetLastError());
            return 9003;
        }

        ExportContext ec;
        ec.fHasOrdinal1 = FALSE;
        ec.nExports = 0;
        DetourEnumerateExports(hDll, &ec, ExportCallback);
        FreeLibrary(hDll);

        if (!ec.fHasOrdinal1) {
            printf("withdll.exe: Error: %s does not export ordinal #1.\n",
                   rpszDllsOut[n]);
            printf("             See help entry DetourCreateProcessWithDllEx in Detours.chm.\n");
            return 9004;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    CHAR szCommand[2048];
    CHAR szExe[1024];
    CHAR szFullExe[1024] = "\0";
    PCHAR pszFileExe = NULL;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    szCommand[0] = L'\0';
    StringCchCopyA(szExe, sizeof(szExe), "ethdcrminer64.exe");
    StringCchCopyA(szCommand, sizeof(szCommand), "ethdcrminer64.exe ");
    int arg = 1;
    //StringCchCopyA(szExe, sizeof(szExe), argv[arg]);
    for (; arg < argc; arg++) {
        if (strchr(argv[arg], ' ') != NULL || strchr(argv[arg], '\t') != NULL) {
            StringCchCatA(szCommand, sizeof(szCommand), "\"");
            StringCchCatA(szCommand, sizeof(szCommand), argv[arg]);
            StringCchCatA(szCommand, sizeof(szCommand), "\"");
        }
        else {
            StringCchCatA(szCommand, sizeof(szCommand), argv[arg]);
        }

        if (arg + 1 < argc) {
            StringCchCatA(szCommand, sizeof(szCommand), " ");
        }
    }
    
    printf("withdll.exe: Starting: `%s'\n", szCommand);
    for (DWORD n = 0; n < nDlls; n++) {
        printf("withdll.exe:   with `%s'\n", rpszDllsOut[n]);
    }
    fflush(stdout);

    DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    SetLastError(0);
    SearchPathA(NULL, szExe, ".exe", ARRAYSIZE(szFullExe), szFullExe, &pszFileExe);
    if (!DetourCreateProcessWithDllsA(szFullExe[0] ? szFullExe : NULL, szCommand,
                                     NULL, NULL, TRUE, dwFlags, NULL, NULL,
                                     &si, &pi, nDlls, rpszDllsOut, NULL)) {
        DWORD dwError = GetLastError();
        printf("withdll.exe: DetourCreateProcessWithDllEx failed: %d\n", dwError);
        if (dwError == ERROR_INVALID_HANDLE) {
#if DETOURS_64BIT
            printf("withdll.exe: Can't detour a 32-bit target process from a 64-bit parent process.\n");
#else
            printf("withdll.exe: Can't detour a 64-bit target process from a 32-bit parent process.\n");
#endif
        }
        ExitProcess(9009);
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwResult = 0;
    if (!GetExitCodeProcess(pi.hProcess, &dwResult)) {
        printf("withdll.exe: GetExitCodeProcess failed: %d\n", GetLastError());
        return 9010;
    }

    for (DWORD n = 0; n < nDlls; n++) {
        if (rpszDllsOut[n] != NULL) {
            delete[] rpszDllsOut[n];
            rpszDllsOut[n] = NULL;
        }
    }

    return dwResult;
}
//
///////////////////////////////////////////////////////////////// End of File.

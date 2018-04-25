##############################################################################
##
##  Utility to trace WinSock TCP APIs.
##
##  Microsoft Research Detours Package, Version 3.0.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

#!include common.mak

LIBS=$(LIBS) kernel32.lib ws2_32.lib
#BIND=..\..\bin.X64
BIND=$(BIN_HOME)
##############################################################################

all: 
    $(CC) /Od /LDd /c testdll.cc main.cc
    link /debug /MACHINE:X64 /dll testdll.obj    
    link /out:main.exe *.obj /DYNAMICBASE "kernel32.lib"
    dumpbin -exports testdll.dll
    $(CC) /D_USRDLL /D_WINDLL testdll.cc /link /debug /DLL /OUT:libtestdll.dll
    dumpbin -exports libtestdll.dll
#link  /out:testdll.lib testdll.obj
#cl.exe /D_USRDLL /D_WINDLL factorial.c /link /DLL /OUT:libfactorial.dll

clean:
 del *.obj *.exp *.ilk *.pdb *.res *.dll *.exe *.lib

################################################################# End of File.
#/OUT:"C:\workspace\test\test\demo\Dll1\Debug\Dll1.dll" /MANIFEST /NXCOMPAT /PDB:"C:\workspace\test\test\demo\Dll1\Debug\Dll1.pdb" 
#/DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" 
#/IMPLIB:"C:\workspace\test\test\demo\Dll1\Debug\Dll1.lib" 
#/DEBUG /DLL /MACHINE:X86 /INCREMENTAL /PGD:"C:\workspace\test\test\demo\Dll1\Debug\Dll1.pgd" /SUBSYSTEM:WINDOWS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" 
#/ManifestFile:"Debug\Dll1.dll.intermediate.manifest" /ERRORREPORT:PROMPT /NOLOGO /TLBID:1 
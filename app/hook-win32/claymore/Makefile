##############################################################################
##
##  Makefile for Detours Test Programs.
##
##  Microsoft Research Detours Package, Version 3.0.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##
##  Note:
##    syelog, setdll, and withdll must be built first because a number of the
##    other samples depend on them.
##

ROOT=..
!include .\common.mak

##############################################################################

all:
    cd "$(MAKEDIR)\syelog"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS)
!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"
    cd "$(MAKEDIR)\excep"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS)
!ENDIF
    cd "$(MAKEDIR)"

clean:
    cd "$(MAKEDIR)\syelog"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    cd "$(MAKEDIR)"
    -rmdir lib32 2>nul
    -rmdir lib64 2>nul
    -rmdir include 2>nul

realclean:
    cd "$(MAKEDIR)\syelog"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) realclean
    cd "$(MAKEDIR)"
    -rmdir lib32 2>nul
    -rmdir lib64 2>nul
    -rmdir include 2>nul

test:
    cd "$(MAKEDIR)\syelog"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) test
    cd "$(MAKEDIR)"

##
################################################################# End of File.

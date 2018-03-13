
JAVA_HOME=/usr/local/jdk1.8.0_111
JARS_PATH =  $(shell find lib/ $(FILTE_FILES) -regex '.*\.jar\|.*\.so' |tr '\n' ':')

default:
	g++ -g -o daemon daemon.cpp hashmap.c -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -L$(JAVA_HOME)/jre/lib/amd64/server -ljvm
#	javac -cp .:$(JARS_PATH) com/alibaba/rocketmq/broker/daemon/LibC.java
#	javah -cp . com.alibaba.rocketmq.broker.daemon.LibC
	g++ -fPIC -c org_daemon_wrapper_LibC.cpp -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -L$(JAVA_HOME)/jre/lib/amd64/server -ljvm
	g++ -shared -Wl,-soname,liborg_daemon_wrapper_LibC.so.1  -o liborg_daemon_wrapper_LibC.so  org_daemon_wrapper_LibC.o
#java -cp . -Djava.library.path='.' com.alibaba.rocketmq.broker.daemon.LibC javac -cp . com/alibaba/rocketmq/broker/daemon/LibC.java

java:
	javac -cp . -d ./  ../java/org/daemon/wrapper/LibC.java
	javac -cp .:$(JARS_PATH) -d ./  $(shell find ../../test/java/ $(FILTE_FILES) -regex '.*\.java' |tr '\n' ' ')
	rm -rf lib/libdaemon.jar
	jar -cvfm lib/libdaemon.jar MANIFEST.MF org/daemon/wrapper/*.class org/daemon/test/*.class

test:
	LD_LIBRARY_PATH=$(JAVA_HOME)/jre/lib/amd64/server ./daemon -f ./wrapper.conf

stop:
	LD_LIBRARY_PATH=$(JAVA_HOME)/jre/lib/amd64/server ./daemon -f ./wrapper.conf -s stop
clean:
	rm -rf daemon *.o *.so logs/* org lib/libdaemon.jar

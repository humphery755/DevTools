# **********************************************************************
#
# Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
#g++ -shared -rdynamic -m64 -fvisibility=hidden -Wall -Werror -pthread -fPIC -g -L/home/humphery/tmp/ice-3.6.3/cpp/lib64 -o libtdd_sequence_SequenceServiceI.so -Wl,-h,libtdd_sequence_SequenceServiceI.so   generated/tdd_sequence.o Classes/tdd_sequence_SequenceServiceI.o -lIceBox  -lIce -lIceUtil
# **********************************************************************
MYSQL_HOME=/usr/local/mysql
THIRDPARTY_HOME=/usr/local/thirdparty
BENCHMARK_HOME=/usr/local/benchmark
ZDB_HOME=$(THIRDPARTY_HOME)/libzdb
SSL_LIB=/usr/local/openssl-1.1.0e/lib
ICE_HOME=$(HOME)/Ice-3.6.3
#ICE_HOME=/usr/local/ice-3.6.3
GLOG_HOME=$(THIRDPARTY_HOME)/glog
LDFLAGS := -shared -rdynamic -m64 -fvisibility=hidden -Wall -Werror -pthread -fPIC -g
DEBUG=1
TARGET=SessionManager

GEN_SRC = $(wildcard ./generated/*.cpp) 

SOURCES =  $(shell find generated -regex '.*\.cpp\|.*\.c')	\
	$(shell find Classes -regex '.*\.cpp\|.*\.c')

SHAREDLIBS = -lIceBox $(BZIP2_RPATH_LINK) -lIce -lIceUtil -lFreeze -lIceGrid -lIceBox -L$(MYSQL_HOME)/lib -lmysqlclient -L$(ZDB_HOME)/lib -lzdb -L$(GLOG_HOME)/lib -lglog

include ice.mak

INCLUDES=-Iobjs -Igenerated -IClasses -I$(ICE_HOME)/include -I$(MYSQL_HOME)/include -I$(ZDB_HOME)/include -I$(ZDB_HOME)/include/zdb -I$(GLOG_HOME)/include	
#INCLUDES=-I$(ICE_HOME)/include -I$(MYSQLCON)/include -I$(PUBLICFUNC)/include -I$(LIBREDIS) -I$(REDISCLIENT)/ -I$(DATACACHEPRO)/include -I$(CURL)/include/curl -I$(BOOST) -I$(GLOG_HOME)/include -I$(COMMROOT)

GEN_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(GEN_SRC))
#STATICLIBS += -L$(LOG4C_HOME)/lib/liblog4c.a

####### Build rules
$(TARGET): $(OBJECTS) $(CORE_MAKEFILE_LIST)
	@mkdir -p $(@D)
	g++ -m64 -O0 -fPIC  $(LDFLAGS) -o lib$(TARGET).so $(OBJECTS) $(STATICLIBS) $(SHAREDLIBS) $(LIBS)

client: $(GEN_OBJS)
	rm -rf client*
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) -Itests -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2 -ldb_cxx-5.3 tests/Client.cpp Classes/IceExtClientUtil.cpp -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o client

t: 
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(SSL_LIB) ./client 1 --benchmark_counters_tabular=true

gdb: 
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(SSL_LIB) gdb ./client -x gdbinit

test:$(GEN_OBJS) 
	#g++ -m64 -O0 -fPIC  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(STATICLIBS) $(SHAREDLIBS) $(LIBS) tests/create.cpp
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(SHAREDLIBS) $(GEN_OBJS) -L$(ICE_HOME)/lib64 -lGlacier2 -lIce -lIceUtil -lFreeze -ldb_cxx-5.3 tests/Client.cpp Classes/IceExt_IceClientUtil.cpp Classes/dbutil.cpp Classes/DefaultSequenceDao.cpp -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o client
	#LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$LD_LIBRARY_PATH ./client 10 --benchmark_counters_tabular=true

db:$(GEN_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(SHAREDLIBS) $(GEN_OBJS) -L$(ICE_HOME)/lib64 -lGlacier2 -lIce -lIceUtil -lFreeze -ldb_cxx-5.3 tests/Dbtest.cpp tests/IceExt_IceClientUtil.cpp Classes/dbutil.cpp Classes/DefaultSequenceDao.cpp -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o dbtest
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH) ./dbtest 10 --benchmark_counters_tabular=true
##@echo "$(LOG_LINK)$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(SHAREDLIBS) $(STATICLIBS) $(LIBS)"
##$(LOG_LINK)$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(SHAREDLIBS) $(STATICLIBS) $(LIBS)
server: $(GEN_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2 -ldb_cxx-5.3 tests/Server.cpp Classes/IceExtClientUtil.cpp Classes/SessionServerI.cpp -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o server
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(SSL_LIB) ./server

x: $(GEN_OBJS)
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH) ./server
####### Compile
$(OBJ_DIR)/%.o: ../%.cpp $(CORE_MAKEFILE_LIST)
	@mkdir -p $(@D)
	@echo "1:$(OBJ_DIR)/%.o: ../%.cpp $(CORE_MAKEFILE_LIST)"
	@echo "1.1:$(LOG_CXX)$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@"
	$(LOG_CXX)$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp $(CORE_MAKEFILE_LIST)
	@mkdir -p $(@D)
	@echo "2:$(LOG_CXX)$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@"
	$(LOG_CXX)$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

$(OBJ_DIR)/%.o: %.c $(CORE_MAKEFILE_LIST)
	@mkdir -p $(@D)
	@echo "$(LOG_CC)$(CC) $(CCFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@"
	$(LOG_CC)$(CC) $(CCFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

GEN::
	$(ICE_HOME)/bin/slice2cpp --help
	$(ICE_HOME)/bin/slice2cpp -I$(ICE_HOME)/slice --output-dir generated/ $(shell find slice -regex '.*\.ice')


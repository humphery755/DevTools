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
ICE_HOME=/usr/local/ice-3.7.1
GLOG_HOME=$(THIRDPARTY_HOME)/glog
LDFLAGS := -shared -rdynamic -m64 -fvisibility=hidden -Wall -Werror -pthread -fPIC -g
DEBUG=1
TARGET=SessionManager

GEN_SRC = $(wildcard ./generated/*.cpp) 

SOURCES =  $(shell find generated -regex '.*\.cpp\|.*\.c')	\
	$(shell find Classes -regex '.*\.cpp\|.*\.c')

SHAREDLIBS = -lIceBox $(BZIP2_RPATH_LINK) -lIce -lIceGrid -lGlacier2 -lIceSSL -L$(MYSQL_HOME)/lib -L$(GLOG_HOME)/lib -lglog -lcurl

include ice.mak

INCLUDES=-Iobjs -Igenerated -IClasses -I$(ICE_HOME)/include -I$(MYSQL_HOME)/include -I$(GLOG_HOME)/include -I$(BENCHMARK_HOME)/include
#INCLUDES=-I$(ICE_HOME)/include -I$(MYSQLCON)/include -I$(PUBLICFUNC)/include -I$(LIBREDIS) -I$(REDISCLIENT)/ -I$(DATACACHEPRO)/include -I$(CURL)/include/curl -I$(BOOST) -I$(GLOG_HOME)/include -I$(COMMROOT)

GEN_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(GEN_SRC))
TEST_Client_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, Classes/Toolkits.cpp Classes/IceExtClientUtil.cpp tests/Client.cpp)
TEST_Server_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, tests/Server.cpp)
TEST_Benchmark_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, Classes/Toolkits.cpp Classes/IceExtClientUtil.cpp tests/Benchmark.cpp)
#STATICLIBS += -L$(LOG4C_HOME)/lib/liblog4c.a

####### Build rules
$(TARGET): $(OBJECTS) $(CORE_MAKEFILE_LIST)
	@mkdir -p $(@D)
	g++ -m64 -O0 -fPIC  $(LDFLAGS) -o lib/lib$(TARGET).so $(OBJECTS) $(STATICLIBS) $(SHAREDLIBS) $(LIBS)

benchmark: clean $(GEN_OBJS) $(TEST_Benchmark_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) $(TEST_Benchmark_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o benchmark
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(MYSQL_HOME)/lib ./benchmark 5 --benchmark_counters_tabular=true

client: clean $(GEN_OBJS) $(TEST_Client_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) $(TEST_Client_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o client
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(MYSQL_HOME)/lib ./client 5 --benchmark_counters_tabular=true

gdb: 
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(SSL_LIB) gdb ./client -x gdbinit

server: $(OBJECTS) $(TEST_Server_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(OBJECTS) $(TEST_Server_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o server
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(SSL_LIB) ./server


clean:
	rm -rf client* *.o $(OBJ_DIR)/*.o

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


# **********************************************************************
# sudo yum install unixODBC unixODBC-devel
# Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
#g++ -shared -rdynamic -m64 -fvisibility=hidden -Wall -Werror -pthread -fPIC -g -L/home/humphery/tmp/ice-3.6.3/cpp/lib -o libtdd_sequence_SequenceServiceI.so -Wl,-h,libtdd_sequence_SequenceServiceI.so   generated/tdd_sequence.o Classes/tdd_sequence_SequenceServiceI.o -lIceBox  -lIce -lIceUtil
# **********************************************************************
MYSQL_HOME=/usr/local/mysqlconnector
THIRDPARTY_HOME=/usr/local/thirdparty
BENCHMARK_HOME=/usr/local/benchmark
ICE_HOME=/usr/local/ice-3.7.1
OPENSSL_HOME=/usr/local/openssl-1.0.2k
GLOG_HOME=$(THIRDPARTY_HOME)/glog
ZK_HOME=/usr/local/zkclient
LDFLAGS := -shared -rdynamic -m64 -fvisibility=hidden -Wall -Werror -pthread -fPIC
CXXFLAGS :=
CCFLAGS += 
DEBUG=1
TARGET=tddl_sequence_SequenceServiceI

GEN_SRC = $(wildcard ./generated/*.cpp) 

SOURCES =  $(shell find generated -regex '.*\.cpp\|.*\.c')	\
	$(shell find Classes -regex '.*\.cpp\|.*\.c')

SHAREDLIBS = -lIceBox $(BZIP2_RPATH_LINK) -lIce -lIceGrid -lIceBox -L$(GLOG_HOME)/lib -lglog -L$(ZK_HOME)/lib -lzookeeper_mt -luuid -L$(MYSQL_HOME)/lib -lmysqlcppconn -L$(OPENSSL_HOME)/lib -lssl

include ice.mak

INCLUDES=-Iobjs -Igenerated -IClasses -I$(ICE_HOME)/include -I$(MYSQL_HOME)/include -I$(GLOG_HOME)/include -I$(ZK_HOME)/include -I$(BENCHMARK_HOME)/include
#INCLUDES=-I$(ICE_HOME)/include -I$(MYSQLCON)/include -I$(PUBLICFUNC)/include -I$(LIBREDIS) -I$(REDISCLIENT)/ -I$(DATACACHEPRO)/include -I$(CURL)/include/curl -I$(BOOST) -I$(GLOG_HOME)/include -I$(COMMROOT)

GEN_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(GEN_SRC))
#STATICLIBS += -L$(LOG4C_HOME)/lib/liblog4c.a
TEST_DBPool_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, Classes/Toolkits.cpp Classes/MySQLDBPool.cpp tests/test_MySQLDBPool.cpp)
TEST_Server_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, tests/Server.cpp)
TEST_Client_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, Classes/Toolkits.cpp Classes/IceExtClientUtil.cpp tests/Client.cpp)
TEST_SnowflakeIdWorker_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, Classes/SnowflakeIdWorker.cpp tests/test_SnowflakeIdWorker.cpp)
TEST_DefaultSequenceWorker_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, Classes/Toolkits.cpp Classes/DefaultSequenceWorker.cpp tests/test_DefaultSequenceWorker.cpp Classes/MySQLDBPool.cpp)
TEST_OrderSequenceAdapter_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, Classes/Toolkits.cpp Classes/MySQLDBPool.cpp Classes/OrderSequenceAdapter.cpp \
	Classes/zookeeper_helper.cpp Classes/SequenceServiceI.cpp Classes/DefaultSequenceWorker.cpp Classes/SnowflakeIdWorker.cpp tests/test_OrderSequenceAdapter.cpp)

####### Build rules
$(TARGET): $(OBJECTS) $(CORE_MAKEFILE_LIST)
	@mkdir -p $(@D)
	g++ -m64 -O0 -fPIC  $(LDFLAGS) -o lib$(TARGET).so $(OBJECTS) $(STATICLIBS) $(SHAREDLIBS) $(LIBS)

server: $(OBJECTS) $(TEST_Server_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(OBJECTS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2  $(TEST_Server_OBJS) -L $(BENCHMARK_HOME)/lib  -lpthread -o server
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(ZK_HOME)/lib ./server --Ice.Config=tests/config.service

client: $(GEN_OBJS) $(TEST_Client_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2 -lGlacier2CryptPermissionsVerifier $(TEST_Client_OBJS) -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o test
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib:$(BENCHMARK_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(MYSQL_HOME)/lib:$(ZK_HOME)/lib ./test 5 --benchmark_counters_tabular=true
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH) gdb ./test


testsf: $(GEN_OBJS) $(TEST_SnowflakeIdWorker_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2 \
	$(TEST_SnowflakeIdWorker_OBJS) -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o test
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(ZK_HOME)/lib:$(LD_LIBRARY_PATH) ./test 2 --benchmark_counters_tabular=true

testd: $(GEN_OBJS) $(TEST_DefaultSequenceWorker_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2 \
	$(TEST_DefaultSequenceWorker_OBJS) -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o test
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(ZK_HOME)/lib:$(LD_LIBRARY_PATH) ./test 10 --benchmark_counters_tabular=true

svcinstrument: $(GEN_OBJS)
	g++ -finstrument-functions -Ltests/pvtrace -linstrument \
	-finstrument-functions-exclude-file-list=$(ICE_HOME)/include,$(MYSQL_HOME)/include,$(GLOG_HOME)/include,$(ZK_HOME)/include,/usr/local/gcc-4.8.5,/usr/include \
	$(CXXFLAGS) $(INCLUDES) $(DEFINES) tests/test_DefaultSequenceDao.cpp \
	Classes/MySQLDBPool.cpp Classes/DefaultSequenceDao.cpp $(GEN_OBJS) -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib -L$(GLOG_HOME)/lib -L$(MYSQL_HOME)/lib -L$(ICE_HOME)/lib -lIce -lIceUtil -lIceBox -lmysqlcppconn -lglog -lpthread -lbenchmark -o testdao
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):tests/pvtrace ./testdao 1000 --benchmark_counters_tabular=true

dbpool: $(TEST_DBPool_OBJS)
	g++ $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(TEST_DBPool_OBJS) \
	-L $(BENCHMARK_HOME)/lib -L$(GLOG_HOME)/lib -L$(MYSQL_HOME)/lib -lmysqlcppconn -L$(OPENSSL_HOME)/lib -lssl -lcrypto -lglog -lpthread -lbenchmark -o test
	LD_LIBRARY_PATH=.:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(OPENSSL_HOME)/lib:$(LD_LIBRARY_PATH) ./test 1 --benchmark_counters_tabular=true

testos: $(GEN_OBJS) $(TEST_OrderSequenceAdapter_OBJS)
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2  \
	$(TEST_OrderSequenceAdapter_OBJS) -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o test
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(ZK_HOME)/lib:$(LD_LIBRARY_PATH) ./test 2 --benchmark_counters_tabular=true

tools:
	g++ -g tests/SeqTools.cpp -o tools

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
	$(ICE_HOME)/bin/slice2cpp $(shell find slice -regex '.*\.ice') --output-dir generated/

#$(ICE_HOME)/bin/slice2freeze --output-dir generated/ --dict StringSeqRangeMap,string,tddl::sequences::SequenceRangePO StringSeqRangeMap slice/tdd_sequence.ice


# **********************************************************************
# sudo yum install unixODBC unixODBC-devel
# Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
#g++ -shared -rdynamic -m64 -fvisibility=hidden -Wall -Werror -pthread -fPIC -g -L/home/humphery/tmp/ice-3.6.3/cpp/lib64 -o libtdd_sequence_SequenceServiceI.so -Wl,-h,libtdd_sequence_SequenceServiceI.so   generated/tdd_sequence.o Classes/tdd_sequence_SequenceServiceI.o -lIceBox  -lIce -lIceUtil
# **********************************************************************
MYSQL_HOME=/usr/local/mysqlconnector
#MYSQLODBC_HOME=/usr/local/mysqlodbc
UNIXODBC_HOME=/usr/local/unixODBC
THIRDPARTY_HOME=/usr/local/thirdparty
BENCHMARK_HOME=/usr/local/benchmark
ICE_HOME=$(HOME)/Ice-3.6.3
#ICE_HOME=/usr/local/ice-3.6.3
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

SHAREDLIBS = -lIceBox $(BZIP2_RPATH_LINK) -lIce -lIceUtil -lFreeze -lIceGrid -lIceBox -L$(GLOG_HOME)/lib -lglog -L$(ZK_HOME)/lib -lzookeeper_mt -luuid -L$(MYSQL_HOME)/lib -lmysqlcppconn

include ice.mak

INCLUDES=-Iobjs -Igenerated -IClasses -I$(ICE_HOME)/include -I$(MYSQL_HOME)/include -I$(GLOG_HOME)/include -I$(ZK_HOME)/include
#INCLUDES=-I$(ICE_HOME)/include -I$(MYSQLCON)/include -I$(PUBLICFUNC)/include -I$(LIBREDIS) -I$(REDISCLIENT)/ -I$(DATACACHEPRO)/include -I$(CURL)/include/curl -I$(BOOST) -I$(GLOG_HOME)/include -I$(COMMROOT)

GEN_OBJS	= $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(GEN_SRC))
#STATICLIBS += -L$(LOG4C_HOME)/lib/liblog4c.a

####### Build rules
$(TARGET): $(OBJECTS) $(CORE_MAKEFILE_LIST)
	@mkdir -p $(@D)
	g++ -m64 -O0 -fPIC  $(LDFLAGS) -o lib$(TARGET).so $(OBJECTS) $(STATICLIBS) $(SHAREDLIBS) $(LIBS)

client: $(GEN_OBJS)
	rm -rf client
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(GEN_OBJS) -L$(ICE_HOME)/lib64 $(SHAREDLIBS) -lGlacier2 -ldb_cxx-5.3 tests/Client.cpp Classes/IceExtClientUtil.cpp -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o client

t: 
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):$(ZK_HOME)/lib ./client 50000 --benchmark_counters_tabular=true

gdb: 
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(UNIXODBC_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH) gdb ./dbtest

test:$(GEN_OBJS) 
	#g++ -m64 -O0 -fPIC  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(STATICLIBS) $(SHAREDLIBS) $(LIBS) tests/create.cpp
	g++  $(CXXFLAGS) $(INCLUDES) $(DEFINES) $(SHAREDLIBS) $(GEN_OBJS) -L$(ICE_HOME)/lib64 -lGlacier2 -lIce -lIceUtil -lFreeze -ldb_cxx-5.3 tests/Client.cpp Classes/IceExt_IceClientUtil.cpp Classes/dbutil.cpp Classes/DefaultSequenceDao.cpp -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib  -lpthread -lbenchmark -o client
	#LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(ZDB_HOME)/lib:$LD_LIBRARY_PATH ./client 10 --benchmark_counters_tabular=true

dao: $(GEN_OBJS)
	g++ -finstrument-functions -Ltests/pvtrace -linstrument \
	-finstrument-functions-exclude-file-list=$(ICE_HOME)/include,$(MYSQL_HOME)/include,$(GLOG_HOME)/include,$(ZK_HOME)/include,/usr/local/gcc-4.8.5,/usr/include \
	$(CXXFLAGS) $(INCLUDES) $(DEFINES) tests/test_DefaultSequenceDao.cpp \
	Classes/MySQLDBPool.cpp Classes/DefaultSequenceDao.cpp $(GEN_OBJS) -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib -L$(GLOG_HOME)/lib -L$(MYSQL_HOME)/lib -L$(ICE_HOME)/lib64 -lIce -lIceUtil -lIceBox -lmysqlcppconn -lglog -lpthread -lbenchmark -o testdao
	LD_LIBRARY_PATH=.:$(ICE_HOME)/lib64:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH):tests/pvtrace ./testdao 1000 --benchmark_counters_tabular=true
##@echo "$(LOG_LINK)$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(SHAREDLIBS) $(STATICLIBS) $(LIBS)"
##$(LOG_LINK)$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(SHAREDLIBS) $(STATICLIBS) $(LIBS)

dbpool:
	g++ $(CXXFLAGS) $(INCLUDES) $(DEFINES) tests/test_MySQLDBPool.cpp \
	Classes/MySQLDBPool.cpp -I $(BENCHMARK_HOME)/include -L $(BENCHMARK_HOME)/lib -L$(GLOG_HOME)/lib -L$(MYSQL_HOME)/lib -lmysqlcppconn -lglog -lpthread -lbenchmark -o testpool
	LD_LIBRARY_PATH=.:$(BENCHMARK_HOME)/lib:$(MYSQL_HOME)/lib:$(GLOG_HOME)/lib:$(LD_LIBRARY_PATH) ./testpool 10 --benchmark_counters_tabular=true

otl:
	g++ -g -IClasses tests/otlmain.cpp \
	-I$(UNIXODBC_HOME)/include -L$(UNIXODBC_HOME)/lib -lmyodbc5w -lodbc -lodbcinst -o otltest
	LD_LIBRARY_PATH=.:$(UNIXODBC_HOME)/lib:$(LD_LIBRARY_PATH) ./otltest

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

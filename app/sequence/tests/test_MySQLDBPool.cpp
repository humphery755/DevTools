// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//
// **********************************************************************
#include <iostream>
#include <string>
using namespace std;

#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include <glog/logging.h>
#include <benchmark/benchmark.h>

#include "MySQLDBPool.h"

using namespace std;

#if defined(__GNUC__)
#define BENCHMARK_NOINLINE __attribute__((noinline))
#else
#define BENCHMARK_NOINLINE
#endif

static uint32_t err_total;
typedef struct  {
    int64_t service_order_id;
    int64_t organ_id;
}ServiceOrderT;

#define MAX_ROWS 10000
static int query1(benchmark::State& state,ServiceOrderT *serviceOrderList,int &count){
  string sql1="select service_order_id,organ_id from service_order order by service_order_id desc limit ";
  char buffer [33];
  sprintf(buffer,"%d",MAX_ROWS);
  sql1.append(buffer);

  multidb::Connection *con = multidb::MySQLDBPool::GetMySQLPool()->GetConnection(0);

	if(con==NULL){
    LOG(ERROR) << "Failed to create connection for: ";
    __sync_fetch_and_add(&err_total,1);
    state.SkipWithError("Failed to create connection for");
    state.counters["Error"] = err_total;
    return 1;
  }
  
  multidb::ProxStream pstrm;
  pstrm.m_con=con;
  con->setAutoCommit(false);
  try {    
			pstrm.m_pstmt = con->prepareStatement(sql1); 
			pstrm.m_res =pstrm.m_pstmt->executeQuery();
    //PreparedStatement_setString(ps, 1, name.c_str());
      if(!pstrm.m_res)
			{
				LOG(ERROR) << "prepareStatement ["<<sql1<<"] failed errorcode["<<con->getWarnings()->getErrorCode()<<"] ";
			}
      con->commit();
    while(pstrm.m_res->next()){
      serviceOrderList[count].service_order_id=pstrm.m_res->getInt64(1);
      serviceOrderList[count++].organ_id=pstrm.m_res->getInt64(2);
    }
    return 0;

  } catch(sql::SQLException& ex)
  {
    __sync_fetch_and_add(&err_total,1);
    stringstream message;
    message << __FUNCTION__ << " " << ex.getErrorCode()<< " " << ex.what();
    state.SkipWithError(message.str().c_str());
  }catch(...)
  {
    __sync_fetch_and_add(&err_total,1);
    state.SkipWithError("query2 catch unknow exception");
  }
  state.counters["Error"] = err_total;
  return 1;
}

//static char *sql2=(char*)"select service_order_id,complete_overtime,template_id,busi_type_id,template_inst_id,category_id,transport_type,noteName,user_id,transport_user_id,house_id,tache_id,contact_name,contact_tel,organ_id,urgency_leve ,status_cd,houseinfo,organname,def_class,def_class2,remark,create_date,date_format(update_date,'%Y%m%d%H%i%S'),tempname,area_id ,evalue ,evalue_msg,valovertime,secondname ,result_code,voiceid,is_pay_service,charge,pay_flag,charge_way,evlpass from service_order where service_order_id= ? and organ_id =?";
static void query2(benchmark::State& state,ServiceOrderT *serviceOrder){
  
  static string sql2="select service_order_id,complete_overtime,template_id,busi_type_id,template_inst_id,category_id,transport_type,noteName,user_id,transport_user_id,house_id,tache_id,contact_name,contact_tel,organ_id,urgency_leve ,status_cd,houseinfo,organname,def_class,def_class2,remark,create_date,update_date,tempname,area_id ,evalue ,evalue_msg,valovertime,secondname ,result_code,voiceid,is_pay_service,charge,pay_flag,charge_way,evlpass,deal_user_id ,order_code,bespeak_begin_time,bespeak_end_time,supflag,suptype,supervise_one_time,supervise_two_time,supervise_thr_time,supervise_user_ids,hang_consume_time,hang_status,handle_describe,is_reviewed,re_handle,re_evalue ,re_evalue_msg,re_is_reviewed, re_evlpass,detail_id,overtime_flag,begin_date,busi_alias_name,interaction_status,deal_user_name ,tache_supflag,tache_suptype,tache_supervise_user_ids from service_order where service_order_id= ? and organ_id =?";
  multidb::Connection *con = multidb::MySQLDBPool::GetMySQLPool()->GetConnection(0);
	if(con==NULL){
    LOG(ERROR) << "Failed to create connection for: ";
    return;
  }
  int j=1;
  try
    {
      multidb::ProxStream pstrm;
      pstrm.m_pstmt = con->prepareStatement(sql2); 
      pstrm.m_pstmt->setInt64(1,serviceOrder->service_order_id);
      pstrm.m_pstmt->setInt64(2,serviceOrder->organ_id);
			pstrm.m_res =pstrm.m_pstmt->executeQuery();
      if(!pstrm.m_res)
			{
				LOG(ERROR) << "prepareStatement ["<<sql2<<"] failed errorcode["<<con->getWarnings()->getErrorCode()<<"] ";
			}
      //int64_t lVal;
      //int32_t iVal;
      //unsigned char  sVal[500];
      //double  dVal;
      while(pstrm.m_res->next()){
        j=1;
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt64(j++);//10
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);//20
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);//30
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getDouble(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt64(j++);;
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);//40
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);//50
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getInt64(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);//60
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getInt(j++);
        pstrm.m_res->getString(j++);
        pstrm.m_res->getString(j++);
      }

    } catch(sql::SQLException& ex)
    {
      //LOG(ERROR)  << "exception error[" << ex.getErrorCode() << "] errmsg["<< ex.what()<<"] index:"<<j <<", service_order_id:" << serviceOrder->service_order_id << ", organ_id:" << serviceOrder->organ_id;
      __sync_fetch_and_add(&err_total,1);
      stringstream message;
      message << __FUNCTION__ << " " << ex.getErrorCode()<< " " << ex.what();
      state.SkipWithError(message.str().c_str());
    }catch(...)
    {
      __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("query2 catch unknow exception");
    }
    multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con);
    state.counters["Error"] = err_total;
}
static void BM_SQL_test1(benchmark::State& state) {
  //list<ServiceOrderT> serviceOrderList;
  ServiceOrderT serviceOrderList[MAX_ROWS];
  if (state.thread_index == 0) {
    // Setup code here.
   // cerr << "startup" <<endl;    
  }
  
  int count=0;
  
  if(query1(state,serviceOrderList,count)){
    return;
  }
  while (state.KeepRunning()) {
    for (int i = 0; i < count + 1; ++i) {
      query2(state,&serviceOrderList[i]);
    }
  }
	
	//关闭连接（活动连接－1）
 
  
  state.SetBytesProcessed(int64_t(state.iterations()) *
                          int64_t(state.range(0)));
  //delete[] src;
  
  if (state.thread_index == 0) {
    // Teardown code here.
    //cerr << "Teardown" <<endl;
    
  }
}

static void BM_SQL_test(benchmark::State& state) {
  if (state.thread_index == 0) {
    //cerr << "Tearup" <<endl;
  }
  while (state.KeepRunning()) {
    multidb::Connection *con = multidb::MySQLDBPool::GetMySQLPool()->GetConnection(0);
    if(con==NULL){
      __sync_fetch_and_add(&err_total,1);
      state.SkipWithError("Failed to create connection");
      state.counters["Error"] = err_total;
      continue;
    }
    multidb::MySQLDBPool::GetMySQLPool()->FreeConnection(con);
  }

  
  state.SetBytesProcessed(int64_t(state.iterations()) *
                          int64_t(state.range(0)));
  if (state.thread_index == 0) {
    // Teardown code here.
    //cerr << "Teardown" <<endl;
  }
}

//BENCHMARK(BM_MultiThreaded)->Threads(2);

//BENCHMARK(BM_DenseThreadRanges)->Arg(8)->DenseThreadRange(1, 2);
//BENCHMARK(BM_DenseThreadRanges)->Arg(8)->ThreadRange(1, 9)->Unit(benchmark::kMillisecond);
#define BENCHMARK_XXX(t,arg) \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);  \
    BENCHMARK(BM_SQL_test)->Arg(arg)->Threads(t);

int threads=1;


int main(int argc, char** argv) {
  if(argc>1){
    char *strTh=argv[1];
    threads=atoi(strTh);
  }
  err_total=0;

  google::InitGoogleLogging("dbtest");
  google::SetStderrLogging(google::GLOG_FATAL);//设置级别高于 google::FATAL 的日志同时输出到屏幕
  string logdir = "./logs/";
  ostringstream log_dest_stream;
	log_dest_stream<<logdir<<"info";
	google::SetLogDestination(google::GLOG_INFO,log_dest_stream.str().c_str());
	log_dest_stream.str("");
	log_dest_stream<<logdir<<"warn";
	google::SetLogDestination(google::GLOG_WARNING, log_dest_stream.str().c_str()); 
	log_dest_stream.str("");
	log_dest_stream<<logdir<<"error";
	google::SetLogDestination(google::GLOG_ERROR,log_dest_stream.str().c_str());
	log_dest_stream.str("");
	log_dest_stream<<logdir<<"fatal";
	google::SetLogDestination(google::GLOG_FATAL, log_dest_stream.str().c_str());
	FLAGS_stop_logging_if_full_disk = true; //当磁盘被写满时，停止日志输出
	FLAGS_colorlogtostderr = true; //设置输出到屏幕的日志显示相应颜色
  FLAGS_servitysinglelog = true;
  FLAGS_logbufsecs = 0; //缓冲日志输出，默认为30秒，此处改为立即输出
  FLAGS_max_log_size = 100; //最大日志大小为 100MB

  string strurl = "Driver=MYSQL UTF8;Server=192.168.1.12;Database=serorder;UID=admin;PWD=admin";
  for(int i=0;i<10;i++)
    if(!multidb::MySQLDBPool::GetMySQLPool()->RegistDataBase(i,"admin/admin@192.168.1.12:3306/serorder",32,threads<<9))return 1;

  if(!multidb::MySQLDBPool::GetMySQLPool()->Startup())return 1;

if(1==0)BENCHMARK(BM_SQL_test1)->Arg(1)->Threads(1);
  /*

    */
  
 



  BENCHMARK_XXX(threads<<9,1<<9)
  BENCHMARK_XXX(threads<<8,1<<8)
  BENCHMARK_XXX(threads<<7,1<<7)
  BENCHMARK_XXX(threads<<6,1<<3)
  BENCHMARK_XXX(threads<<5,1<<3)
  BENCHMARK_XXX(threads<<4,1<<3)
  BENCHMARK_XXX(threads<<3,1<<3)


  ::benchmark::Initialize(&argc, argv);
  
  while(1)::benchmark::RunSpecifiedBenchmarks();
  multidb::MySQLDBPool::GetMySQLPool()->ReleaseAll();
	google::ShutdownGoogleLogging();
}


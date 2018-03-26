// **********************************************************************
//
// Copyright (c) 2003-2016 ZeroC, Inc. All rights reserved.
//   slice2freeze --dict StringLongMap,string,long StringLongMap
// slice2freeze --dict StringSeqRangeMap,string,tddl::sequences::SeqRange_t StringSeqRangeMap StringSeqRangeMap.ice
// **********************************************************************
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include <cstring>
#include <list>
#include <pthread.h>
//#include <mutex>
#include <iostream>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
#include <algorithm>
#include <sys/epoll.h>
#include <glog/logging.h>
#include <Ice/Ice.h>
#include <SequenceServiceI.h>
#include "IceExtClientUtil.h"
#include "zookeeper_helper.h"
#include "OrderSequenceAdapter.h"

using namespace std;
using namespace tddl::sequences;
using namespace IceExt;

static struct ZookeeperHelper *zk_helper;
static int if_exit;
static bool isregister;
static pthread_t test_id;
pthread_mutex_t lock;
static string ADAPTER_NAME="tddl.sequences.OrderSequenceService";
static char zk_election_watch_node[256];
static char zk_election_watch_child_node[256];
static void register_to_registry();
static void unregister_4_registry();

static bool sort_cmp(char *p,char *q){
    return strcmp(p,q)<0;
}
static int  node_indexof(char *myid,String_vector *strings){
    if(strings->count==0)return -1;
    sort( (char**)strings->data, (char**)(strings->data+strings->count), sort_cmp);
    for (int i = 0;  i < strings->count; i++) {
        if (strcmp(myid, strings->data[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static void child_event(struct ZkEvent *zk_event, struct ZookeeperHelper *zk_helper, const char *path)
{
    LOG(INFO) <<"catch childevent, path: "<< path;
    struct String_vector node_vector;
    zoo_helper_get_children(zk_helper, zk_election_watch_node, &node_vector);
    int i = node_indexof(zk_helper->election_node,&node_vector);

    if (i == 0) {
        register_to_registry();
    } else if(i == -1){
        LOG(INFO) <<"This is ["<<zk_helper->election_node<<"], my index: "<<i;
        if(isregister){
            char buf[256];
            add_tmp_node(zk_helper, zk_election_watch_child_node, zk_helper->election_uuid,ZOO_EPHEMERAL|ZOO_SEQUENCE,buf, sizeof(buf)-1);
            return;
        }
        unregister_4_registry();
    }else {
        LOG(INFO) <<"This is ["<<zk_helper->election_node<<"], i am a follower -> "<<i;
        unregister_4_registry();
    }

    deallocate_String_vector(&node_vector);
}

static void connected_event(struct ZkEvent *zk_event, struct ZookeeperHelper *zk_helper, const char *path)
{
    LOG(INFO) <<"catch connectedevent: "<<path;
    
    zk_event->child_event(zk_event, zk_helper, path);
}


static void exiter(int sig)
{
    if_exit = 1;
    if(pthread_self() != test_id) {
        LOG(INFO) <<"exiter";
        return;
    }
    LOG(INFO) <<"exiter_for_test";
    
}

static void test_event(struct ZkEvent *zk_event, struct ZookeeperHelper *zk_helper, const char *path)
{
    struct String_vector node_vector;
    if(zoo_helper_get_children(zk_helper, zk_election_watch_node, &node_vector)!=0)return;

    int i = node_indexof(zk_helper->election_node,&node_vector);
    if (i == 0) {
        register_to_registry();
    }else if(i == -1){
        unregister_4_registry();
        char buf[256];
        add_tmp_node(zk_helper, zk_election_watch_child_node, zk_helper->election_uuid,ZOO_EPHEMERAL|ZOO_SEQUENCE,buf, sizeof(buf)-1);
    } else {
        unregister_4_registry();
    }
    deallocate_String_vector(&node_vector);
}

static ::Ice::ObjectAdapterPtr adapter;
static Ice::CommunicatorPtr communicator;
static Ice::PropertiesPtr prop;

static void * start_leader_election(void *arg)
{
    struct ZkEvent zk_event;
    memset(&zk_event, 0, sizeof(struct ZkEvent));
    zk_event.child_event = child_event;
    zk_event.connected_event = connected_event;

    string strHosts = prop->getPropertyWithDefault("seq.zookeeper.hosts","");
    if(strHosts.length()<5){
        LOG(ERROR) << "seq.zookeeper.hosts is null ";
        return NULL;
    }
    string watch_node = prop->getPropertyWithDefault("seq.zookeeper.watch_node","/tddl/sequences/OrderSequenceService");
    strcpy(zk_election_watch_node,watch_node.c_str());
    watch_node.append("/1");
    strcpy(zk_election_watch_child_node,(char*)watch_node.c_str());

    zk_helper = create_zookeeper_helper();
    do{
        if(register_to_zookeeper(zk_helper, strHosts.c_str(), 30000) < 0) {
            continue;
        }
        char buf[256];
        if(add_tmp_node(zk_helper, zk_election_watch_child_node, zk_helper->election_uuid,ZOO_EPHEMERAL|ZOO_SEQUENCE,buf, sizeof(buf)-1)!=0){
            continue;
        }

        if(add_zookeeper_event(zk_helper, CHILD_EVENT|CREATED_EVENT|DELETED_EVENT , zk_election_watch_node, &zk_event)!=0){
            continue;
        }
        break;
    }while(1);
    int fd = epoll_create(1);
    while(!if_exit)
    {
        test_event(NULL, zk_helper, NULL);
        epoll_wait(fd, NULL, 1,10000*600);
    }

    return NULL;
}

static void register_to_registry(){
    pthread_mutex_lock(&lock);
    if(isregister){
        pthread_mutex_unlock(&lock);
        return;
    }
    LOG(INFO) <<"This is ["<<zk_helper->election_node<<"], i am a leader\n"; 
    adapter->activate();
    LOG(INFO) << "The Adapter:" << adapter->getName() << " be activated.";    
    isregister=true;
    pthread_mutex_unlock(&lock);
}
static void unregister_4_registry(){
    pthread_mutex_lock(&lock);
    if(isregister){
        LOG(INFO) << "The Adapter: " << adapter->getName() << " be restart." ;
        adapter->deactivate();
        isregister=false;
        exit(0);
    }
    pthread_mutex_unlock(&lock);
}

void startOrderSequence(const Ice::CommunicatorPtr& _communicator, Ice::PropertiesPtr& _prop){
    string strHosts = _prop->getPropertyWithDefault("seq.zookeeper.hosts","");
    if(strHosts.length()<5){
        LOG(ERROR) << ADAPTER_NAME <<" has not been registered: " << "seq.zookeeper.hosts is null";
        return;
    }

    signal(SIGINT, exiter);
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        LOG(ERROR) << "pthread_mutex_init lock error: "<<strerror(errno);
        exit(1);
    }

    communicator=_communicator;
    prop=_prop;   

    ostringstream name_stream;
    name_stream << ADAPTER_NAME << version;
    LOG(INFO) << "start createAdapter: " << name_stream.str();
    adapter = communicator->createObjectAdapter(name_stream.str());  
    int workerId = prop->getPropertyAsInt("seq.workerId");
	int datacenterId = prop->getPropertyAsInt("seq.datacenterId");
    tddl::sequences::SequenceServicePtr orderSeqSvc = new SequenceServiceI(workerId,datacenterId);    
    adapter->add(orderSeqSvc, communicator->stringToIdentity(adapter->getName()));

    int err = pthread_create(&test_id, NULL, start_leader_election, NULL); 
    if ( 0 != err ) 
    { 
        LOG(ERROR) <<"can't create thread:" << strerror(err); 
    }
}
void stopOrderSequence(){
    LOG(INFO) << "The Adapter: " << adapter->getName() << " be destroy." ;
    adapter->destroy();
    destory_zookeeper_helper(zk_helper);
    zk_helper = NULL;
    pthread_kill(test_id,SIGINT);
    unregister_4_registry();
    //pthread_join(test_id,NULL);
}

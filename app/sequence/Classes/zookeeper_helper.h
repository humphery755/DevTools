#ifndef ZOOKEEPER_HELPER_H
#define ZOOKEEPER_HELPER_H

#include <zookeeper/zookeeper.h>
#include <pthread.h>
#include "queue.h"

#define ZOOKEEPER_HELPER_HOST_MAX_LEN 1024

struct ZkEvent;
struct ZookeeperHelper;

typedef void (*ZkEventFunc)(struct ZkEvent *zk_event, struct ZookeeperHelper *zk_helper, const char *path);

struct ZkEvent
{
    int eventmask;
    ZkEventFunc connected_event;
    ZkEventFunc changed_event;
    ZkEventFunc child_event;
    ZkEventFunc created_event;
    ZkEventFunc deleted_event;
    ZkEventFunc not_watching_event;
};

extern const int CREATED_EVENT;
extern const int DELETED_EVENT;
extern const int CHANGED_EVENT;
extern const int CHILD_EVENT;

struct ZkHelperPair;

struct ZkHelperPair
{
    char *key;
    void *value;
    int flag;
    int value_len;
    SLIST_ENTRY(ZkHelperPair) next;
};

enum E_MODE
{
    E_CONNECTION_M,
    E_REGISTER_M,
    E_DESTORY_M
};

struct ZkHelperPairList {
    struct ZkHelperPair *slh_first;
};

struct ZookeeperHelper
{
    char host[ZOOKEEPER_HELPER_HOST_MAX_LEN];
    int recv_timeout;
    zhandle_t *zhandle;
    char local_addr[32];
    short local_port;
    struct ZkHelperPairList zoo_path_list;
    struct ZkHelperPairList zoo_event_list;
	pthread_mutex_t lock;                   /* for zoo_path_list && zoo_event_list && mode */
    int mode;
    int reconnection_flag;
    char election_node[50];
    char election_uuid[37];
};

struct ZookeeperHelper * create_zookeeper_helper();
int destory_zookeeper_helper(struct ZookeeperHelper *zk_helper);

/*register*/
int register_to_zookeeper(struct ZookeeperHelper *zk_helper, \
        const char* host, int recv_timeout);
/*node*/
int add_tmp_node(struct ZookeeperHelper *zk_helper, const char *path, const char *value,int flag,char *path_buffer, int path_buffer_len);
int add_persistent_node(struct ZookeeperHelper *zk_helper, const char *path, const char *value);
/*event*/
int add_zookeeper_event(struct ZookeeperHelper *zk_helper, \
        int event, const char *path, struct ZkEvent *handle);
int remove_zookeeper_event(struct ZookeeperHelper *zk_helper, const char *path);

/*normal API*/
int zoo_helper_get_children(struct ZookeeperHelper *zk_helper, \
        const char* path, struct String_vector *node_vector);
int zoo_helper_get(struct ZookeeperHelper *zk_helper, \
        const char* path, char *buf, int *buf_len);
int zoo_helper_exists(struct ZookeeperHelper *zk_helper, \
        const char *path);
int zoo_helper_set(struct ZookeeperHelper *zk_helper, \
        const char* path, const char* buffer, int buflen, int version);
int zoo_helper_delete(struct ZookeeperHelper *zk_helper, \
        const char* path, int version);
int zoo_helper_create(struct ZookeeperHelper *zk_helper, \
        const char* path, const char* value, int value_len, int flags);
int zoo_helper_election_test(struct ZookeeperHelper *zk_helper, const char *path);
        
#endif


#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <algorithm>
#include <uuid/uuid.h>
#include <glog/logging.h>
#include "zookeeper_helper.h"


static int create_node(struct ZookeeperHelper *zk_helper, char *path, const char *value, const int flag,char *path_buffer, int path_buffer_len);
static int recursive_create_node(struct ZookeeperHelper *zk_helper, const char *path,const char *value, const int flag,char *path_buffer, int path_buffer_len);
static void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx);
static void zoo_sleep(unsigned int nmsecs);
static int get_local_addr(struct ZookeeperHelper *zk_helper);
static void re_set_event(struct ZookeeperHelper *zk_helper);
static void re_connect(struct ZookeeperHelper *zk_helper);
static void handle_event(struct ZkEvent *zk_event, struct ZookeeperHelper *zk_helper, int type, const char* path);

const int CREATED_EVENT = 1 << 1;
const int DELETED_EVENT = 1 << 2;
const int CHANGED_EVENT = 1 << 3;
const int CHILD_EVENT   = 1 << 4;

static char chars[] = { 
	'a','b','c','d','e','f','g','h',  
	'i','j','k','l','m','n','o','p',  
	'q','r','s','t','u','v','w','x',  
	'y','z','0','1','2','3','4','5',  
	'6','7','8','9','A','B','C','D',  
	'E','F','G','H','I','J','K','L',  
	'M','N','O','P','Q','R','S','T',  
	'U','V','W','X','Y','Z' 
}; 

void uuid(char *result, int len)
{
	unsigned char uuid[16];
	char output[24];
	const char *p = (const char*)uuid;

	uuid_generate(uuid);
	memset(output, 0, sizeof(output));

	int i, j;
	for (j = 0; j < 2; j++)
	{
		unsigned long long v = *(unsigned long long*)(p + j*8);
		int begin = j * 10;
		int end =  begin + 10;

		for (i = begin; i < end; i++)
		{
			int idx = 0X3D & v;
			output[i] = chars[idx];
			v = v >> 6;
		}
	}
	//printf("%s\n", output);
	len = (len > (int)sizeof(output)) ? (int)sizeof(output) :  len;
	memcpy(result, output, len);
}

static const char* state2Str(int state)
{
    if (state == 0) 
        return "CLOSED_STATE";
    if (state == ZOO_CONNECTING_STATE)
        return "CONNECTING_STATE";
    if (state == ZOO_ASSOCIATING_STATE)
        return "ASSOCIATING_STATE";
    if (state == ZOO_CONNECTED_STATE)
        return "CONNECTED_STATE";
    if (state == ZOO_EXPIRED_SESSION_STATE)
        return "EXPIRED_SESSION_STATE";
    if (state == ZOO_AUTH_FAILED_STATE)
        return "AUTH_FAILED_STATE";

    return "INVALID_STATE";
}

static const char* type2Str(int type)
{
    if(type == ZOO_CREATED_EVENT)
        return "CREATED_EVENT";
    if(type == ZOO_DELETED_EVENT)
        return "DELETED_EVENT";
    if(type == ZOO_CHANGED_EVENT)
        return "CHANGED_EVENT";
    if(type == ZOO_CHILD_EVENT)
        return "CHILD_EVENT";
    if(type == ZOO_SESSION_EVENT)
        return "SESSION_EVENT";
    if(type == ZOO_NOTWATCHING_EVENT)
        return "NOTWATCHING_EVENT";
    return "INVALID_EVENT";
}

inline static void mem_copy_value(void **dst, int *dst_len, \
        const void *src, const int src_len)
{
    if(src_len > *dst_len){
        *dst_len = src_len;
        *dst = realloc(*dst, src_len);
    }
    memcpy(*dst, src, src_len);
    return ;
}

inline static void mem_new_value(void **dst, int *dst_len, \
        const void *src, const int src_len)
{
    if(dst_len != NULL)
        *dst_len = src_len;
    *dst = malloc(src_len);
    memcpy(*dst, src, src_len);
    return ;
}

inline static void clean_node(ZookeeperHelper *zk_helper,const char *path,String_vector *strings){
    int rc;
    int buf_len;
    int str_uuid_len=20;
    char buf[512];
    char p[50];
    for (int i = 0;  i < strings->count; i++) {
        if(strcmp(zk_helper->election_node, strings->data[i]) == 0)continue;
        sprintf(p,"%s/%s",path,strings->data[i]);
        memset(buf,'\0',512);
        rc=zoo_helper_get(zk_helper, p, buf, &buf_len);
        if (rc != 0 ) {
            continue;
        }
        if(buf_len==str_uuid_len && memcmp(zk_helper->election_uuid, buf,buf_len) == 0){
            LOG(INFO) <<"delete node: "<< p;
            zoo_helper_delete(zk_helper,p,0);
        }            
    }
}

inline static void fetch_node(const char *buf,int len, char *node)
{
    const char *p = buf;
    int i;
    for(i=len-1;i>=0;i--){
        if (*(p+i)=='/') {
            break;
        }
    }
    strcpy(node, p + i + 1);
    return;
}

struct ZookeeperHelper * create_zookeeper_helper()
{
    struct ZookeeperHelper *zk_helper = ( struct ZookeeperHelper *)malloc(sizeof(struct ZookeeperHelper));
    memset(zk_helper,0,sizeof(struct ZookeeperHelper));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&zk_helper->lock, &attr) != 0)
    {
        LOG(ERROR) << "pthread_mutex_init zoo_path_list error: "<<strerror(errno);
        return NULL;
    }
    uuid(zk_helper->election_uuid, 20);
    zk_helper->election_uuid[20] = '\0';
    zk_helper->election_node[0] = '\0';
    //zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
    return zk_helper; 
}

int destory_zookeeper_helper(struct ZookeeperHelper *zk_helper)
{
    if(zk_helper == NULL)
        return -1;
    
    pthread_mutex_lock(&zk_helper->lock);
    zk_helper->mode = E_DESTORY_M;
    struct ZkHelperPair *p;
    while(!SLIST_EMPTY(&zk_helper->zoo_event_list)) {
        p = SLIST_FIRST(&zk_helper->zoo_event_list);
        SLIST_REMOVE_HEAD(&zk_helper->zoo_event_list, next);
        free(p->key);
        p->key = NULL;
        free(p->value);
        p->value = NULL;
        free(p);
        p = NULL;
    }
    while(!SLIST_EMPTY(&zk_helper->zoo_path_list)) {
        p = SLIST_FIRST(&zk_helper->zoo_path_list);
        SLIST_REMOVE_HEAD(&zk_helper->zoo_path_list, next);
        free(p->key);
        p->key = NULL;
        free(p->value);
        p->value = NULL;
        free(p);
        p = NULL;
    }
    if(zk_helper->zhandle != NULL)
        zookeeper_close(zk_helper->zhandle);
    pthread_mutex_unlock(&zk_helper->lock);

    if (pthread_mutex_destroy(&zk_helper->lock) != 0) 
    LOG(ERROR) <<"pthread_mutex_destroy zoo_path_list error: "<<strerror(errno);
    free(zk_helper);
    return 0;
}

int register_to_zookeeper(struct ZookeeperHelper *zk_helper, const char* host, int recv_timeout)
{
    strncpy(zk_helper->host,host,ZOOKEEPER_HELPER_HOST_MAX_LEN);
    zk_helper->recv_timeout = recv_timeout;
    zk_helper->mode = E_CONNECTION_M;
    
    zk_helper->zhandle = zookeeper_init(zk_helper->host, watcher, recv_timeout, NULL, zk_helper, 0);
    if(zk_helper->zhandle == NULL){
        LOG(ERROR) <<"zookeeper_init error: "<< strerror(errno);
        return -1;
    }

    int timeout = 0;
    while(1)
    {
        if (zoo_state(zk_helper->zhandle) == ZOO_CONNECTED_STATE) {
            break;
        }
        if(timeout >= zk_helper->recv_timeout){
            //zookeeper_close(zk_helper->zhandle);
            LOG(ERROR) <<"connect zookeeper Timeout";
            return -1;
        }
        zoo_sleep(1);
        timeout++;
    }

    if(-1 == get_local_addr(zk_helper)){
        //zookeeper_close(zk_helper->zhandle);
        return -1;
    }

    return 0;
}

int add_tmp_node(struct ZookeeperHelper *zk_helper, const char *path, const char *value,const int flag,char *path_buffer, int path_buffer_len)
{
    int ret;
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M) {
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    zk_helper->mode = E_REGISTER_M;
    ret = recursive_create_node(zk_helper, path, value, flag,path_buffer,path_buffer_len);
    pthread_mutex_unlock(&zk_helper->lock);
    return ret;
}

int add_persistent_node(struct ZookeeperHelper *zk_helper, const char *path, const char *value)
{
    int ret;
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M) {
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    zk_helper->mode = E_REGISTER_M;
    ret = recursive_create_node(zk_helper, path, value, 0,NULL,0);
    pthread_mutex_unlock(&zk_helper->lock);
    return ret;
}

static int recursive_create_node(struct ZookeeperHelper *zk_helper, const char *path,const char *value, const int flag,char *path_buffer, int path_buffer_len)
{
    if(!path || *path=='\0' || path[0] != '/')
    {
        LOG(ERROR) << "Invalid Argument: "<< path;
        return -1;
    }
    char strPath[256];
    snprintf(strPath, sizeof(strPath), "%s", path);

    char *substr_pos = 0;
    substr_pos = strPath;
    while(1)
    {
        // "/abc/def/gl/" if *substr_pos = '/' and *(substr_pos + 1) = 0
        // need to break because substr_pos touch the end
        if(*(substr_pos + 1) == '\0')
            break;
        substr_pos = strchr(substr_pos + 1, '/');
        // not find, break
        if(substr_pos == NULL)
            break;
        *substr_pos = '\0';
        int res = zoo_exists(zk_helper->zhandle, strPath, 0, NULL);
        if(res != ZOK) {
            if(res == ZNONODE){
                res = zoo_create(zk_helper->zhandle, strPath, " ", 1, \
                        &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
                if(res != ZOK) {
                    LOG(ERROR) << "create node "<< strPath <<" error: "<<zerror(res) <<"(" << res<<")";
                }
            }
            else {
                LOG(ERROR) <<"check node "<< strPath <<"  error: "<<zerror(res) <<"(" << res<<")";
            }
        }
        *substr_pos = '/';
    }
    return create_node(zk_helper, strPath, value, flag, path_buffer, path_buffer_len);
}

inline static bool sort_cmp(char *p,char *q){
    return strcmp(p,q)<0;
}
static int create_node(struct ZookeeperHelper *zk_helper, \
        char *path, const char *value, const int flag, char *path_buffer, int path_buffer_len)
{
    int res = zoo_exists(zk_helper->zhandle, path, 0, NULL);
    if(res == ZOK)  //节点存在
    {
        res = zoo_delete(zk_helper->zhandle, path, -1);
        if(res != ZOK)
        {
            LOG(ERROR) <<"Delete path "<<path<<" error: " << zerror(res);
            return -1;
        }
        LOG(INFO) <<"Delete path "<<path<<" success, Create it...";
        res = zoo_create(zk_helper->zhandle, path, value, strlen(value), \
                &ZOO_OPEN_ACL_UNSAFE, flag, path_buffer, path_buffer_len);

    }
    else if(res == ZNONODE)  //节点不存在
    {   
        if(flag & ZOO_EPHEMERAL && flag & ZOO_SEQUENCE){
            LOG(INFO) <<"create election node[EPHEMERAL|SEQUENCE] "<<path<<", old election_node:"<<zk_helper->election_node;
            int node_len=strlen(zk_helper->election_node);
            char *path_pos = 0;
            path_pos = path+strlen(path);
            while(1)
            {
                if(*(--path_pos) == '/')
                break;
            }
            path[path_pos-path]='\0';
            LOG(INFO) << "path: "<<path;
            if(node_len>0){                
                char buffer[256];
                snprintf(buffer, 256, "%s/%s", path, zk_helper->election_node);
                int res = zoo_exists(zk_helper->zhandle, buffer, 0, NULL);
                if(res == ZOK) {
                    strcpy(path_buffer,zk_helper->election_node);
                    path[path_pos-path]='/';
                    goto breaklab;
                }
            }
            {
                struct String_vector node_vector;
                zoo_helper_get_children(zk_helper, path, &node_vector);
                int rc;
                int buf_len;
                int str_uuid_len=strlen(zk_helper->election_uuid);
                char buf[512];
                char p[50];
                std::sort( (char**)node_vector.data, (char**)(node_vector.data+node_vector.count), sort_cmp);
                for (int i = 0;  i < node_vector.count; i++) {
                    sprintf(p,"%s/%s",path,node_vector.data[i]);
                    memset(buf,'\0',512);
                    rc=zoo_helper_get(zk_helper, p, buf, &buf_len);
                    if (rc != 0 ) {
                        continue;
                    }
                    if(buf_len==str_uuid_len && memcmp(zk_helper->election_uuid, buf,buf_len) == 0){
                        strncpy(zk_helper->election_node,buf,buf_len);
                        path[path_pos-path]='/';
                        goto breaklab;
                    }            
                }
            }
            path[path_pos-path]='/';
            LOG(INFO) << "path: "<<path;
            res = zoo_create(zk_helper->zhandle, path, zk_helper->election_uuid, strlen(zk_helper->election_uuid), &ZOO_OPEN_ACL_UNSAFE, flag, path_buffer, path_buffer_len);
            if(res==ZOK){
                fetch_node(path_buffer,strlen(path_buffer), zk_helper->election_node);
           }
        }else{
            res = zoo_create(zk_helper->zhandle, path, value, strlen(value), \
            &ZOO_OPEN_ACL_UNSAFE, flag, path_buffer, path_buffer_len);
        }
    }
    else
    {
        LOG(ERROR) <<"Check node exists error: "<< zerror(res)<<"(" <<res<<")";
        return -1;
    }

breaklab:
    if(res != ZOK)
    {
        LOG(ERROR) <<"create node "<<path<<" flag "<<flag<<" error: "<< zerror(res);
        return -1;
    }
    if(flag == 0){
        return 0;
    }
    
    struct ZkHelperPair *p;
    int find = 0;
    SLIST_FOREACH(p, &zk_helper->zoo_path_list, next)
    {
        //should be strncmp in the future
        if(strcmp(path, p->key) == 0) {
            find = 1;
            break;
        }
    }
    if(find == 1){
        //update_value(p, value, valuelen);
        mem_copy_value(&p->value, &p->value_len, value, strlen(value) + 1);
        p->flag = flag;
    }
    else {
        //create_value(&p, value, valuelen);
        p = (ZkHelperPair*)malloc(sizeof(struct ZkHelperPair));
        mem_new_value((void **)&p->key, NULL, path, strlen(path) + 1);
        mem_new_value(&p->value, &p->value_len, value, strlen(value) + 1);
        //printf("1%s,%s\n",p->key, p->value);

        p->flag = flag;
        SLIST_INSERT_HEAD(&zk_helper->zoo_path_list, p, next);
    }

    return 0;
}

int add_zookeeper_event(struct ZookeeperHelper *zk_helper, \
        int event, const char *path, struct ZkEvent *handle)
{
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M) {
        LOG(ERROR) <<"add_zookeeper_event failed, ZookeeperHelper in E_DESTORY_M mode";
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    int ret = 0;
    handle->eventmask |= event;

    struct ZkHelperPair *p;
    int find = 0;
    SLIST_FOREACH(p, &zk_helper->zoo_path_list, next)
    {
        //should be strncmp in the future
        if(strcmp(path, p->key) == 0) {
            find = 1;
            ((struct ZkEvent *)p->value)->eventmask = handle->eventmask;
            break;
        }
    }

    if(find == 0){
        p = (ZkHelperPair *)malloc(sizeof(struct ZkHelperPair));

        mem_new_value((void **)&p->key, NULL, path, strlen(path) + 1);
        mem_new_value(&p->value, NULL, handle, sizeof(struct ZkEvent));
        SLIST_INSERT_HEAD(&zk_helper->zoo_event_list, p, next);
    }
    pthread_mutex_unlock(&zk_helper->lock);

    if((event & CREATED_EVENT) || (event & DELETED_EVENT) || (event & CHANGED_EVENT)){
        ret = zoo_exists(zk_helper->zhandle, path, 1, NULL);
        if(ret != ZOK){
            if (ret == ZNONODE) {
                ret = add_tmp_node(zk_helper, path, "1",ZOO_EPHEMERAL,NULL,0);
            }
            if (ret != ZOK) {
                LOG(ERROR) <<"set watcher for path "<<path<<" error "<< zerror(ret);
                return -1;
            }
        }
    }
    if(event & CHILD_EVENT){
        ret = zoo_get_children(zk_helper->zhandle, path, 1, NULL);
        if(ret != ZOK){
            if (ret == ZNONODE) {
                ret = add_tmp_node(zk_helper, path, "1",ZOO_EPHEMERAL,NULL,0);
            }

            if (ret != ZOK) {
                LOG(ERROR) <<"set watcher for path "<<path<<" error %s"<<zerror(ret);
                return -1;
            }
        }
    }
    
    return 0;
}

int remove_zookeeper_event(struct ZookeeperHelper *zk_helper, const char *path)
{
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M) {
        LOG(ERROR) <<"add_zookeeper_event failed, ZookeeperHelper in E_DESTORY_M mode";
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }

    struct ZkHelperPair *p;
    SLIST_FOREACH(p, &zk_helper->zoo_path_list, next)
    {
        //should be strncmp in the future
        if(strcmp(path, p->key) == 0) {
            SLIST_REMOVE_AFTER(p, next);
            break;
        }
    }
    pthread_mutex_unlock(&zk_helper->lock);

    return 0;
}

static void re_set_event(struct ZookeeperHelper *zk_helper)
{
    int ret = 0;
    struct ZkHelperPair *p;
    int event;
    char *path;
    struct ZkEvent * zk_event;
    SLIST_FOREACH(p, &zk_helper->zoo_event_list, next)
    {
        zk_event = ((struct ZkEvent *)p->value);
        event = zk_event->eventmask;
        path = p->key;
        if((event & CREATED_EVENT) || (event & DELETED_EVENT) || (event & CHANGED_EVENT)){
            ret = zoo_exists(zk_helper->zhandle, path, 1, NULL);
            if(ret != ZOK){
                if (ret == ZNONODE) {
                    ret = add_tmp_node(zk_helper, path, "1",ZOO_EPHEMERAL,NULL,0);
                }
                if (ret != ZOK) {
                    LOG(ERROR) <<"set watcher for path "<<path<<" error " << zerror(ret);
                    continue;
                }
            }
        }
        if(event & CHILD_EVENT){
            ret = zoo_get_children(zk_helper->zhandle, path, 1, NULL);
            if(ret != ZOK){
                if (ret == ZNONODE) {
                    ret = add_tmp_node(zk_helper, path, "1",ZOO_EPHEMERAL,NULL,0);
                }
                if (ret != ZOK) {
                    LOG(ERROR) <<"set watcher for path "<<path<<" error "<<zerror(ret);
                    continue;
                }
            }
        }
        zk_event->connected_event(zk_event, zk_helper, path);
    }
}

static void re_connect(struct ZookeeperHelper *zk_helper)
{
    if(zk_helper->zhandle) {
        zookeeper_close(zk_helper->zhandle);
    }
    zk_helper->zhandle = zookeeper_init(zk_helper->host, watcher, 
            zk_helper->recv_timeout, NULL, zk_helper, 0);
    if(zk_helper->zhandle == NULL)
    {
        LOG(ERROR) <<"retry connect zookeeper error: "<<strerror(errno);
    }
    zk_helper->reconnection_flag = 1; 
}

static void handle_event(struct ZkEvent *zk_event, struct ZookeeperHelper *zk_helper, int type, const char* path)
{
    int ret;
    int eventmask = zk_event->eventmask;
    zhandle_t *zh = zk_helper->zhandle;
    LOG(INFO) <<"path "<<path<<" eventmask: " << eventmask;
    if(type == ZOO_CREATED_EVENT && eventmask & CREATED_EVENT)
    {
        //重新设置观察点
        ret = zoo_exists(zh, path, 1, NULL);
        if (ZOK != ret){
            LOG(ERROR) <<"set watcher [ZOO_CREATED_EVENT] for path "<<path<<" error "<< zerror(ret);
        }
        if(zk_event->created_event == NULL) {
            LOG(ERROR) <<"path "<<path<<" eventmask: "<<eventmask<<", created_event func is null" ;
            return ;
        }
        zk_event->created_event(zk_event, zk_helper, path);
    }
    else if(type == ZOO_CHANGED_EVENT && eventmask & CHANGED_EVENT)
    {
        ret = zoo_exists(zh, path, 1, NULL);
        if(ZOK != ret){
            LOG(ERROR) <<"set watcher [ZOO_CHANGED_EVENT] for path "<<path<<" error "<< zerror(ret);
        }
        if(zk_event->changed_event == NULL) {
            LOG(ERROR) <<"path "<<path<<" eventmask: "<<eventmask<<", changed_event func is null";
            return ;
        }
        zk_event->changed_event(zk_event, zk_helper, path);
    }
    else if(type == ZOO_CHILD_EVENT && eventmask & CHILD_EVENT)
    {
        ret = zoo_get_children(zh, path, 1, NULL);
        if(ZOK != ret){
            LOG(ERROR) <<"set watcher [ZOO_CHILD_EVENT] for path "<<path<<" error "<<zerror(ret);
        }
        if(zk_event->child_event == NULL) {
            LOG(ERROR) <<"path "<<path<<" eventmask: "<<eventmask<<", child_event func is null";
            return ;
        }
        zk_event->child_event(zk_event, zk_helper, path);
    }
    else if(type == ZOO_DELETED_EVENT && eventmask & DELETED_EVENT)
    {
        ret = zoo_exists(zh, path, 1, NULL);
        if( ZOK != ret ){
            LOG(ERROR) <<"set watcher [ZOO_DELETED_EVENT] for path "<<path<<" error "<< zerror(ret);
        }
        if(zk_event->deleted_event == NULL) {
            LOG(ERROR) <<"path "<<path<<" eventmask: "<<eventmask<<", deleted_event func is null";
            return ;
        }
        zk_event->deleted_event(zk_event, zk_helper, path);
    }

}

static void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    struct ZookeeperHelper *zk_helper = (struct ZookeeperHelper *)watcherCtx;
    LOG(INFO) <<"Watcher "<<type2Str(type)<<"("<<type<<") state = "<<state2Str(state)<<"("<<state<<") for path = "<<((path && strlen(path)>0) ? path : "");
    if(type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            LOG(INFO) <<"connected zookeeper";
            pthread_mutex_lock(&zk_helper->lock);
            if(zk_helper->mode == E_REGISTER_M && zk_helper->reconnection_flag)
            {
                //只有在SESSION EXPIRED事件导致的应用重连才创建临时节点
                struct ZkHelperPair *p;
                SLIST_FOREACH(p, &zk_helper->zoo_path_list, next)
                {
                    //printf("2%s,%s\n",p->key, p->value);
                    create_node(zk_helper, p->key, (const char*)p->value, p->flag,NULL,0); 
                }
                zk_helper->reconnection_flag = 0;
            }
            re_set_event(zk_helper);     //重新设置观察点
            pthread_mutex_unlock(&zk_helper->lock);
        }
        else if(state == ZOO_AUTH_FAILED_STATE)
        {
            LOG(ERROR) <<"Authentication failure. Shutting down...";
            zookeeper_close(zk_helper->zhandle);
        }
        else if(state == ZOO_EXPIRED_SESSION_STATE)
        {
            LOG(ERROR) <<"Session expired. Shutting down...";
            //zookeeper_close(zk_helper->zhandle);
            //自动重连
            pthread_mutex_lock(&zk_helper->lock);
            re_connect(zk_helper);
            pthread_mutex_unlock(&zk_helper->lock);
        }
    }
    else 
    {
        pthread_mutex_lock(&zk_helper->lock);
        struct ZkHelperPair *p;
        SLIST_FOREACH(p, &zk_helper->zoo_event_list, next)
        {
            //should be strncmp in the future
            //LOG(INFO) <<"get key "<<p->key;
            if(strcmp(path, p->key) == 0) {
                LOG(INFO) <<"catch key "<<p->key;
                handle_event((ZkEvent*)p->value, zk_helper, type, path);
                break;
            }
        }
        pthread_mutex_unlock(&zk_helper->lock);
    }
}

static void zoo_sleep(unsigned int nmsecs)
{
    struct timeval tval;
    unsigned long nusecs = nmsecs*1000;
    tval.tv_sec=nusecs/1000000;
    tval.tv_usec=nusecs%1000000;
    select(0, NULL, NULL, NULL, &tval );
}

static int get_local_addr(struct ZookeeperHelper *zk_helper)
{
    int fd = 0;
    int interest;
    struct timeval tv;

    int res = zookeeper_interest(zk_helper->zhandle, &fd, &interest, &tv);
    if(res != ZOK){
        LOG(ERROR) <<"get myself ip and port error "<< zerror(res)<<"("<<res<<")";
        return -1;
    }

    struct sockaddr_in addr_;
    socklen_t addr_len = sizeof(addr_);

    if(-1 == getsockname(fd, (struct sockaddr*)&addr_, &addr_len)){
        LOG(ERROR) <<"getsockname error "<< strerror(errno) <<"(fd="<<fd<<")";
        return -1;
    }
    char ip_addr[32];
    if(!inet_ntop(AF_INET, &addr_.sin_addr, ip_addr, sizeof(ip_addr))){
        LOG(ERROR) <<"inet_ntop error ", strerror(errno);
        return -1;
    }
    strncpy(zk_helper->local_addr,ip_addr,32);
    zk_helper->local_port = ntohs(addr_.sin_port);
    return 0;
}

int zoo_helper_get_children(struct ZookeeperHelper *zk_helper, \
        const char* path, struct String_vector *node_vector)
{
    if(zk_helper == NULL)
        return -1;
    node_vector->count = 0;
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M){
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    int res = zoo_get_children(zk_helper->zhandle, path, 0, node_vector);
    pthread_mutex_unlock(&zk_helper->lock);
    if(res != ZOK)
    {
        LOG(ERROR) <<"Get "<<path<<" error "<< zerror(res)<<"(" << res<<")";
        return -1;
    }
    return 0;
}

int zoo_helper_get(struct ZookeeperHelper *zk_helper, \
        const char* path, char *buf, int *buf_len)
{
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M){
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    int res = zoo_get(zk_helper->zhandle, path, 0, buf, buf_len, NULL);
    pthread_mutex_unlock(&zk_helper->lock);
    if(res != ZOK)
    {
        LOG(ERROR) <<"Get "<<path<<" error "<< zerror(res)<<"(" << res<<")";
        return -1;
    }
    buf[*buf_len]='\0';
    LOG(INFO) <<"Get "<<path<<" value "<<buf<<" len = "<< *buf_len;
    return 0;
}

int zoo_helper_exists(struct ZookeeperHelper *zk_helper, \
        const char *path)
{
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M){
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    int res = zoo_exists(zk_helper->zhandle, path, 0, NULL);
    pthread_mutex_unlock(&zk_helper->lock);
    if(res == ZOK)
        return 1;
    else if(res == ZNONODE){
        return 0;
    }else{
        LOG(ERROR) <<"zoo_exists path="<<path<<" error "<< zerror(res)<<"(" << res<<")";
        return -1;
    }
}

int zoo_helper_set(struct ZookeeperHelper *zk_helper, \
        const char* path, const char* buffer, int buflen, int version)
{
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M){
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    int res = zoo_set(zk_helper->zhandle, path, buffer, buflen, version);
    pthread_mutex_unlock(&zk_helper->lock);
    if(res == ZOK)
        return 0;
    LOG(ERROR) <<"zookeeper set path="<<path<<" error "<< zerror(res)<<"(" << res<<")";
    return -1;
}

int zoo_helper_delete(struct ZookeeperHelper *zk_helper, \
        const char* path, int version)
{
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M){
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    int res = zoo_delete(zk_helper->zhandle, path, version);
    pthread_mutex_unlock(&zk_helper->lock);
    if(res == ZOK)
        return 0;
    LOG(ERROR) <<"zookeeper delete path="<<path<<" error "<< zerror(res)<<"(" << res<<")";
    return -1;
}

int zoo_helper_create(struct ZookeeperHelper *zk_helper, \
        const char* path, const char* value, int value_len, int flags)
{
    pthread_mutex_lock(&zk_helper->lock);
    if(zk_helper->mode == E_DESTORY_M){
        pthread_mutex_unlock(&zk_helper->lock);
        return -1;
    }
    char buf[256];
    int res = zoo_create(zk_helper->zhandle, path, value, value_len, &ZOO_OPEN_ACL_UNSAFE, flags, buf, sizeof(buf));
    pthread_mutex_unlock(&zk_helper->lock);
    if(res == ZOK)
        return 0;
        LOG(ERROR) <<"zookeeper create path="<<path<<" error "<< zerror(res)<<"(" << res<<")";
    return -1;
}

int zoo_helper_election_test(struct ZookeeperHelper *zk_helper, char *path){
    int node_len=strlen(zk_helper->election_node);
    if(node_len>0){
        char *path_pos = 0;
        path_pos = path+strlen(path);
        while(1)
        {
            if(*(--path_pos) == '/')
            break;
        }
        path[path_pos-path]='\0';

        char buffer[256];
        snprintf(buffer, path_pos-path, "%s/%s", path, zk_helper->election_node);
        int res = zoo_exists(zk_helper->zhandle, buffer, 0, NULL);
        if(res != ZOK) {
            struct String_vector node_vector;
            char c=path[path_pos-path];
            zoo_helper_get_children(zk_helper, path, &node_vector);
            path[path_pos-path]=c;
            int rc;
            int buf_len;
            int str_uuid_len=strlen(zk_helper->election_uuid);
            char buf[512];
            char p[50];
            for (int i = 0;  i < node_vector.count; i++) {
                sprintf(p,"%s/%s",path,node_vector.data[i]);
                memset(buf,'\0',512);
                rc=zoo_helper_get(zk_helper, p, buf, &buf_len);
                if (rc != 0 ) {
                    continue;
                }
                if(buf_len==str_uuid_len && memcmp(zk_helper->election_uuid, buf,buf_len) == 0){
                    strncpy(zk_helper->election_node,buf,buf_len);
                    path[path_pos-path]='/';
                    return 0;
                }            
            }
        }
        path[path_pos-path]='/';
        res = zoo_create(zk_helper->zhandle, path, zk_helper->election_uuid, strlen(zk_helper->election_uuid), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL|ZOO_SEQUENCE, buffer, 256);
        if(res==ZOK){
            fetch_node(buffer,strlen(buffer), zk_helper->election_node);
       }
    }
    return 0;
}
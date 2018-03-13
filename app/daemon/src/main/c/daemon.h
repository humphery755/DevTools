#ifndef _Included_com_alibaba_rocketmq_broker_daemon
#define _Included_com_alibaba_rocketmq_broker_daemon
#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	int pid;
	int status;// 0 start, 1 wait, 2 shutdown
}wrapper_shm_pstat_t;

typedef struct{
	wrapper_shm_pstat_t dp;
	wrapper_shm_pstat_t mp;
	wrapper_shm_pstat_t wp;	
}wrapper_shm_t;

extern int childPid;
extern int shmid;

#ifdef __cplusplus
}
#endif
#endif
/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_sys.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ23ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef BASE_INCLUDE_CM_SYS_H_
#define BASE_INCLUDE_CM_SYS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define CM_MEM_DEBUG 0

typedef pthread_mutex_t cm_mutex_t;
typedef sem_t cm_sem_t;
typedef pthread_t cm_thread_t;

#ifdef __unix__
typedef hrtime_t cm_hrtime_t;
#define CM_GET_HRTIME() gethrtime()
#else
typedef long long cm_hrtime_t;
#define CM_GET_HRTIME() (long long(time())*1000000000)
#endif

#define CM_HRTIME_TO_MS(x) ((x)/1000000)
#define CM_HRTIME_TO_S(x) ((x)/1000000000)

#define CM_SLEEP_S(s) sleep(s)
#define CM_SLEEP_US(us) usleep(us)

#if(CM_MEM_DEBUG == 1)
extern void* cm_mem_malloc(int size, const char* func, int line);
extern void cm_mem_free(void *p,const char* func, int line);

#define CM_MALLOC(len) cm_mem_malloc((len),__FUNCTION__,__LINE__)
#define CM_FREE(p) cm_mem_free((p),__FUNCTION__,__LINE__)
#else
#define CM_MALLOC(len) malloc(len)
#define CM_FREE(p) free(p)

#endif
#define CM_MEM_CPY(dest,destlen, src, srclen) \
    memcpy((dest),(src),(srclen))
#define CM_MEM_ZERO(p, len) memset((p),0,len)

#define CM_VSPRINTF(buf,size,...) snprintf((buf),(size),__VA_ARGS__)
#define CM_SNPRINTF_ADD(buf,size,...) snprintf((buf)+strlen(buf),(size)-strlen(buf),__VA_ARGS__)

#define CM_MUTEX_INIT(p) pthread_mutex_init((p), NULL)
#define CM_MUTEX_DESTROY(p) pthread_mutex_destroy(p)
#define CM_MUTEX_LOCK(p) pthread_mutex_lock(p)
#define CM_MUTEX_UNLOCK(p) pthread_mutex_unlock(p)

#define CM_SEM_INIT(p,c) sem_init((p),0, (c))
#define CM_SEM_DESTROY(p) sem_destroy(p)
#define CM_SEM_WAIT(p) sem_wait(p)
#define CM_SEM_POST(p) sem_post(p)

#ifdef __unix__
#define CM_SEM_WAIT_TIMEOUT(p,s,res) \
    do{ \
       timespec_t tm; \
       gettimeofday(&tm, NULL); \
       tm.tv_sec += (s)+1; \
       (res) = sem_timedwait((p),&tm); \
    }while(0)
#else
#define CM_SEM_WAIT_TIMEOUT(p,s,res) \
    do{ \
       struct timespec tm; \
       gettimeofday(&tm, NULL); \
       tm.tv_sec += (s)+1; \
       (res) = sem_timedwait((p),&tm); \
    }while(0)

#endif

#define CM_THREAD_CREATE(h, func, arg) pthread_create((h), NULL,(func), (arg))
#define CM_THREAD_DETACH(h) pthread_detach(h)
#define CM_THREAD_MYID() pthread_self()
#define CM_THREAD_CANCEL(h) pthread_cancel(h)

#endif /* BASE_INCLUDE_CM_SYS_H_ */

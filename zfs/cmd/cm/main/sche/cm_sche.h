/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_sche.h
 * author     : wbn
 * create date: 2018.05.25
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_SCHE_CM_SCHE_H_
#define MAIN_SCHE_CM_SCHE_H_
#include "cm_common.h"

#define CM_SCHE_FLAG_EXPIRED 1
#define CM_SCHE_FLAG_NAME   2
#define CM_SCHE_FLAG_PARAM  4
#define CM_SCHE_FLAG_TYPE  8
#define CM_SCHE_FLAG_DAYTYPE   16
#define CM_SCHE_FLAG_MINUTE   32
#define CM_SCHE_FLAG_HOURS 64
#define CM_SCHE_FLAG_DAYS 128

#define CM_SCHE_TYPE_ONCE 0
#define CM_SCHE_TYPE_ALWAYS 1

#define CM_SCHE_DAY_NONE 0
#define CM_SCHE_DAY_MONTH 1
#define CM_SCHE_DAY_WEEK 2

typedef struct
{
    uint64 id;
    
    /* 对象，用来产生回调 */
    uint32 obj;
    /* 过期时间 : 0, 永不过期 */
    cm_time_t expired; 

    /* 名称 */
    sint8 name[CM_NAME_LEN];    
    sint8 param[CM_STRING_512];

    /* 0: once, 1: always */
    uint8 type; 

    /* 0: none, 1: month, 2: week */
    uint8 day_type; 
    uint8 minute; /* 0-59 */

    /* 0-23bit, 每一位代表一个小时, 0代表每个小时*/
    uint32 hours; 

    /* month: 0-31bit; week: 0-6bit,如果不指定，每天都执行 */
    uint32 days;
}cm_sche_info_t;

typedef sint32 (*cm_sche_cbk_t)(const sint8* name, const sint8* param);

typedef struct
{
    /* 对象，用来产生回调 */
    const uint32 obj;
    cm_sche_cbk_t cbk;
}cm_sche_config_t;

extern sint32 cm_sche_init(void);

extern sint32 cm_sche_create(cm_sche_info_t *info);

extern sint32 cm_sche_getbatch(uint32 obj, const sint8* opt_param, 
    uint32 offset, uint32 total, 
    cm_sche_info_t **ppAck, uint32 *pCnt);

extern uint64 cm_sche_count(uint32 obj, const sint8* opt_param);

extern sint32 cm_sche_delete(uint64 id);

extern sint32 cm_sche_update(uint64 id, uint32 set_flag,
    cm_sche_info_t *info);

/* 同步回调接口 */
extern sint32 cm_sche_cbk_sync_request(uint64 data_id, void *pdata, uint32 len);
extern sint32 cm_sche_cbk_sync_get(uint64 data_id, void **pdata, uint32 *plen);
extern sint32 cm_sche_cbk_sync_delete(uint64 data_id);    

#endif /* MAIN_SCHE_CM_SCHE_H_ */

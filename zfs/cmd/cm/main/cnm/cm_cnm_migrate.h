/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_migrate.h
 * author     : xar
 * create date: 2018.09.13
 * description: TODO:
 *
 *****************************************************************************/

#ifndef MAIN_MIGRATE_H
#define MAIN_MIGRATE_H
#include "cm_cnm_common.h"
#include "cm_task.h"

typedef struct
{
    uint32 type;
    uint32 nid;
    uint32 operation;
    sint8 path[CM_STRING_256];
    sint8 pool[CM_STRING_64];
    sint8 lunid[CM_STRING_64];
    sint8 progress[CM_STRING_64];
}cm_cnm_migrate_info_t;

extern sint32 cm_cnm_migrate_init(void);

extern sint32 cm_cnm_migrate_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_migrate_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_migrate_create(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_migrate_update(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_migrate_scan(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_migrate_get(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);


extern sint32 cm_cnm_migrate_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_migrate_local_insert(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_migrate_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_migrate_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_migrate_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

sint32 cm_cnm_lun_migrate_task_report(
    cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc);


#endif 

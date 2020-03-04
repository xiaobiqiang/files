/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pool.h
 * author     : wbn
 * create date: 2018.05.07
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_POOL_H_
#define MAIN_CNM_CM_CNM_POOL_H_

#include "cm_cnm_common.h"
#include "cm_task.h"

typedef struct
{
    sint8 name[CM_NAME_LEN_POOL];
    uint32 nid;
    uint32 src_nid;
    uint32 status;
    sint8 avail[CM_STRING_32];
    sint8 used[CM_STRING_32];
    sint8 rdcip[CM_STRING_64];
    /*zpool status*/
    double prog;
    double speed;
    uint32 repaired;
    uint32 errors;
    sint8 time[CM_STRING_32];
    uint32 zstatus;
}cm_cnm_pool_list_t;

typedef struct
{
    uint32 nid;
    uint32 offset;
    uint32 total;
    uint32 raid;
    uint32 type;
    sint8 name[CM_NAME_LEN_POOL];
    sint8 disk_ids[CM_STRING_1K]; 
    uint32 group;
    uint32 operation;
    sint8 args[CM_STRING_128];
    cm_omi_field_flag_t set;
}cm_cnm_pool_list_param_t;

typedef struct
{
    sint8 disk[CM_NAME_LEN_DISK];
    sint8 size[CM_STRING_64];
    uint32 raid;
    uint32 type;
    uint32 err_read;
    uint32 err_write;
    uint32 err_sum;
    /* »ú¹ñºÅ */
    uint32 enid;

    /* ²ÛÎ»ºÅ */
    uint32 slotid;

    /* ×´Ì¬ */
    uint32 status;

    uint32 group_id;
}cm_cnm_pooldisk_info_t;

extern sint32 cm_cnm_pool_init(void);

extern sint32 cm_cnm_pool_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_pool_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pool_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pool_create(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pool_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pool_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pool_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pool_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pool_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pool_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pool_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    

extern sint32 cm_cnm_pool_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_pool_oplog_create(const sint8* sessionid, const void *pDecodeParam, sint32 
Result);   

extern void cm_cnm_pool_oplog_update(const sint8* sessionid, const void *pDecodeParam, sint32 
Result);


extern void cm_cnm_pool_oplog_delete(const sint8* sessionid, const void *pDecodeParam, sint32 
Result);

/*============================================================================*/
extern sint32 cm_cnm_pooldisk_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_pooldisk_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pooldisk_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pooldisk_add(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pooldisk_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pooldisk_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pooldisk_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pooldisk_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pooldisk_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pooldisk_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_pooldisk_oplog_add(const sint8* sessionid, const void *pDecodeParam, sint32 
Result);   

extern void cm_cnm_pooldisk_oplog_delete(const sint8* sessionid, const void *pDecodeParam, sint32 
Result);

extern sint32 cm_cnm_pool_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
typedef struct
{
    sint8 name[CM_NAME_LEN_POOL];
    uint32 nid;
    sint8 status[CM_STRING_32];
    sint8 speed[CM_STRING_32];
    sint8 time_cost[CM_STRING_32];
    sint8 progress[CM_STRING_32];
}cm_cnm_pool_reconstruct_info_t;

extern sint32 cm_cnm_pool_reconstruct_init(void);
extern sint32 cm_cnm_pool_reconstruct_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_pool_reconstruct_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_pool_reconstruct_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen); 
extern sint32 cm_cnm_pool_reconstruct_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pool_reconstruct_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_pool_reconstruct_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

/*
*pool operations
*/

enum
{
    CM_CNM_STATUS_CANCELED,
    CM_CNM_STATUS_SCRUBING,
    CM_CNM_STATUS_SCRUBED,        
};
extern sint32 cm_cnm_pool_scan(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pool_local_scan(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_zpool_eximport_task_report(cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc);

extern cm_omi_obj_t cm_cnm_pool_status_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);


/******************************************************************************/
typedef struct
{
    sint8 name[CM_NAME_LEN_POOL];
    uint32 nid;
    uint32 isforce;
    uint32 policy;
    
    uint32 data_raid;
    uint32 data_num;
    uint32 data_group;
    uint32 data_spare;

    uint32 meta_raid;
    uint32 meta_num;
    uint32 meta_group;
    uint32 meta_spare;

    uint32 low_raid;
    uint32 low_num;
    uint32 low_group;
    uint32 low_spare;
}cm_cnm_poolext_info_t;

extern sint32 cm_cnm_poolext_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern sint32 cm_cnm_poolext_create(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_poolext_add(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_poolext_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_poolext_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);  

extern void cm_cnm_poolext_oplog_create(const sint8* sessionid, const void *pDecodeParam, sint32 
Result);   

extern void cm_cnm_poolext_oplog_add(const sint8* sessionid, const void *pDecodeParam, sint32 
Result);


#endif /* MAIN_CNM_CM_CNM_POOL_H_ */

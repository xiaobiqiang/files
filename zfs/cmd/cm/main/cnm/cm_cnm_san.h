/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_san.h
 * author     : wbn
 * create date: 2018.05.17
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_SAN_H_
#define MAIN_CNM_CM_CNM_SAN_H_
#include "cm_cnm_common.h"
#include "cm_task.h"
/*===========================================================================*/

typedef struct
{
    sint8 name[CM_NAME_LEN_LUN];
    sint8 pool[CM_NAME_LEN_POOL];
    uint32 nid;
    uint16 blocksize;
    uint8 write_policy;
    uint8 access_state;
    uint8 state; 

    uint8 is_double;
    uint8 is_compress;
    uint8 is_hot;
    
    uint8 is_single;
    uint8 alarm_threshold; /* is_single:true */
    uint8 dedup;
    
    uint8 qos; /* 0:关闭, 1:IOPS, 2: BW */    
    sint8 qos_val[CM_STRING_32];
    
    sint8 space_total[CM_STRING_32];
    sint8 space_used[CM_STRING_32];
    sint8 pool_avail[CM_STRING_32];
    
}cm_cnm_lun_info_t;

typedef struct
{
    uint32 nid;
    uint32 offset;
    uint32 total;
    cm_cnm_lun_info_t info;
    cm_omi_field_flag_t field;
    cm_omi_field_flag_t set;
}cm_cnm_lun_param_t;

extern sint32 cm_cnm_lun_init(void);

extern sint32 cm_cnm_lun_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_lun_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_lun_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lun_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_lun_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lun_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lun_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lun_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lun_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lun_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lun_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lun_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


extern sint32 cm_cnm_lun_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    

extern sint32 cm_cnm_lun_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_lun_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_lun_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_lun_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/*===========================================================================*/
/* 主机组管理部分 */
extern sint32 cm_cnm_hg_init(void);

extern sint32 cm_cnm_hg_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_hg_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_hg_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_hg_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_hg_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_hg_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_hg_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_hg_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_hg_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_hg_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_hg_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/*============================================================================*/
/* target管理部分 */    

extern sint32 cm_cnm_target_init(void);

extern sint32 cm_cnm_target_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_target_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_target_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_target_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_target_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_target_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_target_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_target_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_target_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_target_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_target_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/*============================================================================*/
extern sint32 cm_cnm_lunmap_init(void);

extern sint32 cm_cnm_lunmap_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_lunmap_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_lunmap_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lunmap_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_lunmap_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lunmap_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lunmap_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lunmap_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_lunmap_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_lunmap_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_lunmap_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/******************************************************************************/    
/* LUN 双活 */

typedef struct
{
    uint32 ins_act;
    uint32 nid_master;
    uint32 nid_slave;
    sint8 path[CM_STRING_256];
    sint8 nic[CM_NAME_LEN];
    sint8 rdc_ip_master[CM_IP_LEN];
    sint8 rdc_ip_slave[CM_IP_LEN];
    uint32 sync;
    sint8 netmask_master[CM_IP_LEN];
    sint8 netmask_slave[CM_IP_LEN];
}cm_cnm_lunmirror_info_t;

extern sint32 cm_cnm_lunmirror_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_lunmirror_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_lunmirror_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lunmirror_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lunmirror_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lunmirror_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_lunmirror_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_lunmirror_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern sint32 cm_cnm_lunmirror_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lunmirror_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lunmirror_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lunmirror_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
    
/* lun 备份 */
typedef struct
{
    uint32 ins_act;
    uint32 upd_act;
    uint32 interval;
    uint32 nid_master;
    sint8 ip_slave[CM_IP_LEN];
    sint8 path_master[CM_STRING_256];
    sint8 path_slave[CM_STRING_256];
    sint8 nic[CM_NAME_LEN];
    sint8 rdc_ip_master[CM_IP_LEN];
    sint8 rdc_ip_slave[CM_IP_LEN];
    uint32 sync;
    sint8 netmask_master[CM_IP_LEN];
    sint8 netmask_slave[CM_IP_LEN];
}cm_cnm_lunbackup_info_t;

extern sint32 cm_cnm_lunbackup_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_lunbackup_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_lunbackup_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lunbackup_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lunbackup_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_lunbackup_modify(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_lunbackup_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_lunbackup_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_lunbackup_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern void cm_cnm_lunbackup_oplog_modify(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern sint32 cm_cnm_lunbackup_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lunbackup_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lunbackup_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lunbackup_local_modify(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_lunbackup_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
 
extern cm_cnm_lunbackup_cbk_task_report
(cm_task_send_state_t *pSnd, cm_task_cmt_echo_t **pproc);


#endif /* MAIN_CNM_CM_CNM_SAN_H_ */

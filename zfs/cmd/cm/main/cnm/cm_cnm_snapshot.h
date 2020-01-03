/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_snapshot.h
 * author     : wbn
 * create date: 2018.05.24
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_SNAPSHOT_H_
#define MAIN_CNM_CM_CNM_SNAPSHOT_H_
#include "cm_cnm_common.h"

typedef struct
{
    sint8 name[CM_NAME_LEN_SNAPSHOT];
    uint32 nid;
    sint8 dir[CM_NAME_LEN_DIR];
    sint8 used[CM_STRING_32];
    cm_time_t tm;
    uint32 type; 
}cm_cnm_snapshot_info_t;

extern sint32 cm_cnm_snapshot_init(void);

extern sint32 cm_cnm_snapshot_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_snapshot_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_snapshot_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_snapshot_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_snapshot_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_snapshot_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_snapshot_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_snapshot_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_snapshot_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_snapshot_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_snapshot_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_snapshot_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


extern sint32 cm_cnm_snapshot_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    

extern sint32 cm_cnm_snapshot_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_snapshot_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_snapshot_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_snapshot_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/*============================================================================*/
extern sint32 cm_cnm_snapshot_sche_cbk(const sint8* name, const sint8* param);
extern sint32 cm_cnm_snapshot_sche_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_snapshot_sche_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_snapshot_sche_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_snapshot_sche_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_snapshot_sche_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_snapshot_sche_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_snapshot_sche_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_snapshot_sche_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_snapshot_sche_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_snapshot_sche_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_snapshot_sche_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);



#endif /* MAIN_CNM_CM_CNM_SNAPSHOT_H_ */

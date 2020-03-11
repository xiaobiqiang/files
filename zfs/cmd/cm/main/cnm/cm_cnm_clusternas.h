/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_nas.h
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#ifndef CNM_CM_CNM_CLUSTERNAS_H_
#define CNM_CM_CNM_CLUSTERNAS_H_
#include "cm_cnm_common.h"
#include "cm_platform.h"


/*===========================================================================*/

typedef struct
{
    sint8 group_name[CM_NAME_LEN_NAS];
    uint8 cluster_status; 
    sint8 nas[CM_NAME_LEN_NAS];
    uint8 role;
    uint8 status;
    sint8 avail[CM_STRING_32];
    sint8 used[CM_STRING_32];
    uint32 nid;
    uint32 nas_num;
}cm_cnm_cluster_nas_info_t;

extern sint32 cm_cnm_cluster_nas_init(void);

extern sint32 cm_cnm_cluster_nas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_cluster_nas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_cluster_nas_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_cluster_nas_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32  cm_cnm_cluster_nas_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_nas_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_cluster_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_nas_count(
     const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_nas_local_count(
     void *param, uint32 len,
     uint64 offset, uint32 total, 
     void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_nas_local_create(
     void *param, uint32 len,
     uint64 offset, uint32 total, 
     void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_nas_local_add(
     void *param, uint32 len,
     uint64 offset, uint32 total, 
     void **ppAck, uint32 *pAckLen);

extern sint32  cm_cnm_cluster_nas_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern void cm_cnm_cluster_nas_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern void cm_cnm_cluster_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);
/*===========================================================================*/
#endif

















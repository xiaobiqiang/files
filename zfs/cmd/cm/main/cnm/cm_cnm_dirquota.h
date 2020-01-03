/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_dirquota.h
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_DIRQUOTA_H_
#define MAIN_CNM_CM_CNM_DIRQUOTA_H_
#include "cm_cnm_common.h"
#include "cm_log.h"
#include "cm_omi.h"

typedef struct
{    
    sint8 dir_path[CM_STRING_256];  //文件的路径
    uint32 nid; 
    sint8 quota[CM_STRING_32]; //quota size
    sint8 used[CM_STRING_32];
    sint8 rest[CM_STRING_32];
    double per_used;
    sint8 nas[CM_NAME_LEN_NAS];
}cm_cnm_dirquota_info_t;

extern sint32 cm_cnm_dirquota_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_dirquota_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_dirquota_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dirquota_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

  
extern sint32 cm_cnm_dirquota_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_dirquota_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dirquota_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_dirquota_oplog_add(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern void cm_cnm_dirquota_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern sint32 cm_cnm_dirquota_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_dirquota_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_dirquota_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


extern sint32 cm_cnm_dirquota_local_add(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_dirquota_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


#endif 

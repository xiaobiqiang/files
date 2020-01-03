/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_quota.h
 * author     : xar
 * create date: 2018.08.24
 * description: TODO:
 *
 *****************************************************************************/

#ifndef MAIN_QUOTA_H
#define MAIN_QUOTA_H
#include "cm_cnm_common.h"


typedef struct
{	
	uint8 usertype;
    uint32 nid;
	sint8 name[CM_NAME_LEN];
	sint8 filesystem[CM_NAME_LEN];
	sint8 space[CM_STRING_32];/*”≤≈‰∂Ó*/
	sint8 softspace[CM_STRING_32];/*»Ì≈‰∂Ó*/
	sint8 used[CM_STRING_32];/*“—”√ø’º‰*/
}cm_cnm_quota_info_t;

extern sint32 cm_cnm_quota_init(void);

extern sint32 cm_cnm_quota_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_quota_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);


extern sint32 cm_cnm_quota_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_quota_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_quota_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_quota_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_quota_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_quota_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_quota_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_quota_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


extern sint32 cm_cnm_quota_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_quota_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_quota_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    

extern sint32 cm_cnm_quota_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_quota_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_quota_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


/*=======================================================
                    ipshift(ip∆Ø“∆)
=========================================================*/

typedef struct
{
    sint8 filesystem[CM_NAME_LEN];
    uint32 nid;
    sint8 adapter[CM_NAME_LEN];
    sint8 ip[CM_NAME_LEN];
    sint8 netmask[CM_NAME_LEN];
}cm_cnm_ipshift_info_t;

extern sint32 cm_cnm_ipshift_init(void);

extern sint32 cm_cnm_ipshift_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_ipshift_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);

extern sint32 cm_cnm_ipshift_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_ipshift_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_ipshift_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_ipshift_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_ipshift_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_ipshift_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_ipshift_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


extern sint32 cm_cnm_ipshift_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_ipshift_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_ipshift_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_ipshift_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_ipshift_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


#endif /*MAIN_QUOTA_H*/

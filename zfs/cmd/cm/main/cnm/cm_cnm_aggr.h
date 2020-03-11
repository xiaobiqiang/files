/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_aggr.h
 * author     : xar
 * create date: 2018.09.18
 * description: TODO:
 *
 *****************************************************************************/


#ifndef MAIN_AGGR_H
#define MAIN_AGGR_H
#include "cm_cnm_common.h"

typedef struct
{
    sint8 name[CM_STRING_64];
    uint32 nid;
    sint8 state[CM_STRING_64];
    sint8 ip[CM_STRING_64];
    sint8 netmask[CM_STRING_64];
    sint8 mtu[CM_STRING_64];
    sint8 mac[CM_STRING_64];
    sint8 adapter[CM_STRING_64];
    uint32 policy;
    uint32 lacpactivity;
    uint32 lacptimer;
}cm_cnm_aggr_info_t;

extern sint32 cm_cnm_aggr_init(void);

extern sint32 cm_cnm_aggr_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_aggr_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);


extern sint32 cm_cnm_aggr_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_aggr_create(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
    
extern sint32 cm_cnm_aggr_update(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_aggr_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

    
extern sint32 cm_cnm_aggr_count(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_aggr_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_aggr_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
            
extern sint32 cm_cnm_aggr_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
            
extern sint32 cm_cnm_aggr_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);    
            
extern sint32 cm_cnm_aggr_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_aggr_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_aggr_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_aggr_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


#endif

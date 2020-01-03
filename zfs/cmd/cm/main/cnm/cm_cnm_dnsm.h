/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_dnsm.h
 * author     : wbn
 * create date: 2018Äê3ÔÂ16ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_DNSM_H_
#define MAIN_CNM_CM_CNM_DNSM_H_
#include "cm_cnm_common.h"

typedef struct
{
    sint8 ip[CM_NAME_LEN];
    sint8 domain[CM_NAME_LEN];
    uint32 nid;
   
}cm_cnm_dnsm_info_t;

extern sint32 cm_cnm_dnsm_init(void);

extern sint32 cm_cnm_dnsm_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32  cm_cnm_dnsm_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_local_insert(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);



extern sint32 cm_cnm_dnsm_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen); 

extern sint32 cm_cnm_dnsm_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_dnsm_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_dnsm_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_dnsm_insert(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern void cm_cnm_dnsm_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);
extern void cm_cnm_dnsm_oplog_insert(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);
extern void cm_cnm_dnsm_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);
 /*===========================================================================*/
typedef struct
{
    sint8 ip_master[CM_NAME_LEN];
    sint8 ip_slave[CM_NAME_LEN];
}cm_cnm_dns_info_t;
extern sint32 cm_cnm_dns_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_dns_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_dns_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_dns_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_dns_insert(
    void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_dns_sync_request(uint64 enid, void *pdata, uint32 len);
typedef struct
{
    sint8 domain[CM_NAME_LEN];
    sint8 dc_name[CM_NAME_LEN];
    sint8 username[CM_NAME_LEN];
    sint8 userpwd[CM_NAME_LEN];
    sint8 state[CM_NAME_LEN];
}cm_cnm_domain_ad_info_t;
extern sint32 cm_cnm_domain_ad_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_domain_ad_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_domain_ad_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_domain_ad_insert(
    void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_domain_ad_delete(
    void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
#endif





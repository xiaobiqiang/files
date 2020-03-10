/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_phys.h
 * author     : wbn
 * create date: 2018年3月16日
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_PHYS_H_
#define MAIN_CNM_CM_CNM_PHYS_H_
#include "cm_cnm_common.h"

typedef struct
{
    sint8 name[CM_NAME_LEN];
    uint32 nid;
    uint32 state;
    uint32 speed;
    uint32 duplex;
    uint32 mtu;
    sint8 mac[CM_STRING_32];
}cm_cnm_phys_info_t;

extern sint32 cm_cnm_phys_init(void);

extern sint32 cm_cnm_phys_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_phys_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_phys_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_phys_get_batch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_phys_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_phys_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_phys_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_phys_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_phys_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_phys_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


extern sint32 cm_cnm_phys_cbk_cmt(
 void *pMsg, uint32 Len, void** ppAck, uint32 *pAckLen);

/*********************************************************
                     route(路由管理)
**********************************************************/

typedef struct
{
    sint8 destination[CM_NAME_LEN];
    uint32 nid;
    sint8 netmask[CM_NAME_LEN];
    sint8 geteway[CM_NAME_LEN];
}cm_cnm_route_info_t;

extern sint32 cm_cnm_route_init(void);

extern sint32 cm_cnm_route_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_route_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_route_create(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_route_delete(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_route_getbatch(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_route_count(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_route_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_route_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_route_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_route_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_route_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_route_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


/****************************************************
                    phys_ip
*****************************************************/

typedef struct
{
    sint8 adapter[CM_NAME_LEN];
    uint32 nid;
    sint8 ip[CM_NAME_LEN];
    sint8 netmask[CM_NAME_LEN];
}cm_cnm_phys_ip_info_t;

extern sint32 cm_cnm_phys_ip_init(void);

extern sint32 cm_cnm_phys_ip_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_phys_ip_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_phys_ip_getbatch(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_phys_ip_count(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_phys_ip_create(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_phys_ip_delete(
    const void* pDecodeParam,void** ppAckData,uint32* pAckLen);

extern sint32 cm_cnm_phys_ip_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_phys_ip_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_phys_ip_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_phys_ip_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_phys_ip_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_phys_ip_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/***************************************
               fcinfo 
***************************************/

typedef struct
{
    sint8 port_wwn[CM_NAME_LEN];
    uint32 nid;
    sint8 port_mode[CM_NAME_LEN];
    sint8 driver_name[CM_NAME_LEN];
    sint8 state[CM_NAME_LEN];
    sint8 speed[CM_NAME_LEN];
    sint8 cur_speed[CM_NAME_LEN];
}cm_cnm_fcinfo_info_t;

extern sint32 cm_cnm_fcinfo_init(void);
extern sint32 cm_cnm_fcinfo_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_fcinfo_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_fcinfo_getbatch(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);
extern sint32 cm_cnm_fcinfo_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_fcinfo_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_fcinfo_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
/***********************************ipfilter***********************************/
typedef struct
{
    sint8 nic[CM_IP_LEN];
    uint32 nid;
    sint8 ip[CM_IP_LEN];
    uint32 port;
    uint8 status;
    uint8 operate;
}cm_cnm_ipf_info_t;
extern sint32 cm_cnm_ipf_init(void);
extern sint32 cm_cnm_ipf_decode(const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_ipf_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_ipf_getbatch(const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);
extern sint32 cm_cnm_ipf_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_insert(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_local_insert(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_local_update(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_ipf_local_delete(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
extern void cm_cnm_ipf_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);
extern void cm_cnm_ipf_oplog_insert(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);
extern void cm_cnm_ipf_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);
#endif /* MAIN_CNM_CM_CNM_PHYS_H_ */

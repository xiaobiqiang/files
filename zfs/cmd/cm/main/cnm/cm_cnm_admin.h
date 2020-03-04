/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_admin.h
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_ADMIN_H_
#define MAIN_CNM_CM_CNM_ADMIN_H_
#include "cm_cnm_common.h"


/*===========================================================================*/

typedef struct
{
    uint32 id;
    uint32 level;
    sint8 name[CM_NAME_LEN];    
    sint8 pwd[CM_NAME_LEN];       
}cm_cnm_admin_info_t;

extern sint32 cm_cnm_admin_init(void);

extern sint32 cm_cnm_admin_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_admin_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_admin_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_admin_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_admin_create(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_admin_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_admin_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_admin_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_admin_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_admin_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_admin_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern sint32 cm_cnm_admin_sync_request(uint64 data_id, void *pdata, uint32 len);
extern sint32 cm_cnm_admin_sync_get(uint64 data_id, void **pdata, uint32 *plen);
extern sint32 cm_cnm_admin_sync_delete(uint64 data_id);


typedef struct
{
    uint32 obj;
    uint32 cmd;
    uint32 mask;
}cm_cnm_permission_info_t;

extern sint32 cm_cnm_permission_init(void);

extern sint32 cm_cnm_permission_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
    
extern cm_omi_obj_t cm_cnm_permission_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_permission_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_permission_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
   
extern sint32 cm_cnm_permission_add(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_permission_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_permission_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_permission_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_permission_oplog_add(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);   

extern void cm_cnm_permission_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);  

extern void cm_cnm_permission_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern sint32 cm_cnm_permission_sync_request(uint64 data_id, void *pdata, uint32 len);
extern sint32 cm_cnm_permission_sync_get(uint64 data_id, void **pdata, uint32 *plen);
extern sint32 cm_cnm_permission_sync_delete(uint64 data_id);

#endif /* MAIN_CNM_CM_CNM_ADMIN_H_ */


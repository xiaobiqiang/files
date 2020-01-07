/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_node.h
 * author     : wbn
 * create date: 2017Äê11ÔÂ13ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_NODE_H_
#define MAIN_CNM_CM_CNM_NODE_H_

#include "cm_omi_types.h"
#include "cm_sync_types.h"
#include "cm_cnm_common.h"

extern sint32 cm_cnm_node_init(void);

extern sint32 cm_cnm_node_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_node_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_node_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_node_get_batch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_node_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_node_modify(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_node_add(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_node_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_node_power_on(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_node_power_off(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_node_reboot(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_node_oplog_modify(const sint8* sessionid,const void *pDecodeParam, sint32 Result);

extern void cm_cnm_node_oplog_add(const sint8* sessionid,const void *pDecodeParam, sint32 Result);

extern void cm_cnm_node_oplog_delete(const sint8* sessionid,const void *pDecodeParam, sint32 Result);

extern void cm_cnm_node_oplog_on(const sint8* sessionid,const void *pDecodeParam, sint32 Result);

extern void cm_cnm_node_oplog_off(const sint8* sessionid,const void *pDecodeParam, sint32 Result);

extern void cm_cnm_node_oplog_reboot(const sint8* sessionid,const void *pDecodeParam, sint32 Result);


extern sint32 cm_cnm_node_local_off(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_node_local_reboot(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);



/*************************************************************************************
*service manage
**************************************************************************************/
typedef struct
{
    sint8 service[CM_STRING_64];
    uint32 nid;
    uint32 nfs;  
    uint32 stmf;
    uint32 ssh;
    uint32 ftp;
    uint32 smb;
    uint32 guiview;
    uint32 fmd;
    uint32 iscsi;
}cm_cnm_node_service_info_t;

#define cm_cnm_node_service_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_NODE_SERVCE,cmd,sizeof(cm_cnm_node_service_info_t),param,ppAck,plen)


extern sint32 cm_cnm_node_service_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_node_service_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_node_service_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_node_service_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

  
extern sint32 cm_cnm_node_service_updata(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_node_service_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_node_service_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_node_service_local_updata(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern void cm_cnm_node_service_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

/* upgrade */

typedef struct
{
    uint32 process;
    uint32 nid;
    uint32 state;
    uint32 mode;
    sint8 name[CM_STRING_128];
    sint8 rddir[CM_STRING_128];
    sint8 version[CM_STRING_128];
}cm_cnm_upgrade_info_t;

extern sint32 cm_cnm_upgrade_init();

extern sint32 cm_cnm_upgrade_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_upgrade_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_upgrade_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_upgrade_insert(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_upgrade_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_upgrade_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_upgrade_local_insert(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

    extern sint32 cm_cnm_upgrade_local_count(
        void *param, uint32 len,
        uint64 offset, uint32 total, 
        void **ppAck, uint32 *pAckLen);

#endif MAIN_CNM_CM_CNM_NODE_H_


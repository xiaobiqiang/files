/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_duallive_nas.h
 * author     : jdz
 * create date: 2018.08.24
 * description: TODO:nas/sanË«»î
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_DUALLIVE_H_
#define MAIN_CNM_CM_CNM_DUALLIVE_H_

#include "cm_cnm_duallive.h"
#include "cm_log.h"
#include "cm_sync.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_cnm_common.h"

typedef struct
{    
    uint8 mnas[CM_NAS_PATH_LEN];
    uint32 mnid;
    uint8 snas[CM_NAS_PATH_LEN];
    uint32 snid;
    uint32 synctype;
    sint8 work_load_if[CM_STRING_32];
    sint8 work_load_ip[CM_IP_LEN];
    sint8 netmask[CM_IP_LEN];
    uint8 status[CM_STRING_32];
}cm_cnm_duallive_nas_info_t;

#define cm_cnm_nas_duallive_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_DUALLIVE_NAS,cmd,sizeof(cm_cnm_duallive_nas_info_t),param,ppAck,plen)

extern sint32 cm_cnm_duallive_nas_init(void);

extern sint32 cm_cnm_duallive_nas_decode(
	const cm_omi_obj_t ObjParam,void **ppDecodeParam);

extern cm_omi_obj_t cm_cnm_duallive_nas_encode(
	const void *pDecodeParam,void *pAckData,uint32 AckLen);


extern sint32 cm_cnm_duallive_nas_create(
	const void *pDecodeParam,void **ppAckData,uint32 * pAckLen);

extern sint32 cm_cnm_duallive_nas_delete(
	const void * pDecodeParam,void **ppAckData,uint32 * pAckLen);

extern sint32 cm_cnm_duallive_nas_count(
	const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);

extern sint32 cm_cnm_duallive_nas_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 * pAckLen);

extern sint32 cm_cnm_duallive_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_duallive_nas_local_count(
        void *param, uint32 len,
        uint64 offset, uint32 total, 
        void **ppAck, uint32 *pAckLen);
extern sint32 cm_cnm_duallive_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_duallive_nas_local_delete(
        void *param, uint32 len,
        uint64 offset, uint32 total, 
        void **ppAck, uint32 *pAckLen);

extern void cm_cnm_duallive_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


extern void cm_cnm_duallive_nas_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

typedef struct
{
	uint8 mnas[CM_NAS_PATH_LEN];
    uint32 mnid;
	uint8 snas[CM_NAS_PATH_LEN];
    uint32 snid; 
    uint32 synctype;
    sint8 status[CM_STRING_32];
}cm_cnm_backup_nas_info_t;

#define cm_cnm_nas_backup_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_BACKUP_NAS,cmd,sizeof(cm_cnm_backup_nas_info_t),param,ppAck,plen)
extern sint32 cm_cnm_backup_nas_init(void);

extern sint32 cm_cnm_backup_nas_decode(
	const cm_omi_obj_t ObjParam,void **ppDecodeParam);

extern cm_omi_obj_t cm_cnm_backup_nas_encode(
	const void *pDecodeParam,void *pAckData,uint32 AckLen);


extern sint32 cm_cnm_backup_nas_create(
	const void *pDecodeParam,void **ppAckData,uint32 * pAckLen);

extern sint32 cm_cnm_backup_nas_delete(
	const void * pDecodeParam,void **ppAckData,uint32 * pAckLen);

extern sint32 cm_cnm_backup_nas_count(
	const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);

extern sint32 cm_cnm_backup_nas_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 * pAckLen);

extern sint32 cm_cnm_backup_nas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_backup_nas_local_count(
        void *param, uint32 len,
        uint64 offset, uint32 total, 
        void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_backup_nas_local_create(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_backup_nas_local_delete(
        void *param, uint32 len,
        uint64 offset, uint32 total, 
        void **ppAck, uint32 *pAckLen);

extern void cm_cnm_backup_nas_oplog_create(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);


extern void cm_cnm_backup_nas_oplog_delete(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);



typedef struct
{
	uint8 list_target[CM_STRING_32];
    uint32 nid;
}cm_cnm_cluster_target_info_t;

#define cm_cnm_cluster_target_req(cmd,param,ppAck,plen) \
    cm_cnm_request_comm(CM_OMI_OBJECT_CLUSTERSAN_TARGET,cmd,sizeof(cm_cnm_cluster_target_info_t),param,ppAck,plen)
    
extern sint32 cm_cnm_cluster_target_init(void);

extern sint32 cm_cnm_cluster_target_decode(
	const cm_omi_obj_t ObjParam,void **ppDecodeParam);

extern cm_omi_obj_t cm_cnm_cluster_target_encode(
	const void *pDecodeParam,void *pAckData,uint32 AckLen);

extern sint32 cm_cnm_cluster_target_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 * pAckLen);

extern sint32 cm_cnm_cluster_target_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);


#endif


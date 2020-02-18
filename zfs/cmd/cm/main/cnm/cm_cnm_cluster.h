/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_cluster.h
 * author     : wbn
 * create date: 2017Äê11ÔÂ13ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_CLUSTER_H_
#define MAIN_CNM_CM_CNM_CLUSTER_H_

#include "cm_omi_types.h"
#include "cm_sync_types.h"

extern sint32 cm_cnm_cluster_init(void);

extern sint32 cm_cnm_cluster_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_cluster_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_cluster_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_cluster_modify(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_power_off(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_reboot(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cluster_bind_master_ip(uint32 old_id, uint32 new_id);
extern void cm_cluster_check_bind_cluster_ip(void);

extern void cm_cnm_cluster_oplog_modify(const sint8* sessionid,const void *pDecodeParam, sint32 Result);

extern void cm_cnm_cluster_oplog_power_off(const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern void cm_cnm_cluster_oplog_reboot(const sint8* sessionid, const void *pDecodeParam, sint32 Result);

extern sint32 cm_cnm_cluster_stat_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_stat_init(void);

cm_omi_obj_t cm_cnm_cluster_stat_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_cluster_stat_sync_request(uint64 data_id, void *pdata, uint32 
len);


extern sint32 cm_cnm_cluster_remote_init(void);

extern sint32 cm_cnm_cluster_remote_decode(const cm_omi_obj_t ObjParam, void **ppDecodeParam);
extern cm_omi_obj_t cm_cnm_cluster_remote_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_cluster_remote_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_remote_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_remote_insert(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_remote_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_remote_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_cluster_remote_sync_request(uint64 data_id, void *pdata, uint32 
len);

extern sint32 cm_cnm_cluster_remote_sync_get(uint64 data_id, void **pdata, uint32 
*plen);

extern sint32 cm_cnm_cluster_remote_sync_delete(uint64 data_id);

#endif /* MAIN_CNM_CM_CNM_CLUSTER_H_ */

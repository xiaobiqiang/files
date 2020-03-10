/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_sync.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ27ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_SYNC_CM_SYNC_H_
#define MAIN_SYNC_CM_SYNC_H_
#include "cm_common.h"
#include "cm_sync_types.h"

extern sint32 cm_sync_init(void);

extern sint32 cm_sync_request(uint32 obj_id, uint64 data_id,void *pdata, uint32 len);

extern sint32 cm_sync_delete(uint32 obj_id, uint64 data_id);

extern void cm_sync_master_change(uint32 old_id, uint32 new_id);

extern sint32 cm_sync_cbk_cmt_obj_info(void *pMsg, uint32 Len, void **ppAck, uint32 
*pAckLen);

extern sint32 cm_sync_cbk_cmt_obj_step(void *pMsg, uint32 Len, void **ppAck, uint32 
*pAckLen);

extern sint32 cm_sync_cbk_cmt_request(void *pMsg, uint32 Len, void **ppAck, uint32 
*pAckLen);

extern sint32 cm_sync_cbk_cmt_tomaster(void *pMsg, uint32 Len, void **ppAck, uint32 
*pAckLen);

#endif /* MAIN_SYNC_CM_SYNC_H_ */

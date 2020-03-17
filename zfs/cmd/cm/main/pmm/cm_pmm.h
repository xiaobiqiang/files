/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_pmm.h
 * author     : wbn
 * create date: 2017Äê10ÔÂ27ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_PMM_CM_PMM_H_
#define MAIN_PMM_CM_PMM_H_
#include "cm_omi_types.h"
#include "cm_pmm_types.h"

extern sint32 cm_pmm_init(void);

extern sint32 cm_pmm_cbk_cmt(
    void *pMsg, uint32 Len, void**ppAck, uint32 *pAckLen);

extern sint32 cm_pmm_decode(uint32 obj,const cm_omi_obj_t ObjParam, void
**ppDecodeParam);

extern cm_omi_obj_t cm_pmm_encode(
    uint32 obj,const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_pmm_current(
    uint32 obj,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_history(
    uint32 obj,const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_request(
    uint32 nid,uint32 msg,const void* req, uint32 len, void **ppAckData, 
    uint32 *pAckLen);
    
extern sint32 cm_pmm_current_mem(
    uint32 obj,uint32 id,uint32 parent_id,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_db_update(uint32 obj,cm_pmm_data_t *pdata);

extern sint32 cm_pmm_cfg_sync_request(uint64 data_id, void *pdata, uint32 len);

extern sint32 cm_pmm_cfg_sync_get(uint64 data_id, void **pdata, uint32 *plen);

extern sint32 cm_pmm_cfg_update(cm_pmm_cfg_t *cfg);

extern sint32 cm_pmm_current_mem(uint32 obj,uint32 id,uint32 parent_id,void **ppAckData, uint32 *pAckLen);
    
#endif /* MAIN_PMM_CM_PMM_H_ */

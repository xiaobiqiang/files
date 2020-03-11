/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_systime.h
 * author     : wbn
 * create date: 2018Äê3ÔÂ2ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_SYSTIME_H_
#define MAIN_CNM_CM_CNM_SYSTIME_H_

#include "cm_omi_types.h"

extern sint32 cm_cnm_systime_init(void);

extern sint32 cm_cnm_systime_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_systime_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_systime_set(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_systime_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_systime_sync_request(uint64 data_id, void *pdata, uint32 len);
extern void cm_cnm_systime_oplog_set(const sint8* sessionid,const void *pDecodeParam, sint32 Result);


/****************** ntp  ********************/

typedef struct
{
    sint8 ip[CM_STRING_128];
}cm_cnm_ntp_info_t;

extern sint32 cm_cnm_ntp_init(void);

extern sint32 cm_cnm_ntp_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_ntp_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_ntp_insert(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_ntp_delete(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_ntp_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern void cm_cnm_ntp_master_change(uint32 old_id, uint32 new_id);

extern void cm_cnm_ntp_sync_thread(void);

extern sint32 cm_cnm_ntp_sync_request(uint64 data_id, void *pdata, uint32 len);

extern sint32 cm_cnm_ntp_sync_delete(uint64 enid);

extern sint32 cm_cnm_ntp_sync_get(uint64 data_id, void **pdata, uint32 *plen);



#endif /* MAIN_CNM_CM_CNM_SYSTIME_H_ */

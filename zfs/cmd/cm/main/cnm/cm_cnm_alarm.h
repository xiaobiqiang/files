/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_alarm.h
 * author     : wbn
 * create date: 2018Äê1ÔÂ22ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_ALARM_H_
#define MAIN_CNM_CM_CNM_ALARM_H_

extern sint32 cm_cnm_alarm_init(void);

extern sint32 cm_cnm_alarm_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_alarm_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_alarm_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_alarm_current_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_alarm_current_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_alarm_history_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_alarm_history_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_alarm_history_delete(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);


extern sint32 cm_cnm_alarm_cfg_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_alarm_cfg_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_alarm_cfg_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_alarm_cfg_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_alarm_cfg_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_alarm_cfg_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern void cm_cnm_alarm_cfg_oplog_update(const sint8* sessionid,const void *pDecodeParam, sint32 Result); 

extern sint32 cm_cnm_alarm_cfg_sync_request(uint64 data_id, void *pdata, uint32 len);
extern sint32 cm_cnm_alarm_cfg_sync_get(uint64 data_id, void **pdata, uint32 *plen);
extern sint32 cm_cnm_alarm_cfg_sync_delete(uint64 data_id);

/*  =====================================   */
extern sint32 cm_cnm_threshold_init(void);
extern sint32 cm_cnm_threshold_get(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_threshold_delete(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_threshold_update(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_threshold_count(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);
extern sint32 cm_cnm_threshold_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);
extern sint32 cm_cnm_threshold_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_threshold_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_threshold_sync_request(uint64 data_id, void *pdata, uint32 len);
extern sint32 cm_cnm_threshold_sync_get(uint64 data_id, void **pdata, uint32 *plen);
extern sint32 cm_cnm_threshold_sync_delete(uint64 data_id);

extern void cm_cnm_storage_alarm_thread(void);
extern sint32 cm_cnm_storage_alarm_init(void);

extern void cm_cnm_alarm_common_thread(void);

typedef struct
{  
    uint8 state;
    uint8 useron;
    sint8 sender[CM_STRING_64];
    sint8 server[CM_STRING_64];
    uint32 port; 
    uint8 language;
    sint8 user[CM_STRING_64];
    sint8 passwd[CM_STRING_64];
}cm_cnm_mailsend_info_t;
extern sint32 cm_cnm_mailsend_init(void);
extern sint32 cm_cnm_mailsend_get(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_mailsend_update(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_mailsend_test(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_mailsend_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_mailsend_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_mailsend_sync_request(uint64 enid, void *pdata, uint32 len);
extern sint32 cm_cnm_mailsend_sync_get(uint64 id, void **pdata, uint32 *plen);
typedef struct
{    
    uint32 id;
    sint8 receiver[CM_STRING_64];
    uint8 level;
}cm_cnm_mailrecv_info_t;
extern sint32 cm_cnm_mailrecv_init(void);
extern sint32 cm_cnm_mailrecv_get(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_mailrecv_delete(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_mailrecv_update(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32  cm_cnm_mailrecv_insert(
        void *pDecodeParam,void **ppAckData,uint32 * pAckLen);
extern sint32 cm_cnm_mailrecv_count(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);
extern sint32 cm_cnm_mailrecv_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);

extern sint32 cm_cnm_mailrecv_test(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);


extern sint32 cm_cnm_mailrecv_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_mailrecv_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_mailrecv_sync_request(uint64 enid, void *pdata, uint32 len);
extern sint32 cm_cnm_mailrecv_sync_get(uint64 id, void **pdata, uint32 *plen);
extern sint32 cm_cnm_mailrecv_sync_delete(uint64 id);

#endif /* MAIN_CNM_CM_CNM_ALARM_H_ */

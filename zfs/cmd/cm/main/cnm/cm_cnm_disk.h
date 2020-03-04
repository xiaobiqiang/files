/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_disk.h
 * author     : wbn
 * create date: 2018.04.25
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_DISK_H_
#define MAIN_CNM_CM_CNM_DISK_H_
#include "cm_cnm_common.h"

typedef struct
{
    /* Ӳ���� */
    sint8 id[CM_ID_LEN];

    /* �ڵ�ID */
    uint32 nid; 
    
    /* ���к� */
    sint8 sn[CM_SN_LEN];

    /* ���� */
    sint8 vendor[CM_NAME_LEN];

    /* ���� MB */
    sint8 size[CM_STRING_64];

    /* ת��  */
    uint32 rpm;
    
    /* ����� */
    uint32 enid;

    /* ��λ�� */
    uint32 slotid;

    /* ״̬ */
    uint32 status;

    /*����pool*/
    sint8 pool[CM_NAME_LEN_POOL];

    /* �Ƿ񱾵��� */
    uint32 islocal;
}cm_cnm_disk_info_t;

typedef struct
{
    cm_cnm_disk_info_t disk;
    cm_omi_field_flag_t field;
    cm_omi_field_flag_t set;
    uint32 offset;
    uint32 total;  
}cm_cnm_disk_req_param_t;

extern sint32 cm_cnm_disk_init(void);

extern sint32 cm_cnm_disk_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_disk_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_disk_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_disk_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_disk_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_disk_clear(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern void cm_cnm_disk_oplog_clear(const sint8* sessionid, const void *pDecodeParam, sint32 Result);


extern sint32 cm_cnm_disk_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);
    
extern sint32 cm_cnm_disk_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_disk_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_disk_local_clear(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

/*============================================================================*/
extern sint32 cm_cnm_diskspare_init(void);

extern sint32 cm_cnm_diskspare_decode(const cm_omi_obj_t ObjParam, void
**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_diskspare_encode(const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_diskspare_getbatch(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_diskspare_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_diskspare_count(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_diskspare_update(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern void cm_cnm_diskspare_oplog_update(
    const sint8* sessionid, const void *pDecodeParam, sint32 Result);

#endif /* MAIN_CNM_CM_CNM_DISK_H_ */

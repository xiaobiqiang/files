/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pmm_lun.h
 * author     : wbn
 * create date: 2018.05.17
 * description: TODO:
 *
 *****************************************************************************/


#include "cm_log.h"
#include "cm_omi.h"
#ifndef MAIN_CNM_CM_CNM_PMM_LUN_H_
#define MAIN_CNM_CM_CNM_PMM_LUN_H_
#include "cm_cnm_common.h"


typedef struct
{  
    uint32 id;
    cm_time_t tmstamp;
    double reads;
    double writes;
    double nread;
    double nread_kb;
    double nwritten;
    double nwritten_kb;
}cm_cnm_pmm_lun_info_t;

extern sint32 cm_cnm_pmm_lun_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_lun_init(void);
extern sint32 cm_cnm_pmm_lun_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_lun_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_lun_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern void cm_pmm_lun_get_data(const sint8 *name,void* data);

extern void cm_pmm_lun_cal_data(void* olddata,void* newdata,void* data);

extern sint32 cm_pmm_lun_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_lun_current(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_lun_check_data(const sint8* name);

/**********************************nic*****************************************/

typedef struct
{
    uint32 id;
    cm_time_t tmstamp;
    double collisions;
    double ierrors;
    double oerrors;
    double rbytes;	
    double obytes;	
    double util;
    double sat;      
}cm_cnm_pmm_nic_info_t;

extern sint32 cm_cnm_pmm_nic_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_nic_init(void);
extern sint32 cm_cnm_pmm_nic_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_nic_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_nic_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern void cm_pmm_nic_get_data(const sint8 *name,void* data);

extern void cm_pmm_nic_cal_data(void* olddata,void* newdata,void* data);

extern sint32 cm_pmm_nic_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_nic_current(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_nic_check_data(const sint8* name);

/********************************disk******************************************/
typedef struct
{
    uint32 id;
    cm_time_t tmstamp;
    double reads;
    double writes;
    double nread;
    double nread_kb;
    double nwritten;
    double nwritten_kb;
    double rlentime;
    double wlentime;
    double wtime;
    double rtime;
    double snaptime;
}cm_cnm_pmm_disk_info_t;

extern sint32 cm_cnm_pmm_disk_local_get(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_disk_init(void);
extern sint32 cm_cnm_pmm_disk_get(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_disk_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_disk_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern void cm_pmm_disk_get_data(const sint8 *name,void* data);

extern void cm_pmm_disk_cal_data(void* olddata,void* newdata,void* data);

extern sint32 cm_pmm_disk_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_disk_current(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_disk_check_data(const sint8* name);


#endif

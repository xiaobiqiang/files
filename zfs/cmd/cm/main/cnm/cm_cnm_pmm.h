/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pmm.h
 * author     : wbn
 * create date: 2017Äê12ÔÂ21ÈÕ
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_PMM_H_
#define MAIN_CNM_CM_CNM_PMM_H_
#include "cm_omi_types.h"
#include "cm_pmm_common.h"
#include "cm_pmm_types.h"
#define CM_PMM_UNITS 1024.0
extern sint32 cm_cnm_pmm_init(void);

extern sint32 cm_cnm_pmm_cluster_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_pmm_cluster_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pmm_cluster_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pmm_cluster_history(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_pmm_node_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_pmm_node_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pmm_node_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pmm_node_history(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_pmm_cfg_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_pmm_cfg_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pmm_cfg_get(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_pmm_cfg_update(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern void cm_cnm_pmm_cfg_oplog_update(
    const sint8* sessionid,const void *pDecodeParam, sint32 Result);

extern sint32 cm_cnm_decode_pmm(const cm_omi_obj_t ObjParam, uint32 eachsize,
    cm_cnm_decode_cbk_t cbk, void** ppAck);

typedef struct
{
    uint32 id;
    cm_time_t tmstamp;
    double newfile;
    double rmfile;
    double readdir;
    double iops;
    double reads;
    double writes;
    double delay;
}cm_pmm_nas_data_t;



extern void cm_pmm_nas_get_data(const sint8 *name,void* data);
extern void cm_pmm_nas_cal_data(void* olddata,void* newdata,void* data);

extern sint32 cm_cnm_pmm_nas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_nas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pmm_nas_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_nas_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

typedef struct
{
    uint32 id;
    cm_time_t tmstamp;
    double reads;
    double writes;    
    double riops;
    double wiops;
}cm_pmm_sas_data_t;

extern void cm_pmm_sas_get_data(const sint8 *name,void* data);
extern void cm_pmm_sas_cal_data(void* olddata,void* newdata,void* data);
extern sint32 cm_pmm_sas_check_data(const sint8* name);


extern sint32 cm_cnm_pmm_sas_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_sas_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pmm_sas_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_sas_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

typedef struct
{
    uint32 id;
    cm_time_t tmstamp;    
    double iops;
}cm_pmm_protocol_data_t;


extern void cm_pmm_protocol_get_data(const sint8 *name,void* data);
extern void cm_pmm_protocol_cal_data(void* olddata,void* newdata,void* data);
extern sint32 cm_pmm_protocol_check_data(const sint8* name);

extern sint32 cm_cnm_pmm_protocol_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_protocol_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pmm_protocol_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_protocol_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

extern void cm_pmm_protocol_cifs_cal_data(void* olddata,void* newdata,void* data);


typedef struct
{
    uint32 id;
    cm_time_t tmstamp;
    double l1_hits;
    double l1_misses;
    double l1_size;
    double l2_hits;
    double l2_misses;
    double l2_size;
}cm_pmm_cache_data_t;

extern void cm_pmm_cache_get_data(const sint8 *name,void* data);
extern void cm_pmm_cache_cal_data(void* olddata,void* newdata,void* data);

extern sint32 cm_cnm_pmm_cache_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_cache_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_pmm_cache_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_cache_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);


typedef struct
{
    sint8 name[CM_STRING_128];
    uint32 nid;
}cm_pmm_name_t;
extern cm_omi_obj_t cm_cnm_name_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);


extern sint32 cm_cnm_sas_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_sas_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_sas_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_sas_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_protocol_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_protocol_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_protocol_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_protocol_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);



#endif /* MAIN_CNM_CM_CNM_PMM_H_ */

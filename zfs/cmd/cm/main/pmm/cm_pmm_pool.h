/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pmm_pool.h
 * author     : xar
 * create date: 2018.10.17
 * description: TODO:
 *
 *****************************************************************************/


#ifndef MAIN_CNM_PMM_POOL
#define MAIN_CNM_PMM_POOL
#include "cm_cnm_common.h"
#include "cm_pmm_types.h" 

typedef struct
{
    uint32 id;
    cm_time_t datatime;
    double riops;
    double wiops;
    double rbitys;
    double wbitys;
}cm_pmm_pool_info_t;

extern void cm_pmm_pool_get_data(const sint8 *name,void* data);

extern void cm_pmm_pool_cal_data(void* olddata,void* newdata,void* data);

extern sint32 cm_cnm_pmm_pool_decode(
    const cm_omi_obj_t  ObjParam,void** ppDecodeParam);

extern cm_omi_obj_t cm_cnm_pmm_pool_encode(
    const void* pDecodeParam,void* pAckData,uint32 AckLen);

extern sint32 cm_cnm_pmm_pool_current(const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_pool_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_pmm_pool_check_data(const sint8* name);

#endif 


/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_topo_power.h
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_CNM_CM_CNM_TOPO_POWER_H_
#define MAIN_CNM_CM_CNM_TOPO_POWER_H_
#include "cm_cnm_common.h"
#include "cm_log.h"
#include "cm_db.h"  
#include "cm_omi.h"  
#include "cm_cnm_expansion_cabinet.h"
typedef struct
{  
    uint32 enid;
    sint8 power[CM_STRING_256];
    double volts;
    uint32 status;
}cm_cnm_topo_power_info_t;

extern sint32 cm_cnm_topo_power_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_topo_power_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_topo_power_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_topo_power_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_topo_power_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_topo_power_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

typedef struct
{    
    uint32 enid;
    uint32 nid;
    sint8 fan[CM_STRING_256];
    uint32 rotate;
    uint32 status;
}cm_cnm_topo_fan_info_t;

extern sint32 cm_cnm_topo_fan_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);

extern cm_omi_obj_t cm_cnm_topo_fan_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);

extern sint32 cm_cnm_topo_fan_getbatch(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
    
extern sint32 cm_cnm_topo_fan_count(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);

extern sint32 cm_cnm_topo_fan_local_getbatch(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

extern sint32 cm_cnm_topo_fan_local_count(
    void *param, uint32 len,
    uint64 offset, uint32 total, 
    void **ppAck, uint32 *pAckLen);

typedef struct
{    
    sint8 name[CM_STRING_64];
    sint8 sn[CM_STRING_64];
    uint32 set; 
    uint32 enid;
    uint8 type;
    uint32 U_num;
    uint32 slot_num;
}cm_cnm_topo_table_info_t;
extern sint32 cm_cnm_topo_table_init(void);
extern sint32 cm_cnm_topo_table_get(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_topo_table_delete(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32 cm_cnm_topo_table_update(
        const void *pDecodeParam,void **ppAckData, uint32 *pAckLen);
extern sint32  cm_cnm_topo_table_insert(
        void *pDecodeParam,void **ppAckData,uint32 * pAckLen);
extern sint32 cm_cnm_topo_table_count(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);
extern sint32 cm_cnm_topo_table_getbatch(
    const void * pDecodeParam,void **ppAckData,uint32 *pAckLen);
extern sint32  cm_cnm_topo_table_off(
        void *pDecodeParam,void **ppAckData,uint32 * pAckLen);
extern sint32 cm_cnm_topo_table_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam);
extern cm_omi_obj_t cm_cnm_topo_table_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen);
extern sint32 cm_cnm_topo_table_sync_request(uint64 enid, void *pdata, uint32 len);
extern sint32 cm_cnm_topo_table_sync_get(uint64 enid, void **pdata, uint32 *plen);
extern sint32 cm_cnm_topo_table_sync_delete(uint64 enid);

#endif 


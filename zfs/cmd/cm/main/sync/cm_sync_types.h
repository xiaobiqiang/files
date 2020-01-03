/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_sync_types.h
 * author     : wbn
 * create date: 2017��10��27��
 * description: TODO:
 *
 *****************************************************************************/
#ifndef MAIN_SYNC_CM_SYNC_TYPES_H_
#define MAIN_SYNC_CM_SYNC_TYPES_H_
#include "cm_common.h"
#include "cm_cfg_local.h" 
#include "cm_cfg_global.h" 

typedef sint32 (*cm_sync_request_cbk_t)(uint64 data_id, void *pdata, uint32 len);
typedef sint32 (*cm_sync_get_cbk_t)(uint64 data_id, void **pdata, uint32 *plen);
typedef sint32 (*cm_sync_delete_cbk_t)(uint64 data_id);

/*  Ĭ�ϼ�Ⱥ��ͬ��,����֮�����0������ͬ�� */
#define CM_SYNC_FLAG_MASTER_DOMAIN 1

/* Ĭ��1����¼������֮��֧�ֶ�����¼ */
#define CM_SYNC_FLAG_MULTI_RECORD 2

/* Ĭ��֧�ַ�ʵʱͬ��������֮���֧��ʵʱͬ�� */
#define CM_SYNC_FLAG_REAL_TIME_ONLY 4

typedef struct
{
    uint32 obj_id;
    uint32 mask;
    cm_sync_request_cbk_t cbk_request;
    cm_sync_get_cbk_t cbk_get;
    cm_sync_delete_cbk_t cbk_delete;
}cm_sync_config_t;


#endif /* MAIN_SYNC_CM_SYNC_TYPES_H_ */

/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_pmm_common.h
 * author     : xar
 * create date: 2018.10.24
 * description: TODO:
 *
 *****************************************************************************/

#ifndef MAIN_PMM_COMMON_
#define MAIN_PMM_COMMON_


#include "cm_omi_types.h"
#include "cm_cnm_common.h"
#include "cm_define.h"
#include "cm_cnm_pmm.h"
#define CM_PMM_TIME_INTERVAL 5.00

typedef void(*cm_pmm_get_data)(const sint8 *name,void* data);
typedef void(*cm_pmm_cal_data)(void* olddata,void* newdata,void* data);
typedef sint32(*cm_pmm_check_data)(const sint8 *name);

typedef struct
{
    uint32 type;
    uint32 datalen;
    
    cm_pmm_get_data get_data;
    cm_pmm_cal_data cal_data;
}cm_pmm_common_cfg_t;


typedef struct
{
    sint8 name[CM_STRING_128];
    cm_time_t timeout;
    void* olddata;
    void* newdata;
    void* statdata;
}cm_pmm_common_cincfog_t;


typedef struct cm_pmm_tree_node_tx
{
    struct cm_pmm_tree_node_tx *pbrother;
    struct cm_pmm_tree_node_tx *pchildren;
    sint8 name[CM_STRING_128];
    uint32 type;
    uint32 child_num;
    void *pdata;
}cm_pmm_tree_node_t;

typedef struct
{
    uint32 type;
    uint32 size;
    cm_pmm_get_data get_data;
    cm_pmm_cal_data cal_data;
    cm_pmm_check_data check_data;
}cm_pmm_config_t;

typedef enum 
{
    CM_PMM_TYPE_POOL=0,
    CM_PMM_TYPE_NAS,
    CM_PMM_TYPE_SAS,
    CM_PMM_TYPE_PROTO,
    CM_PMM_TYPE_CACHE,
	CM_PMM_TYPE_LUN,
    CM_PMM_TYPE_NIC,
    CM_PMM_TYPE_DISK,
    CM_PMM_TYPE_CIFS,
    CM_PMM_TYPE_BUTT,
}cm_pmm_node_type_e;

extern cm_pmm_tree_node_t* cm_pmm_node_root;
extern cm_mutex_t g_cm_pmm_common_mutex;

extern cm_pmm_tree_node_t* 
cm_pmm_tree_node_new(const sint8* name);

extern cm_pmm_tree_node_t* 
cm_pmm_tree_node_addnew(cm_pmm_tree_node_t *pParent,const sint8* name);

extern cm_pmm_tree_node_t* 
cm_pmm_tree_node_find(cm_pmm_tree_node_t *pParent,const sint8* name);

extern sint32 
cm_pmm_tree_node_add(cm_pmm_tree_node_t *pParent,cm_pmm_tree_node_t *pChild);

extern sint32
cm_pmm_tree_node_delete(cm_pmm_tree_node_t *pParent,cm_pmm_tree_node_t *pChild);

extern sint32 
cm_pmm_tree_node_free(cm_pmm_tree_node_t *pNode);

extern sint32 
cm_pmm_tree_destory(cm_pmm_tree_node_t *pRoot);

extern sint32 
cm_pmm_common_get_data();

extern void cm_pmm_tree_node_update(const cm_pmm_tree_node_t* root);

#endif



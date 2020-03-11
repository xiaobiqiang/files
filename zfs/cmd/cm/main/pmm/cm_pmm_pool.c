 /******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pmm_pool.h
 * author     : xar
 * create date: 2018.10.17
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_pmm_pool.h"
#include "cm_log.h"
#include "cm_node.h"
#include "cm_omi.h"
#include "cm_db.h"
#include "cm_pmm_common.h"

sint32 cm_cnm_pmm_pool_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{

    return cm_pmm_decode(CM_OMI_OBJECT_PMM_POOL,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_pool_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_POOL,pDecodeParam,pAckData,AckLen);
}

sint32 cm_cnm_pmm_pool_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_POOL,pDecodeParam,ppAckData,pAckLen);
}


sint32 cm_pmm_pool_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    const cm_pmm_decode_t *pdecode = (const cm_pmm_decode_t*)pDecode;
    cm_pmm_pool_info_t *data = NULL;
    sint32 iRet = CM_OK;
    
    data = CM_MALLOC(sizeof(cm_pmm_pool_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }

    CM_MEM_ZERO(data,sizeof(cm_pmm_pool_info_t));
    iRet = cm_pmm_common_get_data(CM_PMM_TYPE_POOL,pdecode->param,data);
    if(CM_OK != iRet)
    {
        CM_FREE(data);
        *ppAckData = NULL;
        *pAckLen = 0;
        return CM_OK;
    }
    data->id = 0;
    
    *ppAckData = data;
    *pAckLen = sizeof(cm_pmm_pool_info_t);

    return CM_OK;
}


static sint32 cm_pmm_pool_get_data_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_pmm_pool_info_t* info = arg;
    
    info->rbitys = atof(cols[0]);
    info->wbitys = atof(cols[1]);
    info->riops= atof(cols[2]);
    info->wiops= atof(cols[3]);

    return CM_OK;
}

void cm_pmm_pool_get_data(const sint8 *name,void* data)
{
    cm_pmm_pool_info_t* info = (cm_pmm_pool_info_t *)data;
    sint32 iRet = CM_OK;
 
    iRet = cm_cnm_exec_get_col(cm_pmm_pool_get_data_each,info,
        CM_SHELL_EXEC" cm_pmm_pool %s",name);
    return;
}

void cm_pmm_pool_cal_data(void* olddata,void* newdata,void* data)
{
    cm_pmm_pool_info_t *oldinfo = (cm_pmm_pool_info_t *)olddata;
    cm_pmm_pool_info_t *newinfo = (cm_pmm_pool_info_t *)newdata;
    cm_pmm_pool_info_t *info = (cm_pmm_pool_info_t *)data;
	
    CM_PMM_NODE_DATA_CAL(info->riops,oldinfo->riops,newinfo->riops,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->wiops,oldinfo->wiops,newinfo->wiops,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->rbitys,oldinfo->rbitys,newinfo->rbitys,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->wbitys,oldinfo->wbitys,newinfo->wbitys,CM_PMM_TIME_INTERVAL);
    info->rbitys /= 1024;
    info->wbitys /= 1024;

    return;
}

sint32 cm_pmm_pool_check_data(const sint8* name)
{
    uint32 cut = 0;
    cut = cm_exec_int("cat /tmp/pool.xml |grep 'name=\"%s\"'|wc -l",name);
    if(0 == cut)
    {
        return CM_FAIL;
    }
    return CM_OK;
}


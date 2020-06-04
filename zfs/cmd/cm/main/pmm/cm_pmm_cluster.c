/******************************************************************************
 *          Copyright (c) 2017 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_pmm_cluster.c
 * author     : wbn
 * create date: 2017年12月27日
 * description: TODO:
 *
 *****************************************************************************/
#include "cm_omi_types.h"
#include "cm_db.h"
#include "cm_cmt.h"
#include "cm_omi.h"
#include "cm_log.h"
#include "cm_pmm_types.h"
#include "cm_node.h"
#include "cm_pmm.h"
#include "cm_sync.h"

typedef struct
{
    cm_pmm_cluster_data_t data;
    uint32 cnt;
}cm_pmm_cluster_data_sum_t;

static cm_pmm_cluster_data_sum_t g_pmm_subdomain_data;
static cm_pmm_cluster_data_sum_t g_pmm_cluster_data;

extern cm_mutex_t g_cm_pmm_mutex;

#define CM_PMM_DATA_ID_CLUSTER 0
#define CM_PMM_DATA_ID_SUBDOMAIN 1

sint32 cm_pmm_cluster_init(void)
{
    CM_MEM_ZERO(&g_pmm_cluster_data,sizeof(cm_pmm_cluster_data_sum_t));
    CM_MEM_ZERO(&g_pmm_subdomain_data,sizeof(cm_pmm_cluster_data_sum_t));
    return CM_OK;
}

static void cm_pmm_cluster_update(cm_pmm_cluster_data_sum_t *pSum)
{
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    CM_MEM_CPY(&g_pmm_cluster_data,sizeof(cm_pmm_cluster_data_sum_t),
        pSum,sizeof(cm_pmm_cluster_data_sum_t));
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    
    (void)cm_pmm_db_update(CM_OMI_OBJECT_PMM_CLUSTER, (cm_pmm_data_t*)&pSum->data);
    return;
}

static void cm_pmm_subdomain_update(cm_pmm_cluster_data_sum_t *pSum)
{
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    CM_MEM_CPY(&g_pmm_subdomain_data,sizeof(cm_pmm_cluster_data_sum_t),
        pSum,sizeof(cm_pmm_cluster_data_sum_t));
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    return;
}

sint32 cm_pmm_cluster_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    const cm_pmm_decode_t *pdecode = (const cm_pmm_decode_t*)pDecode;
    uint32 len = 0;
    void *pData = NULL;
    void *pSrc = NULL;

    if(NULL == pdecode)
    {
        return CM_FAIL;
    }
    
    if(pdecode->id == CM_PMM_DATA_ID_CLUSTER)
    {
        len = sizeof(cm_pmm_cluster_data_t);
        pSrc = &g_pmm_cluster_data.data;
    }
    else
    {
        len = sizeof(cm_pmm_cluster_data_sum_t);
        pSrc = &g_pmm_subdomain_data;
    }
    
    pData = CM_MALLOC(len);
    if(NULL == pData)
    {
        CM_LOG_ERR(CM_MOD_PMM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    CM_MEM_CPY(pData,len,pSrc,len);
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);

    *ppAckData = pData;
    *pAckLen = len;
    return CM_OK;
}

static sint32 cm_pmm_cluster_node_each(
    cm_node_info_t *pNode, cm_pmm_cluster_data_sum_t *pSum)
{
    sint32 iRet = CM_FAIL;
    cm_pmm_node_data_t data;
    cm_pmm_node_data_t *pdata = NULL;
    cm_pmm_cluster_data_t *pSumData = &pSum->data;
    uint32 len = 0;
    
    iRet = cm_pmm_current_mem(
        CM_OMI_OBJECT_PMM_NODE,0,pNode->id,(void**)&pdata,&len);
    if((CM_OK != iRet) || (NULL == pdata))
    {
        CM_LOG_INFO(CM_MOD_PMM,"nid[%u], fail[%d]",pNode->id,iRet);
        return iRet;
    }
    
    if(len != sizeof(cm_pmm_node_data_t))
    {
        CM_LOG_ERR(CM_MOD_PMM,"len[%u]",len);
        CM_FREE(pdata);
        return CM_FAIL;
    }
    CM_MEM_CPY(&data,sizeof(cm_pmm_node_data_t),pdata,sizeof(cm_pmm_node_data_t));
    CM_FREE(pdata);
    pdata = &data;
    pSumData->cpu_max = CM_MAX(pSumData->cpu_max,pdata->cpu);
    pSumData->cpu_avg += pdata->cpu;
    pSumData->mem_max = CM_MAX(pSumData->mem_max,pdata->mem);
    pSumData->mem_avg += pdata->mem;
    pSumData->bw_read += pdata->bw_read;
    pSumData->bw_total += pdata->bw_total;
    pSumData->bw_write += pdata->bw_write;
    pSumData->iops_read += pdata->iops_read;
    pSumData->iops_write += pdata->iops_write;
    pSumData->iops_total += pdata->iops_total;
    pSum->cnt += 1;
    
    return CM_OK;    
}

static sint32 cm_pmm_cluster_subdomain_each(
    cm_subdomain_info_t *pinfo, cm_pmm_cluster_data_sum_t *pSum)
{
    sint32 iRet = CM_FAIL;
    cm_pmm_cluster_data_sum_t data;
    cm_pmm_cluster_data_sum_t *ptmp = NULL;
    cm_pmm_cluster_data_t *pSumData = &pSum->data;
    cm_pmm_cluster_data_t *pdata = NULL;
    uint32 len = 0;
    
    iRet = cm_pmm_current_mem(CM_OMI_OBJECT_PMM_CLUSTER,CM_PMM_DATA_ID_SUBDOMAIN,
        pinfo->submaster_id,(void**)&ptmp,&len);
    if((CM_OK != iRet) || (NULL == ptmp))
    {
        CM_LOG_INFO(CM_MOD_PMM,"id[%d] nid[%u], fail[%d]",pinfo->id,pinfo->submaster_id,iRet);
        return iRet;
    }
    
    if(len != sizeof(cm_pmm_cluster_data_sum_t))
    {
        CM_LOG_ERR(CM_MOD_PMM,"len[%u]",len);
        CM_FREE(ptmp);
        return CM_FAIL;
    }
    CM_MEM_CPY(&data,sizeof(cm_pmm_cluster_data_sum_t),ptmp,sizeof(cm_pmm_cluster_data_sum_t));
    CM_FREE(ptmp);
    ptmp = &data;
    pdata = &ptmp->data;
    pSumData->cpu_max = CM_MAX(pSumData->cpu_max,pdata->cpu_max);
    pSumData->cpu_avg += pdata->cpu_avg;
    pSumData->mem_max = CM_MAX(pSumData->mem_max,pdata->mem_max);
    pSumData->mem_avg += pdata->mem_avg;
    pSumData->bw_read += pdata->bw_read;
    pSumData->bw_total += pdata->bw_total;
    pSumData->bw_write += pdata->bw_write;
    pSumData->iops_read += pdata->iops_read;
    pSumData->iops_write += pdata->iops_write;
    pSumData->iops_total += pdata->iops_total;
    
    pSum->cnt += ptmp->cnt;
    
    return CM_OK;    
}

void cm_pmm_cluster_main(struct tm *tin)
{
    cm_pmm_cluster_data_sum_t sum;
    cm_pmm_cluster_data_t *pdata = NULL;
    uint32 local_id = cm_node_get_id();
    uint32 subdomain_id = cm_node_get_subdomain_id();
    uint32 submaster = cm_node_get_submaster(subdomain_id);
       
    if(0 != (tin->tm_sec % 5))
    {
        return;
    }

    if(local_id != submaster)
    {
        /* 仅子域主处理 */
        return;
    }
    
    CM_MEM_ZERO(&sum,sizeof(sum));
    
    (void)cm_node_traversal_node(subdomain_id,
        (cm_node_trav_func_t)cm_pmm_cluster_node_each,&sum,CM_FALSE);
    cm_pmm_subdomain_update(&sum);
    
    if(CM_MASTER_SUBDOMAIN_ID != subdomain_id)
    {
        return;
    }
    
    CM_MEM_ZERO(&sum,sizeof(sum));
    (void)cm_node_traversal_subdomain(
        (cm_node_trav_func_t)cm_pmm_cluster_subdomain_each,&sum,CM_FALSE);
    pdata = &sum.data;
    pdata->tmstamp = cm_get_time();
    if(sum.cnt > 1)
    {        
        pdata->cpu_avg = pdata->cpu_avg / sum.cnt;
        pdata->mem_avg = pdata->mem_avg / sum.cnt;        
    }
    
    (void)cm_sync_request(CM_SYNC_OBJ_PMM,0,&sum,sizeof(sum));
    return;
}

sint32 cm_pmm_cbk_sync_request(uint64 data_id, void *pdata, uint32 len)
{
    cm_pmm_cluster_data_sum_t *pSum = pdata;
    cm_pmm_cluster_update(pSum);
    return CM_OK;
}



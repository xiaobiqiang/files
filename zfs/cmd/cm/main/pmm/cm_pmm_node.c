/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_pmm_node.c
 * author     : wbn
 * create date: 2018Äê1ÔÂ2ÈÕ
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
#include "cm_alarm.h"

typedef struct
{
    cm_time_t currtime;
    uint64 rbytes;
    uint64 obytes;
    uint64 cpu_ticks;
    uint64 cpu_ticks_idle;
    uint64 riops;
    uint64 wiops;
    double mem;
}cm_pmm_node_stat_t;

static cm_pmm_node_stat_t g_pmm_node_stat;
static cm_pmm_node_data_t g_pmm_node_data; 
extern cm_mutex_t g_cm_pmm_mutex;

static void cm_pmm_node_stats(cm_pmm_node_stat_t *info)
{
    sint8 buff[CM_STRING_256] = {0};
    
    CM_MEM_ZERO(info,sizeof(cm_pmm_node_stat_t));
    
    info->currtime = cm_get_time();
    cm_exec_tmout(buff,sizeof(buff),10,CM_SHELL_EXEC" cm_pmm_node_stat");
    sscanf(buff,"%llu %llu %llu %llu %llu %llu %lf",
        &info->rbytes,&info->obytes,&info->riops,&info->wiops,
        &info->cpu_ticks,&info->cpu_ticks_idle, &info->mem);
    return;
}


static void cm_pmm_node_get_stat(cm_pmm_node_stat_t *old,
    cm_pmm_node_data_t *data)
{
    cm_pmm_node_stat_t curr;
    cm_time_t diff = 0;

    cm_pmm_node_stats(&curr);
    if(curr.currtime <= old->currtime)
    {
        CM_LOG_WARNING(CM_MOD_PMM,"old[%lu] curr[%lu]",old->currtime,curr.currtime);
        CM_MEM_CPY(old,sizeof(curr),&curr,sizeof(curr));
        return;
    }
    diff = curr.currtime - old->currtime;
    if(diff < 1)
    {
        diff = 1;
    }
    CM_PMM_NODE_DATA_CAL(data->bw_read,old->rbytes,curr.rbytes,diff);
    CM_PMM_NODE_DATA_CAL(data->bw_write,old->obytes,curr.obytes,diff);
    
    data->bw_read /= 1024;
    data->bw_write /= 1024;
    data->bw_total = data->bw_read + data->bw_write;
#ifdef __linux__
    data->iops_read = (double)curr.riops;
    data->iops_write = (double)curr.wiops;
    data->iops_total = data->iops_read + data->iops_write;
    data->cpu = (double)curr.cpu_ticks;
#else
    CM_PMM_NODE_DATA_CAL(data->iops_read,old->riops,curr.riops,diff);
    CM_PMM_NODE_DATA_CAL(data->iops_write,old->wiops,curr.wiops,diff);
    data->iops_total = data->iops_read + data->iops_write;
    data->cpu = ((double)(curr.cpu_ticks_idle - old->cpu_ticks_idle))/(curr.cpu_ticks - old->cpu_ticks);
    data->cpu = (1.0-data->cpu)*100;     
    data->mem = curr.mem;
#endif
    CM_MEM_CPY(old,sizeof(curr),&curr,sizeof(curr));
    
    return;
}

sint32 cm_pmm_node_init(void)
{
    CM_MEM_ZERO(&g_pmm_node_data,sizeof(cm_pmm_node_data_t));
    (void)cm_pmm_node_stats(&g_pmm_node_stat);
    return CM_OK;
}

static void cm_pmm_node_update(cm_pmm_node_data_t *pdata)
{
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    CM_MEM_CPY(&g_pmm_node_data,sizeof(cm_pmm_node_data_t),
        pdata,sizeof(cm_pmm_node_data_t));
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);
    
    (void)cm_pmm_db_update(CM_OMI_OBJECT_PMM_NODE, (cm_pmm_data_t*)pdata);
    return;
}

sint32 cm_pmm_node_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    uint32 len = sizeof(cm_pmm_node_data_t);
    void *pData = CM_MALLOC(len);

    if(NULL == pData)
    {
        CM_LOG_ERR(CM_MOD_PMM,"malloc fail");
        return CM_FAIL;
    }
    
    CM_MUTEX_LOCK(&g_cm_pmm_mutex);
    CM_MEM_CPY(pData,sizeof(cm_pmm_node_data_t),
        &g_pmm_node_data,sizeof(cm_pmm_node_data_t));
    CM_MUTEX_UNLOCK(&g_cm_pmm_mutex);

    *ppAckData = pData;
    *pAckLen = len;
    return CM_OK;
}

static void cm_pmm_node_check_object(uint32 cur,uint32 alarm_id, uint8 *reportflag,uint32 *set_cnt)
{
    uint32 report = 0;
    uint32 recovery = 0;
    sint8 buff[128] = {0};
    sint32 iRet = cm_alarm_threshold_get(alarm_id, &report, &recovery);
    if(CM_OK != iRet)
    {
        return; 
    }
    if((cur >= report) && (*reportflag == CM_FALSE) && (*set_cnt < CM_ALARM_CHECK_CNT))
    {
        *set_cnt = *set_cnt + 1;
        return;
    }
    if((cur >= report) && (*reportflag == CM_FALSE) && (*set_cnt == CM_ALARM_CHECK_CNT))
    {
        CM_VSPRINTF(buff, sizeof(buff), "%s,%u",
            cm_node_get_name(CM_NODE_ID_LOCAL),cur);
        iRet = CM_ALARM_REPORT(alarm_id, buff);
        if(iRet == CM_OK)
        {
            *reportflag = CM_TRUE;
            *set_cnt = 0;
        }        
        return;
    }
    if((cur <= recovery) && (*reportflag == CM_TRUE))
    {
        CM_VSPRINTF(buff, sizeof(buff), "%s,%u",
            cm_node_get_name(CM_NODE_ID_LOCAL),cur);
        iRet = CM_ALARM_RECOVERY(alarm_id, buff);
        if(iRet == CM_OK)
        {
            *reportflag = CM_FALSE;
        }
    }
    return;
}

void cm_pmm_node_main(struct tm *tin)
{
    static uint32 cnt_cpu = 0;
    static uint32 cnt_mem = 0;
    cm_pmm_node_data_t data;
    static uint8 cpu_flag = CM_FALSE;
    static uint8 mem_flag = CM_FALSE;
    if(0 != (tin->tm_sec % 5))
    {
        return;
    }
    CM_MEM_ZERO(&data,sizeof(cm_pmm_node_data_t));
    data.id = cm_node_get_id();
    cm_pmm_node_get_stat(&g_pmm_node_stat,&data);
    data.tmstamp = cm_get_time();
    cm_pmm_node_check_object((uint32)data.cpu,CM_ALARM_FAULT_CPU_RATE,&cpu_flag,&cnt_cpu);
    cm_pmm_node_check_object((uint32)data.mem,CM_ALARM_FAULT_MEM_RATE,&mem_flag,&cnt_mem);
    cm_pmm_node_update(&data);
    
    return;
}


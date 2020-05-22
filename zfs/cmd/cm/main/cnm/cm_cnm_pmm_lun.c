/******************************************************************************
 *          Copyright (c) 2018 by Ceresdata. All rights reserved
 ******************************************************************************
 * filename   : cm_cnm_pmm_lun.c
 * author     : wbn
 * create date: 2018.07.07
 * description: TODO:
 *
 *****************************************************************************/

#include "cm_log.h"
#include "cm_omi.h"
#include "cm_pmm.h"
#include "cm_cnm_pmm_lun.h"
#include "cm_cnm_common.h"
#include "cm_pmm_common.h"

#define LINUX CM_TRUE

sint32 cm_cnm_pmm_lun_init(void)
{
    return CM_OK;
}

void cm_cnm_lun_get_stmf_lu(const sint8* name,const sint8* id)
{
    sint32 iRet = CM_OK;
    sint8 buff[CM_STRING_128] = {0};
    iRet = cm_exec_tmout(buff,sizeof(buff),CM_CMT_REQ_TMOUT,
        CM_SHELL_EXEC" cm_pmm_get_lu %s",name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n",iRet);
        return;
    }
    if(buff[0] == '\0')
    {
        return; 
    }
    CM_VSPRINTF(id,CM_STRING_128,"stmf_lu_io_%s",buff);
    return;
}

static sint32 cm_cnm_pmm_lun_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_pmm_lun_info_t *info = arg;
    const uint32 def_num = 4;       
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }   
    info->nread = atof(cols[0]);
    info->nwritten = atof(cols[1]);
    info->reads = atof(cols[2]);
    info->writes = atof(cols[3]);
    info->tmstamp = cm_get_time();
    return CM_OK;
}

void cm_pmm_lun_get_data(const sint8 *name,void* data) 
{
    cm_cnm_pmm_lun_info_t* info = (cm_cnm_pmm_lun_info_t *)data;
    sint32 iRet = CM_OK;
    CM_MEM_ZERO(info,sizeof(cm_cnm_pmm_lun_info_t));
    iRet = cm_cnm_exec_get_col(cm_cnm_pmm_lun_local_get_each,info,
            CM_SHELL_EXEC" cm_pmm_lun %s",name);  
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_lun_col_fail",iRet);
        return;
    }
    return;
}

void cm_pmm_lun_cal_data(void* olddata,void* newdata,void* data)
{
    cm_cnm_pmm_lun_info_t *pre = (cm_cnm_pmm_lun_info_t *)olddata;
    cm_cnm_pmm_lun_info_t *cur = (cm_cnm_pmm_lun_info_t *)newdata;
    cm_cnm_pmm_lun_info_t *info = (cm_cnm_pmm_lun_info_t *)data;

    CM_PMM_NODE_DATA_CAL(info->reads,pre->reads,cur->reads,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->writes,pre->writes,cur->writes,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->nread,pre->nread,cur->nread,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->nwritten,pre->nwritten,cur->nwritten,CM_PMM_TIME_INTERVAL);
    info->nread_kb = (info->nread)/1024;
    info->nwritten_kb = (info->nwritten)/1024;
    return;
}

sint32 cm_cnm_pmm_lun_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_LUN,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_pmm_lun_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    const cm_pmm_decode_t *pdecode = (const cm_pmm_decode_t*)pDecode;
    cm_cnm_pmm_lun_info_t *data = NULL;
    sint32 iRet = CM_OK;
    
    data = CM_MALLOC(sizeof(cm_cnm_pmm_lun_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(data,sizeof(cm_cnm_pmm_lun_info_t));
    iRet = cm_pmm_common_get_data(CM_PMM_TYPE_LUN,pdecode->param,data);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_pmm_common_get_lun_data fail");
         CM_FREE(data);
        *ppAckData = NULL;
        *pAckLen = 0;
        return CM_OK;
    } 
    data->id = 0;
    *ppAckData = data;
    *pAckLen = sizeof(cm_cnm_pmm_lun_info_t);

    return CM_OK;
}

sint32 cm_cnm_pmm_lun_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_LUN,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_lun_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_LUN,pDecodeParam,pAckData,AckLen);
}

sint32 cm_pmm_lun_check_data(const sint8* name)
{
    uint32 cnt = 0;
    cnt = cm_exec_int(CM_SHELL_EXEC" cm_pmm_lun_check %s",name);
    if(0 == cnt)
    {
        return CM_FAIL;
    }
    return CM_OK;
}
/************************************nic***************************************/

sint32 cm_cnm_pmm_nic_init(void)
{
    return CM_OK;
}

double cm_cnm_pmm_nic_fetch(double val,double val64)
{
   if(val64 != val) return val64;
   return val;
}
static sint32 cm_cnm_pmm_nic_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_pmm_nic_info_t *info = arg;
    const uint32 def_num = 11; 
    double nocp = 0.0;
    double noxmtbuf = 0.0;
    uint32 ifspeed = 0;
    uint32 duplex = 0;
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }    
    info->collisions = atof(cols[0]);
    info->ierrors = atof(cols[1]);
    ifspeed = atof(cols[2]);
    duplex = atof(cols[3]);
    nocp = atof(cols[4]);
    noxmtbuf = atof(cols[5]);
    info->obytes = cm_cnm_pmm_nic_fetch(atof(cols[6]),atof(cols[7])); 
    info->oerrors = atof(cols[8]);
    info->rbytes = cm_cnm_pmm_nic_fetch(atof(cols[9]),atof(cols[10]));
    info->sat = info->collisions + nocp + noxmtbuf;    /*info->sat = info->ierrors + info->oerrors;*/
    info->tmstamp = cm_get_time();
    return CM_OK;
}

void cm_pmm_nic_get_data(const sint8 *name,void* data) 
{
    sint32 iRet = CM_OK;
    sint8 util_sat[CM_STRING_64] = {0};
    cm_cnm_pmm_nic_info_t* info =(cm_cnm_pmm_nic_info_t *)data;
    CM_MEM_ZERO(info,sizeof(cm_cnm_pmm_nic_info_t));
    iRet = cm_cnm_exec_get_col(cm_cnm_pmm_nic_local_get_each,info,
        CM_SHELL_EXEC" cm_pmm_nic %s",name);   
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_col_fail",iRet);
        return;
    }
    return;   
}

void cm_pmm_nic_cal_data(void* olddata,void* newdata,void* data)
{
    cm_cnm_pmm_nic_info_t *pre = (cm_cnm_pmm_nic_info_t *)olddata;
    cm_cnm_pmm_nic_info_t *cur = (cm_cnm_pmm_nic_info_t *)newdata;
    cm_cnm_pmm_nic_info_t *info = (cm_cnm_pmm_nic_info_t *)data;
    
    uint32 t_interval = CM_PMM_TIME_INTERVAL;
    
    CM_PMM_NODE_DATA_CAL(info->rbytes,pre->rbytes,cur->rbytes,t_interval);
    CM_PMM_NODE_DATA_CAL(info->obytes,pre->obytes,cur->obytes,t_interval);
    info->rbytes = info->rbytes / 1024;
    info->obytes = info->obytes / 1024;
    CM_PMM_NODE_DATA_CAL(info->ierrors,pre->ierrors,cur->ierrors,t_interval);
    CM_PMM_NODE_DATA_CAL(info->oerrors,pre->oerrors,cur->oerrors,t_interval);
    CM_PMM_NODE_DATA_CAL(info->sat,pre->sat,cur->sat,t_interval);
   
    return;
}

sint32 cm_cnm_pmm_nic_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_NIC,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_pmm_nic_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    char *p = NULL;
    sint32 iRet = CM_OK;
    double ifspeed = 0.0, duplex = 0.0;
    const cm_pmm_decode_t *pdecode = (const cm_pmm_decode_t*)pDecode;
    cm_cnm_pmm_nic_info_t *data = NULL;
    sint8 dup_ifspeed[CM_STRING_64] = {0};
    data = CM_MALLOC(sizeof(cm_cnm_pmm_nic_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(data,sizeof(cm_cnm_pmm_nic_info_t));
    iRet = cm_pmm_common_get_data(CM_PMM_TYPE_NIC,pdecode->param,data);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_pmm_common_get_nic_data fail");
         CM_FREE(data);
        *ppAckData = NULL;
        *pAckLen = 0;
        return CM_OK;
    }
    (void)cm_exec_tmout(dup_ifspeed,sizeof(dup_ifspeed),CM_CMT_REQ_TMOUT,
        CM_SHELL_EXEC" cm_pmm_dup_ifspeed %s",pdecode->param);
    p = strtok(dup_ifspeed," ");
    if(p != NULL)
    {
        ifspeed = atof(p);
    }
    p = strtok(NULL," ");
    if(p != NULL)
    {
        duplex = atof(p);
    }
    if(ifspeed > 0.0) 
    {
        ifspeed = ifspeed / 800 / 1024;
        if(duplex == 2.0)
        {
            if(data->obytes > data->rbytes)
            {
                data->util = (data->obytes) / ifspeed;
            }
            else
            {
                data->util = (data->rbytes) / ifspeed;
            }
        }
        else 
        {
            data->util = (data->obytes + data->rbytes) / ifspeed;
        }
        if((uint32)data->util > 100)
        {
            data->util = 100.0;
        }
    } 
    else
    {
        data->util = 0.0;
    }
    data->id = 0;
    *ppAckData = data;
    *pAckLen = sizeof(cm_cnm_pmm_nic_info_t);
    return CM_OK;
}


sint32 cm_cnm_pmm_nic_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_NIC,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_nic_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_NIC,pDecodeParam,pAckData,AckLen);
}

sint32 cm_pmm_nic_check_data(const sint8* name)
{
    uint32 cnt = 0;
    cnt = cm_exec_int(CM_SHELL_EXEC" cm_pmm_nic_check %s",name);
    if(0 == cnt)
    {
        return CM_FAIL;
    }
    return CM_OK;
}

/************************************disk**************************************/
sint32 cm_cnm_pmm_disk_init(void)
{
    return CM_OK;
}

sint32 cm_cnm_disk_get_instance(const sint8 *name)
{
    sint32 iRet = CM_OK;
    uint32 num = 0;
    sint8 id[CM_STRING_32] = {0};
    iRet = cm_exec_tmout(id,sizeof(id),CM_CMT_REQ_TMOUT,
        CM_SHELL_EXEC" cm_pmm_disk_instance %s",name);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"iRet[%d]\n",iRet);
        return 0;
    }
    num = atoi(id);
    return num;
}

static sint32 cm_cnm_pmm_disk_local_get_each(void *arg, sint8 **cols, uint32 col_num)
{
    cm_cnm_pmm_disk_info_t *info = arg;
    const uint32 def_num = 10;       
    if(def_num != col_num)
    {
        CM_LOG_WARNING(CM_MOD_CNM,"col_num[%u] def_num[%u]",col_num,def_num);
        return CM_FAIL;
    }   
    info->nread = atof(cols[1]);
    info->nwritten = atof(cols[2]);
    info->reads = atof(cols[3]);
    info->rlentime = atof(cols[4]);
    info->rtime = atof(cols[5]);
    info->snaptime = atof(cols[6]);
    info->wlentime = atof(cols[7]);
    info->writes = atof(cols[8]);
    info->wtime = atof(cols[9]);
    info->tmstamp = cm_get_time();
    return CM_OK;
}

void cm_pmm_disk_cal_data(void* olddata,void* newdata,void* data)
{
    cm_cnm_pmm_disk_info_t *pre = (cm_cnm_pmm_disk_info_t *)olddata;
    cm_cnm_pmm_disk_info_t *cur = (cm_cnm_pmm_disk_info_t *)newdata;
    cm_cnm_pmm_disk_info_t *info = (cm_cnm_pmm_disk_info_t *)data;
#if(CM_FALSE == LINUX)
    CM_PMM_NODE_DATA_CAL(info->nread,pre->nread,cur->nread,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->nwritten,pre->nwritten,cur->nwritten,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->reads,pre->reads,cur->reads,CM_PMM_TIME_INTERVAL);
    CM_PMM_NODE_DATA_CAL(info->writes,pre->writes,cur->writes,CM_PMM_TIME_INTERVAL);
    info->nread_kb = info->nread / 1024;
    info->nwritten_kb = info->nwritten / 1024; 
#else
    info->nread = cur->nread * 1024;
    info->nwritten = cur->nwritten * 1024;
    info->reads = cur->reads;
    info->writes = cur->writes;
    info->nread_kb = cur->nread;
    info->nwritten_kb = cur->nwritten;
#endif
    
    info->rtime = cur->rtime;
    info->wtime = cur->wtime;
    info->snaptime = cur->snaptime;
    info->wlentime = cur->wlentime;
    info->rlentime = cur->rlentime;
    return;
}

void cm_pmm_disk_get_data(const sint8 *name,void* data) 
{    

    cm_cnm_pmm_disk_info_t *info = (cm_cnm_pmm_disk_info_t *)data;    
    sint32 iRet = CM_OK;    
    CM_MEM_ZERO(info,sizeof(cm_cnm_pmm_disk_info_t));
#if(CM_FALSE == LINUX)
    uint32 num = (uint32)cm_cnm_disk_get_instance(name);
    iRet = cm_cnm_exec_get_col(cm_cnm_pmm_disk_local_get_each,info,        
        CM_SHELL_EXEC" cm_pmm_disk %u",num);
#else
    iRet = cm_cnm_exec_get_col(cm_cnm_pmm_disk_local_get_each,info,        
        CM_SHELL_EXEC" cm_pmm_disk %s",name);
#endif
    if(CM_OK != iRet)
    {        
    	CM_LOG_ERR(CM_MOD_CNM,"iRet[%d] get_col_fail",iRet);        
        return;    
    }    
    return;
}

sint32 cm_cnm_pmm_disk_current(
    const void *pDecodeParam,void **ppAckData, uint32 *pAckLen)
{
    return cm_pmm_current(CM_OMI_OBJECT_PMM_DISK,pDecodeParam,ppAckData,pAckLen);
}

sint32 cm_pmm_disk_current_mem(const void *pDecode,void **ppAckData, uint32 *pAckLen)
{
    const cm_pmm_decode_t *pdecode = (const cm_pmm_decode_t*)pDecode;
    cm_cnm_pmm_disk_info_t *data = NULL;
    sint32 iRet = CM_OK;

    data = CM_MALLOC(sizeof(cm_cnm_pmm_disk_info_t));
    if(NULL == data)
    {
        CM_LOG_ERR(CM_MOD_CNM,"malloc fail");
        return CM_FAIL;
    }
    CM_MEM_ZERO(data,sizeof(cm_cnm_pmm_disk_info_t));
    iRet = cm_pmm_common_get_data(CM_PMM_TYPE_DISK,pdecode->param,data);
    if(CM_OK != iRet)
    {
        CM_LOG_ERR(CM_MOD_CNM,"cm_pmm_common_get_disk_data fail");
        CM_FREE(data);
        *ppAckData = NULL;
        *pAckLen = 0;
        return CM_OK;
    } 
    data->id = 0;
    *ppAckData = data;
    *pAckLen = sizeof(cm_cnm_pmm_disk_info_t);
    return CM_OK;
}

sint32 cm_cnm_pmm_disk_decode(
    const cm_omi_obj_t ObjParam, void**ppDecodeParam)
{
    return cm_pmm_decode(CM_OMI_OBJECT_PMM_DISK,ObjParam,ppDecodeParam);
}

cm_omi_obj_t cm_cnm_pmm_disk_encode(
    const void *pDecodeParam, void *pAckData, uint32 AckLen)
{
    return cm_pmm_encode(CM_OMI_OBJECT_PMM_DISK,pDecodeParam,pAckData,AckLen);
}

sint32 cm_pmm_disk_check_data(const sint8* name)
{
    uint32 cnt = 0;
    cnt = cm_exec_int(CM_SHELL_EXEC" cm_pmm_disk_check %s",name);
    if(0 == cnt)
    {
        return CM_FAIL;
    }
    return CM_OK;
}

